#include "power.h"
#include "config.h"
#include "utilities.h"

#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_timer.h"

#include <stdio.h>
#include <stdbool.h>

/* Config */

#define POWER_GPIO_WAKE_BTN         CFG_WSN_GATEWAY_GPIO_WAKE_BTN
#define POWER_GPIO_STATUS_LED       CFG_WSN_GATEWAY_GPIO_STATUS_LED
#define POWER_LED_BLINK_INTERVAL_MS 500
#define POWER_RAIL_STABLE_MS        10

/* Log tags */

static const char *TAG_SLEEP = "[POWER][SLEEP]";
static const char *TAG_GPIO = "[POWER][GPIO]";

/* Static state */

static TaskHandle_t s_led_blink_task_handle = NULL;

/* ================================================================== */
/* GPIO */
/* ================================================================== */

static int _gpio_init(void)
{

    /* Status LED gpio */
    gpio_reset_pin(POWER_GPIO_STATUS_LED);
    gpio_config_t io_status_led = {
        .pin_bit_mask = 1ULL << POWER_GPIO_STATUS_LED,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if (gpio_config(&io_status_led) != ESP_OK)
    {
        Utils_LogE(TAG_GPIO, "LED GPIO config failed");
        return -1;
    }

    gpio_set_level(POWER_GPIO_STATUS_LED, 0);

    /* Wake button gpio */
    gpio_reset_pin(POWER_GPIO_WAKE_BTN);
    gpio_config_t io_wake_btn = {
        .pin_bit_mask = 1ULL << POWER_GPIO_WAKE_BTN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if (gpio_config(&io_wake_btn) != ESP_OK)
    {
        Utils_LogE(TAG_GPIO, "Button GPIO config failed");
        return -1;
    }

    return 0;
}

/* ================================================================== */
/* Sleep */
/* ================================================================== */

int power_sleep(void)
{
    /* Wait for the wake button to be released before configuring wakeup */
    while (rtc_gpio_get_level(POWER_GPIO_WAKE_BTN) == 0)
    {
        Utils_DelayMs(100);
    }
    Utils_DelayMs(100);

    if (rtc_gpio_init(POWER_GPIO_WAKE_BTN) != ESP_OK)
        return -1;
    if (rtc_gpio_pullup_en(POWER_GPIO_WAKE_BTN) != ESP_OK)
        return -1;
    if (rtc_gpio_pulldown_dis(POWER_GPIO_WAKE_BTN) != ESP_OK)
        return -1;
    if (esp_sleep_enable_ext0_wakeup(POWER_GPIO_WAKE_BTN, 0) != ESP_OK)
        return -1;

    Utils_LogI(TAG_SLEEP, "Entering deep sleep...");
    esp_deep_sleep_start();

    return 0;
}

/* ================================================================== */
/* LED */
/* ================================================================== */

static int _power_led(bool on)
{
    return gpio_set_level(POWER_GPIO_STATUS_LED, on) == ESP_OK ? 0 : -1;
}

static void _led_blink_task(void *arg)
{
    while (1)
    {
        _power_led(true);
        Utils_DelayMs(POWER_LED_BLINK_INTERVAL_MS);
        _power_led(false);
        Utils_DelayMs(POWER_LED_BLINK_INTERVAL_MS);
    }
}

int power_led_blink(bool on)
{
    if (on)
    {
        if (s_led_blink_task_handle == NULL)
        {
            BaseType_t ret = xTaskCreatePinnedToCore(_led_blink_task, "led_blink", 1024, NULL, 4, &s_led_blink_task_handle, 1);
            if (ret != pdPASS)
            {
                return -1;
            }
        }
    }
    else
    {
        if (s_led_blink_task_handle != NULL)
        {
            vTaskDelete(s_led_blink_task_handle);
            s_led_blink_task_handle = NULL;
            _power_led(false);
        }
    }
    return 0;
}

/* ================================================================== */
/* Init */
/* ================================================================== */

int power_init(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_EXT0)
    {
        Utils_LogI(TAG_SLEEP, "Wake up from deep sleep by button.");
    }
    else
    {
        Utils_LogI(TAG_SLEEP, "Normal power on.");
    }

    if (_gpio_init() != 0)
    {
        return -1;
    }

    /* Block wakeup if battery is too low, before enabling the sensor rail */

    if (_power_led(true) != 0)
    {
        return -1;
    }

    return 0;
}