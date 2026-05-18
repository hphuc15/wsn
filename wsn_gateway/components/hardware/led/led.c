#include "led.h"
#include "status.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_STATUS_IO                   GPIO_NUM_2
#define LED_STATUS_BLINK_INTERVAL_MS    500

void led_init(void){
    gpio_config_t led_io = {
        .pin_bit_mask   = (1ULL << LED_STATUS_IO),
        .mode           = GPIO_MODE_OUTPUT,
        .intr_type      = GPIO_INTR_DISABLE,
        .pull_up_en     = GPIO_PULLUP_DISABLE,
        .pull_down_en   = GPIO_PULLDOWN_DISABLE
    };
    gpio_config(&led_io);
}

void led_on(void){
    gpio_set_level(LED_STATUS_IO, 1);
}

void led_off(void){
    gpio_set_level(LED_STATUS_IO, 0);
}

void led_togle(void){
    gpio_set_level(LED_STATUS_IO, !(gpio_get_level(LED_STATUS_IO)));
}

void led_blink(void){
    led_togle();
}

void led_task(void *args){
    while(1){
        if(wsn_gateway_status == WSN_GATEWAY_ONLINE){
            led_on();
            vTaskDelay(pdMS_TO_TICKS(100));
        } else if (wsn_gateway_status == WSN_GATEWAY_OFFLINE){
            led_off();
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            led_blink();
            vTaskDelay(pdMS_TO_TICKS(LED_STATUS_BLINK_INTERVAL_MS));
        }
    }
}