#include "app.h"
#include "network.h"
#include "utilities.h"
#include "hardware.h"
#include "receiver.h"

#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "[APP]";

static volatile bool s_config_mode = false;

static void forward_task(void *args){
    (void)args;

    Receiver_Packet pkt;
    char json_buf[96];

    while (1) {
        if (receiver_read(&pkt, 0) != 0) {
            continue;
        }

        int len = Receiver_ToJson(&pkt, json_buf, sizeof(json_buf));
        if (len < 0) {
            Utils_LogW(TAG, "Failed to build JSON for id=%u", pkt.device_id);
            continue;
        }

        if (!network_wait_ready(10000)) {
            Utils_LogW(TAG, "Network not ready, dropping packet id=%u", pkt.device_id);
            continue;
        }

        Utils_LogI(TAG, "Forwarding: %s", json_buf);
        network_send(json_buf);
    }
}

void short_press(void){
    s_config_mode = true;
    hardware_led_blink(true);
    network_config();
    hardware_led_blink(false);
    s_config_mode = false;
}

void long_press(void){
    hardware_shutdown();
}

void connected_wifi(void){
    hardware_led(true);
}

void disconnected_wifi(void){
    if (s_config_mode) {
        return;
    }
    hardware_led(false);
}

int app_init(void){
    hardware_set_btn_long_cb(long_press);
    hardware_set_btn_short_cb(short_press);
    if (hardware_init() != 0) {
        Utils_LogE(TAG, "hardware_init failed.");
        return -1;
    }

    network_set_wifi_connected_cb(connected_wifi);
    network_set_wifi_disconnected_cb(disconnected_wifi);
    network_init();

    Utils_DelayMs(5000);

    if (receiver_init() != 0) {
        Utils_LogE(TAG, "receiver_init failed.");
        return -1;
    }

    if (receiver_start() != 0) {
        Utils_LogE(TAG, "receiver_start failed.");
        return -1;
    }

    if (xTaskCreatePinnedToCore(forward_task, "forward_task", 4096, NULL, 4, NULL, 1) != pdPASS) {
        Utils_LogE(TAG, "Failed to create forward_task.");
        return -1;
    }

    return 0;
}

void app_run(void){
}