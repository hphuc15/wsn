/* The sx127x module is coming soon */
#include "receiver.h"
#include "sx127x.h"
#include "utilities.h"
#include "config.h"

#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"

#define LORA_SPI_HOST       SPI2_HOST
#define LORA_SPI_CLOCK_HZ   (8 * 1000 * 1000)
#define LORA_SPI_MAX_LEN    256   /* 1 addr byte + max 255 data bytes */

/* ================================================================
 *  Beacon (TDMA master clock)
 * ================================================================ */

#define BEACON_BROADCAST_ID      0x00
#define BEACON_COMMAND_CODE      150
#define BEACON_NETWORK_CYCLE_S   30
#define BEACON_SLOT_DURATION_MS  300

typedef struct __attribute__((packed)) {
    uint8_t  broadcast_id;
    uint8_t  command_code;
    uint16_t network_cycle_s;
    uint16_t slot_duration_ms;
} Beacon_Packet;

/* ================================================================
 *  Pin config
 * ================================================================ */

#define PIN_SCK     CFG_WSN_GATEWAY_LORA_IO_SCK
#define PIN_MISO    CFG_WSN_GATEWAY_LORA_IO_MISO
#define PIN_MOSI    CFG_WSN_GATEWAY_LORA_IO_MOSI
#define PIN_NSS     CFG_WSN_GATEWAY_LORA_IO_NSS
#define PIN_DIO0    CFG_WSN_GATEWAY_LORA_IO_DIO0
#define PIN_NRESET  CFG_WSN_GATEWAY_LORA_IO_NRESET

/* ================================================================
 *  Private state
 * ================================================================ */

static const char *TAG      = "[RECEIVER]";
static const char *TAG_LORA = "[RECEIVER_LORA]";

static SX127x_Dev           s_dev;
static spi_device_handle_t  s_spi;
static QueueHandle_t        s_rx_queue;
static TaskHandle_t         s_recv_task;
static SemaphoreHandle_t    s_started_sem;
static int64_t              s_last_beacon_us = 0;

/* ================================================================
 *  HAL: SX127x_Dev callbacks
 * ================================================================ */

static int hal_spi_transfer(uint8_t reg, const uint8_t *tx, uint8_t *rx, size_t len)
{
    if (len + 1 > LORA_SPI_MAX_LEN) {
        return -1;
    }

    /* static: avoid 256+256 bytes on the task stack every call */
    static uint8_t tx_buf[LORA_SPI_MAX_LEN];
    static uint8_t rx_buf[LORA_SPI_MAX_LEN];

    tx_buf[0] = reg;
    if (tx) {
        memcpy(tx_buf + 1, tx, len);
    } else {
        memset(tx_buf + 1, 0, len);   /* dummy bytes to clock out on reads */
    }

    spi_transaction_t t = {
        .length    = (len + 1) * 8,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };

    /* CS is handled by the SPI driver (spics_io_num = PIN_NSS in spi_init) */
    if (spi_device_transmit(s_spi, &t) != ESP_OK) {
        return -1;
    }

    if (rx) {
        memcpy(rx, rx_buf + 1, len);
    }

    return 0;
}

static void hal_set_nreset(int8_t level)
{
    if (level < 0) {
        gpio_set_direction(PIN_NRESET, GPIO_MODE_INPUT);
    } else {
        gpio_set_direction(PIN_NRESET, GPIO_MODE_OUTPUT);
        gpio_set_level(PIN_NRESET, level);
    }
}

static void hal_delay_ms(uint32_t ms)
{
    Utils_DelayMs(ms);
}

static void hal_delay_us(uint32_t us)
{
    esp_rom_delay_us(us);
}

/* ================================================================
 *  RX callback
 * ================================================================ */

static void on_rx_done(SX127x_Dev *dev, const SX127x_Packet *packet)
{
    (void)dev;

    if (packet->length != sizeof(Receiver_Packet)) {
        Utils_LogW(TAG_LORA, "Unexpected packet size: got %u, expected %u",
                   packet->length, (unsigned)sizeof(Receiver_Packet));
        return;
    }

    Receiver_Packet item;
    memcpy(&item, packet->data, sizeof(Receiver_Packet));

    Utils_LogI(TAG_LORA, "RX: id=%u temp=%.1fC humi=%.1f%% soil=%.1f%% (rssi=%d snr=%.2f)",
               item.device_id, item.temp_c / 10.0f, item.humi_rh / 10.0f, item.soil_humi_vwc / 10.0f,
               packet->rssi, packet->snr * 0.25f);

    if (xQueueSend(s_rx_queue, &item, 0) != pdTRUE) {
        Utils_LogW(TAG_LORA, "RX queue full, dropping packet");
    }
}

/* ================================================================
 *  TX callback (beacon)
 * ================================================================ */

static void on_tx_done(SX127x_Dev *dev)
{
    Utils_LogI(TAG, "Beacon TxDone, back to RX.");
    SX127x_SetInvertIQ(dev, false);
    SX127x_StartReceive(dev, true);
}

/* ================================================================
 *  DIO0 ISR
 * ================================================================ */

