#include "network.h"
#include "http_transport.h"
#include "mqtt_transport.h"
#include "wifi.h"
#include "hardware_config.h"
/* ESP-IDF libs */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
/* Standard libs */
#include <string.h>

/* LOG TAG */

static const char *TAG_WIFI         = "[NETWORK][WIFI]";
static const char *TAG_TRANSPORT    = "[NETWORK][TRANSPORT]";

/* STATE */
static network_proto_t s_proto = NETWORK_PROTO_HTTP;
static bool s_transport_up = false;

/* TRANSPORT VTABLE*/
typedef struct {
    esp_err_t (*init)(void);
    esp_err_t (*publish)(const char *payload);
    bool (*is_ready)(void);
    esp_err_t (*stop)(void);
} transport_ops_t;

static const transport_ops_t s_transports[] = {
    [NETWORK_PROTO_HTTP] = {
        .init = http_transport_init,
        .publish = http_transport_publish,
        .is_ready = http_transport_is_ready,
        .stop = http_transport_stop,
    },
    [NETWORK_PROTO_MQTT] = {
        .init = mqtt_transport_init,
        .publish = mqtt_transport_publish,
        .is_ready = mqtt_transport_is_ready,
        .stop = mqtt_transport_stop,
    },
};

static const transport_ops_t *s_active = NULL;

/* WIFI CALLBACKs */

/**
 * @brief WiFi connected callback
 * Use to set network application
 */
static void network_wifi_connected_cb(void) {
    ESP_LOGI(TAG_WIFI, "Connected");
    hw_led_set(HW_LED_ON);

    if (s_active && !s_transport_up) {
        esp_err_t err = s_active->init();
        if (err == ESP_OK) {
            s_transport_up = true;
            ESP_LOGI(TAG_TRANSPORT, "Transport up");
        } else {
            ESP_LOGE(TAG_TRANSPORT, "Init failed: %s", esp_err_to_name(err));
        }
    }
}

/**
 * @brief WiFi disconnected callback
 * Use to set network application
 */
static void network_wifi_disconnected_cb(void) {
    ESP_LOGW(TAG_WIFI, "Disconnected");
    hw_led_set(HW_LED_OFF);
    s_transport_up = false;
    if (s_active){
        s_active->stop();
    }
}

/* PUBLIC APIs */

network_err_t network_init(network_proto_t proto) {
    if (proto >= sizeof(s_transports) / sizeof(s_transports[0])) {
        return NETWORK_ERR_INVALID_ARG;
    }
    s_proto = proto;
    s_active = &s_transports[proto];

    ESP_LOGI(TAG_TRANSPORT, "Proto: %s", proto == NETWORK_PROTO_MQTT ? "MQTTS" : "HTTPS");

    /* WiFi Setup */
    wifi_set_connected_cb(network_wifi_connected_cb);
    wifi_set_disconnected_cb(network_wifi_disconnected_cb);
    wifi_init();
    wifi_connect();

    return NETWORK_OK;
}

network_err_t network_publish(const char *payload) {
    if (!s_active || !s_transport_up) {
        ESP_LOGW(TAG_TRANSPORT, "Not ready");
        return NETWORK_ERR_INVALID_STATE;
    }

    esp_err_t err = ESP_ERR_INVALID_STATE;
    if (s_transport_up && s_active) {
        ESP_LOGD(TAG_TRANSPORT, "Publishing %d bytes", strlen(payload));
        err = s_active->publish(payload);
    }

    switch (err) {
        case ESP_OK:
            ESP_LOGI(TAG_TRANSPORT, "Published OK");
            return NETWORK_OK;
        case ESP_ERR_INVALID_ARG:
            ESP_LOGE(TAG_TRANSPORT, "Invalid arg");
            return NETWORK_ERR_INVALID_ARG;
        case ESP_ERR_INVALID_STATE:
            ESP_LOGW(TAG_TRANSPORT, "Invalid state");
            return NETWORK_ERR_INVALID_STATE;
        default:
            ESP_LOGE(TAG_TRANSPORT, "Transport error: %s", esp_err_to_name(err));
            return NETWORK_ERR_TRANSPORT;
    }
}

bool network_is_ready(void) {
    return s_transport_up && s_active && s_active->is_ready();
}

network_err_t network_stop(void) {
    if (!s_active) {
        return NETWORK_OK;
    }
    
    s_transport_up = false;
    esp_err_t err = s_active->stop();
    s_active = NULL;

    wifi_stop();

    switch (err) {
        case ESP_OK:                return NETWORK_OK;
        case ESP_ERR_INVALID_ARG:   return NETWORK_ERR_INVALID_ARG;
        case ESP_ERR_INVALID_STATE: return NETWORK_ERR_INVALID_STATE;
        default:                    return NETWORK_ERR_TRANSPORT;
    }
}

network_err_t network_reconfigure(void) {
    s_transport_up = false;
    if (s_active) s_active->stop();

    wifi_stop();
    hw_led_set(HW_LED_BLINK);

    wifi_config();
    return NETWORK_OK;
}