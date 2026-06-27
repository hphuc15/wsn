#include "app.h"
#include "network.h"
#include "utilities.h"
#include "hardware.h"
#include "receiver.h"

#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static volatile bool s_config_mode = false;

/* ================================================================
 *  Forwarder task: core 1, blocks on the LoRa RX queue, sends
 *  each packet as JSON the instant it arrives. No polling.
 * ================================================================ */

static void forward_task(void *args)
{
    (void)args;

    Receiver_Packet pkt;
    char json_buf[96];

    while (1)
    {
        /* Blocks forever until recv_task pushes a packet. */
        if (receiver_read(&pkt, 0) != 0)
        {
            continue; /* shouldn't happen with timeout_ms=0, but stay safe */
        }

        int len = Receiver_ToJson(&pkt, json_buf, sizeof(json_buf));
        if (len < 0)
        {
            Utils_LogW("[APP]", "Failed to build JSON for id=%u", pkt.device_id);
            continue;
        }

        if (!network_wait_ready(10000))
        {
            Utils_LogW("[APP]", "Network not ready, dropping packet id=%u", pkt.device_id);
            continue;
        }

        Utils_LogI("[APP]", "Forwarding: %s", json_buf);
        network_send(json_buf);
    }
}

/* ================================================================
 *  Existing callbacks
 * ================================================================ */

void short_press(void) {
    s_config_mode = true;
    hardware_led_blink(true);
    network_config();
    hardware_led_blink(false);
    s_config_mode = false;
}

void long_press(void) {
    hardware_shutdown();
}

void connected_wifi(void)
{
    hardware_led(true);
}

void disconnected_wifi(void) {
    if(s_config_mode){
        return ;
    }
    hardware_led(false);
}

int app_init(void)
{

    hardware_set_btn_long_cb(long_press);
    hardware_set_btn_short_cb(short_press);
    if (hardware_init() != 0)
    {
        Utils_LogE("[APP]", "hardware_init failed");
        return -1;
    }

    network_set_wifi_connected_cb(connected_wifi);
    network_set_wifi_disconnected_cb(disconnected_wifi);
    network_init();

    Utils_DelayMs(5000);
    
    if (receiver_init() != 0)
    {
        Utils_LogE("[APP]", "receiver_init failed");
        return -1;
    }

    if (receiver_start() != 0)
    {
        Utils_LogE("[APP]", "receiver_start failed");
        return -1;
    }

    /* Pinned to core 1, same side as recv_task - keeps node-side work
     * (LoRa RX + forwarding) off core 0, which handles WiFi/HTTP/MQTT. */
    if (xTaskCreatePinnedToCore(forward_task, "forward_task", 4096, NULL, 4, NULL, 1) != pdPASS)
    {
        Utils_LogE("[APP]", "Failed to create forward_task");
        return -1;
    }

    return 0;
}

void app_run(void)
{
    /* Forwarding is now event-driven via forward_task. Keep this
     * function for anything else that still needs periodic polling
     * (e.g. local sensors), or leave it empty. */
}