static void IRAM_ATTR dio0_isr(void *arg)
{
    (void)arg;
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(s_recv_task, &woken);
    portYIELD_FROM_ISR(woken);
}

/* ================================================================
 *  Beacon helper
 * ================================================================ */

static void send_beacon(void)
{
    Beacon_Packet beacon = {
        .broadcast_id     = BEACON_BROADCAST_ID,
        .command_code     = BEACON_COMMAND_CODE,
        .network_cycle_s  = BEACON_NETWORK_CYCLE_S,
        .slot_duration_ms = BEACON_SLOT_DURATION_MS,
    };

    SX127x_Status iq = SX127x_SetInvertIQ(&s_dev, false);
    if (iq != SX127X_OK) {
        Utils_LogW(TAG, "Beacon SetInvertIQ failed: %s", SX127x_StatusStr(iq));
    }

    SX127x_Status s = SX127x_Send(&s_dev, (const uint8_t *)&beacon, sizeof(beacon));
    if (s != SX127X_OK) {
        Utils_LogW(TAG, "Beacon send failed: %s", SX127x_StatusStr(s));
        SX127x_SetInvertIQ(&s_dev, false);
        SX127x_StartReceive(&s_dev, true);
        return;
    }

    Utils_LogI(TAG, "Beacon sent, cycle=%us slot=%ums", BEACON_NETWORK_CYCLE_S, BEACON_SLOT_DURATION_MS);
}

/* ================================================================
 *  LoRa dispatch task
 * ================================================================ */

static void recv_task(void *args)
{
    (void)args;

    xSemaphoreTake(s_started_sem, portMAX_DELAY);

    s_last_beacon_us = esp_timer_get_time() - (int64_t)BEACON_NETWORK_CYCLE_S * 1000000;

    while (1) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));

        SX127x_HandleDIO0(&s_dev);

        int64_t now_us = esp_timer_get_time();
        if (now_us - s_last_beacon_us >= (int64_t)BEACON_NETWORK_CYCLE_S * 1000000) {
            s_last_beacon_us = now_us;
            send_beacon();
        }
    }
}

/* ================================================================
 *  Internal init helpers
 * ================================================================ */

static int gpio_init(void)
{
    gpio_config_t dio0_io_cfg = {
        .pin_bit_mask = (1ULL << PIN_DIO0),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type    = GPIO_INTR_POSEDGE,
    };

    if (gpio_config(&dio0_io_cfg) != ESP_OK) {
        return -1;
    }

    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return -1;
    }

    if (gpio_isr_handler_add(PIN_DIO0, dio0_isr, NULL) != ESP_OK) {
        return -1;
    }

    gpio_config_t nreset_io_cfg = {
        .pin_bit_mask = (1ULL << PIN_NRESET),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    if (gpio_config(&nreset_io_cfg) != ESP_OK) {
        return -1;
    }

    if (gpio_set_level(PIN_NRESET, 1) != ESP_OK) {
        return -1;
    }

    return 0;
}

static int spi_init(void)
{
    spi_bus_config_t bus = {
        .mosi_io_num     = PIN_MOSI,
        .miso_io_num     = PIN_MISO,
        .sclk_io_num     = PIN_SCK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = LORA_SPI_MAX_LEN,
    };

    if (spi_bus_initialize(LORA_SPI_HOST, &bus, SPI_DMA_CH_AUTO) != ESP_OK) {
        return -1;
    }

    spi_device_interface_config_t spi_dev_sx1278_cfg = {
        .clock_speed_hz = LORA_SPI_CLOCK_HZ,
        .mode           = 0,
        .spics_io_num   = PIN_NSS,
        .queue_size     = 4,
    };

    if (spi_bus_add_device(LORA_SPI_HOST, &spi_dev_sx1278_cfg, &s_spi) != ESP_OK) {
        return -1;
    }

    return 0;
}

static int lora_init(void)
{
    s_dev.spi_transfer = hal_spi_transfer;
    s_dev.set_nreset   = hal_set_nreset;
    s_dev.delay_ms     = hal_delay_ms;
    s_dev.delay_us     = hal_delay_us;
    s_dev.on_rx_done   = on_rx_done;
    s_dev.on_tx_done   = on_tx_done;

    SX127x_Config cfg = {
        .modem = SX127X_MODEM_LORA,
        .lora  = {
            .frequency_hz     = CFG_WSN_GATEWAY_LORA_FREQ_HZ,
            .bandwidth        = CFG_WSN_GATEWAY_LORA_BANDWIDTH,
            .spreading_factor = CFG_WSN_GATEWAY_LORA_SF,
            .coding_rate      = CFG_WSN_GATEWAY_LORA_CODING_RATE,
            .preamble_length  = CFG_WSN_GATEWAY_LORA_PREAMBLE_LENGTH,
            .tx_power_dbm     = CFG_WSN_GATEWAY_LORA_TX_POWER_DBM,
            .sync_word        = CFG_WSN_GATEWAY_LORA_SYNC_WORD,
            .crc_on           = CFG_WSN_GATEWAY_LORA_ENABLE_CRC,
            .implicit_header  = CFG_WSN_GATEWAY_LORA_IMPLICIT_HEADER,
        },
    };

    SX127x_Status s = SX127x_Reset(&s_dev, SX127X_RESET_MANUAL);
    if (s != SX127X_OK) {
        Utils_LogE(TAG, "Reset failed: %s", SX127x_StatusStr(s));
        return -1;
    }

    s = SX127x_Init(&s_dev, &cfg);
    if (s != SX127X_OK) {
        Utils_LogE(TAG, "Init failed: %s", SX127x_StatusStr(s));
        return -1;
    }

    Utils_LogI(TAG, "SX127x ready.");
    Utils_LogI(TAG, "Config: freq=%lu bw=%d sf=%d cr=%d preamble=%u sync=0x%02X crc=%d implicit=%d txpow=%d",
               (unsigned long)cfg.lora.frequency_hz, (int)cfg.lora.bandwidth,
               (int)cfg.lora.spreading_factor, (int)cfg.lora.coding_rate,
               cfg.lora.preamble_length, cfg.lora.sync_word,
               (int)cfg.lora.crc_on, (int)cfg.lora.implicit_header,
               (int)cfg.lora.tx_power_dbm);
    return 0;
}

