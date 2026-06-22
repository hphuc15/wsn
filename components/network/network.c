#include "network.h"
#include "config.h"
#include "utilities.h"

#include "wifi.h"
#include "transport.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

static const char *TAG = "[NETWORK]";

#define NETWORK_BIT_TRANSPORT_READY BIT0

typedef void (*wifi_cb_t)(void);

static wifi_cb_t          s_app_connected_cb = NULL;
static EventGroupHandle_t s_net_eg           = NULL;

static void on_wifi_connected(void) {
    if (s_app_connected_cb) {
        s_app_connected_cb();
    }

    xEventGroupClearBits(s_net_eg, NETWORK_BIT_TRANSPORT_READY);

    transport_deinit();
    if (transport_load_config() != 0) {
        Utils_LogE(TAG, "Failed to load transport config.");
        return;
    }
    if (transport_init() != 0) {
        Utils_LogE(TAG, "Failed to initialize transport.");
        return;
    }
    Utils_LogI(TAG, "Transport OK.");

    xEventGroupSetBits(s_net_eg, NETWORK_BIT_TRANSPORT_READY);
}

int network_send(const char *data) {
    return transport_publish(data);
}

int network_config(void) {
    xEventGroupClearBits(s_net_eg, NETWORK_BIT_TRANSPORT_READY);

    transport_deinit();

    if (wifi_config() != 0) {
        return -1;
    }
    Utils_DelayMs(1000);

    if (transport_load_config() != 0) {
        return -1;
    }

    int ret = transport_init();
    if (ret == 0) {
        xEventGroupSetBits(s_net_eg, NETWORK_BIT_TRANSPORT_READY);
    }
    return ret;
}

void network_set_wifi_connected_cb(void *cb) {
    s_app_connected_cb = (wifi_cb_t)cb;
}

void network_set_wifi_disconnected_cb(void *cb) {
    wifi_set_disconnected_cb(cb);
}

/**
 * @brief Block until the transport (HTTP URL / MQTT client) is ready to publish.
 * @param timeout_ms 0 = wait forever.
 * @return true if ready, false on timeout.
 */
bool network_wait_ready(uint32_t timeout_ms) {
    if (!s_net_eg) {
        return false;
    }

    TickType_t ticks = (timeout_ms == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    EventBits_t bits = xEventGroupWaitBits(s_net_eg, NETWORK_BIT_TRANSPORT_READY, pdFALSE, pdTRUE, ticks);
    return (bits & NETWORK_BIT_TRANSPORT_READY) != 0;
}

static void network_init_task(void *args) {
    if (wifi_init() != 0) {
        Utils_LogE(TAG, "Failed to initialize WiFi.");
        vTaskDelete(NULL);
        return;
    }

    wifi_set_connected_cb(on_wifi_connected);

    if (wifi_start() != 0) {
        Utils_LogE(TAG, "Failed to start WiFi.");
    }

    vTaskDelete(NULL);
}

int network_init(void) {
    s_net_eg = xEventGroupCreate();
    if (!s_net_eg) {
        Utils_LogE(TAG, "Failed to create event group.");
        return -1;
    }

    BaseType_t ret = xTaskCreatePinnedToCore(network_init_task, "network_init_task", 6144, NULL, 5, NULL, 1);
    return (ret == pdPASS) ? 0 : -1;
}

void network_deinit(void) {
    transport_deinit();
    if (s_net_eg) {
        vEventGroupDelete(s_net_eg);
        s_net_eg = NULL;
    }
}