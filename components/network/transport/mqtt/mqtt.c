#include "mqtt.h"
#include "utilities.h"

#include "mqtt_client.h"
#include "esp_crt_bundle.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include <string.h>

#define TAG "[MQTT]"

#define MQTT_CONNECT_TIMEOUT_MS 10000
#define MQTT_TOPIC_MAX_LEN 128

#define MQTT_CONNECTED_BIT BIT0
#define MQTT_FAIL_BIT BIT1

/* Module state */

static esp_mqtt_client_handle_t s_client = NULL;
static EventGroupHandle_t s_mqtt_eg = NULL;
static char s_topic[MQTT_TOPIC_MAX_LEN];

/* Event handler */

static void mqtt_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        Utils_LogI(TAG, "Connected to broker.");
        xEventGroupSetBits(s_mqtt_eg, MQTT_CONNECTED_BIT);
        break;
    case MQTT_EVENT_DISCONNECTED:
        Utils_LogW(TAG, "Disconnected from broker.");
        xEventGroupClearBits(s_mqtt_eg, MQTT_CONNECTED_BIT);
        break;
    case MQTT_EVENT_ERROR:
        Utils_LogE(TAG, "MQTT error event.");
        xEventGroupSetBits(s_mqtt_eg, MQTT_FAIL_BIT);
        break;
    default:
        break;
    }
}

/* Public API */

int mqtt_init(const char *host, uint32_t port, const char *topic, const char *username, const char *password, bool tls)
{
    if (!host || !topic)
    {
        Utils_LogE(TAG, "Failed to initialize MQTT.");
        return -1;
    }

    strncpy(s_topic, topic, sizeof(s_topic) - 1);
    s_topic[sizeof(s_topic) - 1] = '\0';

    s_mqtt_eg = xEventGroupCreate();
    if (!s_mqtt_eg)
    {
        Utils_LogE(TAG, "Failed to create event group.");
        return -1;
    }

    esp_mqtt_client_config_t cfg = {
        .broker = {
            .address = {
                .hostname = host,
                .port = port,
                .transport = tls ? MQTT_TRANSPORT_OVER_SSL
                                 : MQTT_TRANSPORT_OVER_TCP,
            },
            .verification.crt_bundle_attach = tls ? esp_crt_bundle_attach : NULL,
        },
        .credentials = {
            .username = (username && *username) ? username : NULL,
            .authentication.password = (password && *password) ? password : NULL,
        },
    };

    s_client = esp_mqtt_client_init(&cfg);
    if (!s_client)
    {
        Utils_LogE(TAG, "Failed to initialize MQTT client.");
        return -1;
    }

    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    if (esp_mqtt_client_start(s_client) != ESP_OK)
    {
        Utils_LogE(TAG, "Failed to start MQTT client.");
        esp_mqtt_client_destroy(s_client);
        s_client = NULL;
        return -1;
    }

    EventBits_t bits = xEventGroupWaitBits(s_mqtt_eg, MQTT_CONNECTED_BIT | MQTT_FAIL_BIT, pdFALSE, pdFALSE, pdMS_TO_TICKS(MQTT_CONNECT_TIMEOUT_MS));

    if (!(bits & MQTT_CONNECTED_BIT) || (bits & MQTT_FAIL_BIT))
    {
        Utils_LogE(TAG, "Failed to connect to broker (timeout or error).");
        return -1;
    }

    Utils_LogI(TAG, "Topic: %s", s_topic);
    return 0;
}

int mqtt_publish(const char *data)
{
    if (!s_client || !data)
    {
        Utils_LogE(TAG, "Failed to publish data.");
        return -1;
    }

    if (!(xEventGroupGetBits(s_mqtt_eg) & MQTT_CONNECTED_BIT))
    {
        Utils_LogW(TAG, "Not connected - skipping publish.");
        return -1;
    }

    int msg_id = esp_mqtt_client_publish(s_client, s_topic, data, strlen(data), 1, 0);
    if (msg_id < 0)
    {
        Utils_LogE(TAG, "Failed to publish data.");
        return -1;
    }

    Utils_LogI(TAG, "Published msg_id=%d.", msg_id);
    return 0;
}

void mqtt_deinit(void)
{
    if (s_client)
    {
        esp_mqtt_client_stop(s_client);
        esp_mqtt_client_destroy(s_client);
        s_client = NULL;
    }
    if (s_mqtt_eg)
    {
        vEventGroupDelete(s_mqtt_eg);
        s_mqtt_eg = NULL;
    }
}