/* ================================================================
 *  Public API
 * ================================================================ */

int receiver_init(void)
{
    if (gpio_init() != 0) {
        Utils_LogE(TAG, "Failed to initialize GPIO.");
        return -1;
    }

    if (spi_init() != 0) {
        Utils_LogE(TAG, "Failed to initialize SPI.");
        return -1;
    }

    /* Queue must exist before lora_init(): SX127x_Init() may trigger
     * a spurious early IRQ depending on chip reset state, and
     * on_rx_done() needs s_rx_queue ready to avoid writing to NULL. */
    s_rx_queue = xQueueCreate(8, sizeof(Receiver_Packet));
    if (s_rx_queue == NULL) {
        Utils_LogE(TAG, "Failed to create RX queue.");
        return -1;
    }

    s_started_sem = xSemaphoreCreateBinary();
    if (s_started_sem == NULL) {
        Utils_LogE(TAG, "Failed to create start semaphore.");
        return -1;
    }

    if (lora_init() != 0) {
        Utils_LogE(TAG, "Failed to initialize LoRa.");
        return -1;
    }
    Utils_LogI(TAG_LORA, "LoRa OK.");

    if (xTaskCreatePinnedToCore(recv_task, "recv_task", 4096, NULL, 5, &s_recv_task, 1) != pdPASS) {
        Utils_LogE(TAG, "Failed to create recv_task.");
        return -1;
    }

    return 0;
}

int receiver_start(void)
{
    if (s_rx_queue == NULL) {
        Utils_LogE(TAG, "Cannot start receiver: not initialized.");
        return -1;
    }

    SX127x_Status iq = SX127x_SetInvertIQ(&s_dev, false);
    if (iq != SX127X_OK) {
        Utils_LogE(TAG, "SetInvertIQ failed: %s", SX127x_StatusStr(iq));
        return -1;
    }

    SX127x_Status s = SX127x_StartReceive(&s_dev, true);
    if (s != SX127X_OK) {
        Utils_LogE(TAG, "StartReceive failed: %s", SX127x_StatusStr(s));
        return -1;
    }

    Utils_LogI(TAG, "Listening (RX continuous)...");
    xSemaphoreGive(s_started_sem);

    return 0;
}

int receiver_stop(void)
{
    if (s_rx_queue == NULL) {
        Utils_LogE(TAG, "Cannot stop receiver: not initialized.");
        return -1;
    }

    SX127x_Status s = SX127x_StopReceive(&s_dev);
    if (s != SX127X_OK) {
        Utils_LogE(TAG, "StopReceive failed: %s", SX127x_StatusStr(s));
        return -1;
    }

    return 0;
}

int receiver_read(Receiver_Packet *out, uint32_t timeout_ms)
{
    if (!out) {
        return -1;
    }

    if (s_rx_queue == NULL) {
        Utils_LogE(TAG, "s_rx_queue is NULL. Receiver not initialized properly.");
        return -1;
    }

    TickType_t ticks = (timeout_ms == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);

    if (xQueueReceive(s_rx_queue, out, ticks) != pdTRUE) {
        Utils_LogW(TAG, "RX queue empty.");
        return -1;  /* timeout */
    }

    return 0;
}

int Receiver_ToJson(const Receiver_Packet *pkt, char *out_buf, size_t buf_len)
{
    if (!pkt || !out_buf) {
        return -1;
    }

    /* Fields are x0.1 fixed-point; divide back to float for output. */
    int n = snprintf(out_buf, buf_len,
                      "{\"device_id\":%u,\"temp\":%.1f,\"humi\":%.1f,\"soil_humi\":%.1f}",
                      pkt->device_id,
                      pkt->temp_c / 10.0f,
                      pkt->humi_rh / 10.0f,
                      pkt->soil_humi_vwc / 10.0f);

    if (n < 0 || (size_t)n >= buf_len) {
        return -1;   /* truncated or snprintf error */
    }

    return n;
}