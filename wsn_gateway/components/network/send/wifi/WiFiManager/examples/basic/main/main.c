#include <stdio.h>
#include "WiFiManager.h"

#include "driver/gpio.h"

static WiFiManager_t wm = {0};

static void LedOn(void)
{
    gpio_config_t gpio_2 = {
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = (1ULL << GPIO_NUM_2),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE};
    gpio_config(&gpio_2);
    gpio_set_level(GPIO_NUM_2, 1);
}

void app_main(void)
{
    esp_netif_init();

    wm.config.ap = WM_AP_CONFIG_DEFAULT();
    WiFiManager_Init(&wm);
    /*
        wmp_init(&wm.page);
        WiFiManagerPage_AddParam(&wm.page, "mqtt_host", "MQTT Broker", "e.g. broker.hivemq.com", "", "text", false);
        WiFiManagerPage_AddParam(&wm.page, "mqtt_port", "MQTT Port", "1883", "1883", "number", false);
        WiFiManagerPage_AddParam(&wm.page, "api_token", "API Token", "your-secret-token", "", "password", true);
     */
    wm.ConnectedAP_Cb = LedOn;

    WiFiManager_ConfigViaAP(&wm);
    while (1)
    {
        ESP_LOGI("[Test]", "Hello ...."); /*
         ESP_LOGI("[Test get params]", "mqtt_host: %s", WiFiManagerPage_GetParam(&wm.page, "mqtt_host"));
         ESP_LOGI("[Test get params]", "mqtt_port: %s", WiFiManagerPage_GetParam(&wm.page, "mqtt_port"));
         ESP_LOGI("[Test get params]", "api_token: %s", WiFiManagerPage_GetParam(&wm.page, "api_token"));
  */
        wifi_mode_t mode;
        esp_wifi_get_mode(&mode);
        ESP_LOGW("[WIFI MODE]", "%d", mode);
        ESP_LOGI("[Test WiFi Cre]", "ssid: %s", wm.config.sta.ssid);
        ESP_LOGI("[Test WiFi Cre]", "password: %s", wm.config.sta.password);
        ESP_LOGI("[Test WiFi Cre]", "Authode: %d", wm.config.sta.threshold.authmode);

        vTaskDelay(pdMS_TO_TICKS(40000));
    }
}
