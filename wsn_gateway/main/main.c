/* example_main.c
 * ESP32 + SX127x LoRa — ESP-IDF
 * Wiring:
 *   GPIO18 = SCK,  GPIO19 = MISO, GPIO23 = MOSI
 *   GPIO5  = NSS,  GPIO4  = DIO0, GPIO2  = NRESET
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sx127x.h"

static const char *TAG = "LORA_EXAMPLE";

/* ================================================================
 *  PIN CONFIG
 * ================================================================ */
#define PIN_SCK     18
#define PIN_MISO    19
#define PIN_MOSI    23
#define PIN_NSS     15
#define PIN_DIO0    26
#define PIN_NRESET  14

/* ================================================================
 *  HAL — implement các function pointer cho SX127x_Dev
 * ================================================================ */
static spi_device_handle_t spi_handle;

static int hal_spi_transfer(uint8_t reg, uint8_t *tx, uint8_t *rx, size_t len) {
    /* Static = BSS section, DMA-capable, 4-byte aligned */
    static uint8_t tx_buf[257];
    static uint8_t rx_buf[257];

    tx_buf[0] = reg;
    if (tx) memcpy(tx_buf + 1, tx, len);
    else    memset(tx_buf + 1, 0x00, len);

    spi_transaction_t t = {
        .length    = (len + 1) * 8,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };

    if (spi_device_transmit(spi_handle, &t) != ESP_OK) return -1;

    if (rx) memcpy(rx, rx_buf + 1, len);
    return 0;
}

static void hal_set_nreset(int8_t level) {
    if (level < 0) {
        /* Float: set về input */
        gpio_set_direction(PIN_NRESET, GPIO_MODE_INPUT);
    } else {
        gpio_set_direction(PIN_NRESET, GPIO_MODE_OUTPUT);
        gpio_set_level(PIN_NRESET, level);
    }
}

static void hal_delay_ms(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static void hal_delay_us(uint32_t us) {
    /* esp_rom_delay_us hoặc busy-wait */
    for (volatile uint32_t i = 0; i < us * 240 / 5; i++);
}

/* ================================================================
 *  CALLBACKS
 * ================================================================ */
static void on_tx_done(SX127x_Dev *dev) {
    ESP_LOGI(TAG, "TX done");
}

static void on_rx_done(SX127x_Dev *dev, uint8_t *data, uint8_t len, int16_t rssi, int8_t snr) {
    ESP_LOGI(TAG, "RX done — %d bytes, RSSI=%d dBm, SNR=%d dB", len, rssi, snr);
    ESP_LOG_BUFFER_HEX(TAG, data, len);
}

/* ================================================================
 *  DIO0 INTERRUPT
 * ================================================================ */
static SX127x_Dev *g_dev;

static void IRAM_ATTR dio0_isr_handler(void *arg) {
    /* Không gọi trực tiếp SX127x_OnDIO0 trong ISR vì có SPI bên trong
     * → dùng task notification để defer ra task */
    BaseType_t woken = pdFALSE;
    extern TaskHandle_t g_lora_task;
    vTaskNotifyGiveFromISR(g_lora_task, &woken);
    portYIELD_FROM_ISR(woken);
}

TaskHandle_t g_lora_task;

/* ================================================================
 *  SPI INIT
 * ================================================================ */
static void spi_init(void) {
    spi_bus_config_t bus = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 1 * 1000 * 1000,  /* Giảm xuống 1 MHz để test trước */
        .mode           = 0,
        .spics_io_num   = PIN_NSS,
        .queue_size     = 4,
        .command_bits   = 0,                 /* ← bỏ */
        .address_bits   = 0,
    };
    spi_bus_add_device(SPI2_HOST, &dev_cfg, &spi_handle);
}

/* ================================================================
 *  LORA TASK
 * ================================================================ */
static void lora_task(void *arg) {
    SX127x_Dev dev = {
        .modem        = SX127X_MODEM_LORA,
        .spi_transfer = hal_spi_transfer,
        .set_nreset   = hal_set_nreset,
        .delay_ms     = hal_delay_ms,
        .delay_us     = hal_delay_us,
        .txdone_cb    = on_tx_done,
        .rxdone_cb    = on_rx_done,
    };
    g_dev = &dev;

    SX127x_Config config = {
        .lora = {
            .frequency_hz     = 433000000UL,
            .bandwidth        = SX127X_LORA_BW_125K,
            .spreading_factor = SX127X_LORA_SF_7,
            .coding_rate      = SX127X_LORA_CR_4_5,
            .preamble_length  = 8,
            .tx_power_dbm     = 17,
            .sync_word        = 0x12,
            .crc_enable       = true,
            .implicit_header  = false,
        }
    };

    /* Reset + Init */
    SX127x_Reset(&dev, SX127X_RESET_MANUAL);

    SX127x_Status status = SX127x_Init(&dev, &config);
    if (status != SX127X_OK) {
        ESP_LOGE(TAG, "Init failed: %d", status);
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "SX127x ready");

    /* TX một gói, sau đó chuyển sang RX continuous */
    uint8_t msg[] = "Hello LoRa";
    SX127x_Payload tx_payload = { .data = msg, .length = sizeof(msg) - 1 };
    SX127x_Send(&dev, &tx_payload);
    ESP_LOGI(TAG, "Sent: %s", msg);

    /* Chờ TxDone qua interrupt, rồi bật RX */
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(5000));
    SX127x_OnDIO0(&dev);

    SX127x_StartReceive(&dev, true);
    ESP_LOGI(TAG, "Listening...");

    /* Loop: chờ interrupt từ DIO0 */
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        SX127x_OnDIO0(&dev);
    }
}

/* ================================================================
 *  APP MAIN
 * ================================================================ */
void app_main(void) {
    spi_init();

    /* DIO0 interrupt — rising edge */
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << PIN_DIO0),
        .mode         = GPIO_MODE_INPUT,
        .intr_type    = GPIO_INTR_POSEDGE,
    };
    gpio_config(&io);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_DIO0, dio0_isr_handler, NULL);

    xTaskCreate(lora_task, "lora_task", 4096, NULL, 5, &g_lora_task);
}