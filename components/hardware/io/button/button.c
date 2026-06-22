#include "button.h"
#include "config.h"
#include "utilities.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "driver/gpio.h"

#define BTN_LONG_PRESS_MS 3000
#define BTN_SHORT_DEBOUNCE_MS 50

static const char *TAG_BUTTON = "[BUTTON]";

static void (*s_short_cb)(void) = NULL;
static void (*s_long_cb)(void) = NULL;

static void _btn_on_short_press(void)
{
    Utils_LogI(TAG_BUTTON, "Short press");
    if (s_short_cb)
    {
        s_short_cb();
    }
}

static void _btn_on_long_press(void)
{
    Utils_LogI(TAG_BUTTON, "Long press");
    if (s_long_cb)
    {
        s_long_cb();
    }
}

static void _short_press_task(void *arg)
{
    _btn_on_short_press();
    vTaskDelete(NULL);
}

static void btn_task(void *arg)
{
    while (1)
    {
        if (gpio_get_level(CFG_WSN_GATEWAY_GPIO_WAKE_BTN) == 0)
        {
            int64_t start = esp_timer_get_time();
            bool long_triggered = false;

            while (gpio_get_level(CFG_WSN_GATEWAY_GPIO_WAKE_BTN) == 0)
            {
                if ((esp_timer_get_time() - start) >= (BTN_LONG_PRESS_MS * 1000LL))
                {
                    long_triggered = true;
                    break;
                }
                Utils_DelayMs(50);
            }

            if (long_triggered)
            {
                _btn_on_long_press();
            }
            else if ((esp_timer_get_time() - start) >= (BTN_SHORT_DEBOUNCE_MS * 1000LL))
            {
                xTaskCreate(_short_press_task, "btn_short", 4096, NULL, 3, NULL);
            }

            while (gpio_get_level(CFG_WSN_GATEWAY_GPIO_WAKE_BTN) == 0)
            {
                Utils_DelayMs(20);
            }
            Utils_DelayMs(BTN_SHORT_DEBOUNCE_MS);
        }
        Utils_DelayMs(20);
    }
}

void btn_set_short_press_cb(void (*cb)(void))
{
    s_short_cb = cb;
}

void btn_set_long_press_cb(void (*cb)(void))
{
    s_long_cb = cb;
}

int btn_init(void)
{
    xTaskCreatePinnedToCore(btn_task, "btn_task", 4096, NULL, 4, NULL, 1);
    return 0;
}