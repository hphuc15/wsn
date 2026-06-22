#include "transport.h"
#include "http/http.h"
#include "mqtt/mqtt.h"
#include "config.h"
#include "utilities.h"
#include <string.h>
#include <stdlib.h>

#define NETWORK_DEFAULT_PROTOCOL    TRANSPORT_HTTP
#define NETWORK_DEFAULT_HOST        CRE_NETWORK_DEFAULT_HOST
#define NETWORK_DEFAULT_PORT        CRE_NETWORK_DEFAULT_PORT
#define NETWORK_DEFAULT_PATH        CRE_NETWORK_DEFAULT_PATH
#define NETWORK_DEFAULT_TLS         CRE_NETWORK_DEFAULT_TLS

static const char *TAG = "[NETWORK][TRANSPORT]";

static const transport_config_t default_config = {
    .protocol = NETWORK_DEFAULT_PROTOCOL,
    .http = {
        .host = NETWORK_DEFAULT_HOST,
        .port = NETWORK_DEFAULT_PORT,
        .path = NETWORK_DEFAULT_PATH,
        .tls = NETWORK_DEFAULT_TLS,
    }};
static transport_config_t config = {0};

int transport_load_config(void)
{
    if (Utils_NVS_Init() != UTILS_OK)
    {
        Utils_LogE(TAG, "Failed to initialize NVS. Using default server config.");
        config = default_config;
        return 0;
    }

    char protocol[8] = {0};
    if (Utils_NVS_ReadString(CRE_NVS_NAMESPACE_SERVER, CRE_NVS_KEY_PROTOCOL, protocol, sizeof(protocol)) != UTILS_OK)
    {
        config = default_config;
        return 0;
    }

    int32_t port_val = 0;

    if (strcmp(protocol, "http") == 0 || strcmp(protocol, "https") == 0)
    {
        config.protocol = TRANSPORT_HTTP;
        config.http.tls = (strcmp(protocol, "https") == 0);

        if (Utils_NVS_ReadString(CRE_NVS_NAMESPACE_SERVER, CRE_NVS_KEY_SERVERHOST, config.http.host, sizeof(config.http.host)) != UTILS_OK)
        {
            strncpy(config.http.host, default_config.http.host, sizeof(config.http.host));
        }
        if (Utils_NVS_ReadString(CRE_NVS_NAMESPACE_SERVER, CRE_NVS_KEY_SERVERPATH, config.http.path, sizeof(config.http.path)) != UTILS_OK)
        {
            strncpy(config.http.path, default_config.http.path, sizeof(config.http.path));
        }
        if (Utils_NVS_ReadInt(CRE_NVS_NAMESPACE_SERVER, CRE_NVS_KEY_SERVERPORT, &port_val) == UTILS_OK)
        {
            config.http.port = (uint32_t)port_val;
        }
        else
        {
            config.http.port = default_config.http.port;
        }
    }
    else if (strcmp(protocol, "mqtt") == 0 || strcmp(protocol, "mqtts") == 0)
    {
        config.protocol = TRANSPORT_MQTT;
        config.mqtt.tls = (strcmp(protocol, "mqtts") == 0);

        if (Utils_NVS_ReadString(CRE_NVS_NAMESPACE_SERVER, CRE_NVS_KEY_SERVERHOST, config.mqtt.host, sizeof(config.mqtt.host)) != UTILS_OK)
        {
            strncpy(config.mqtt.host, default_config.http.host, sizeof(config.mqtt.host));
        }
        if (Utils_NVS_ReadString(CRE_NVS_NAMESPACE_SERVER, CRE_NVS_KEY_SERVERPATH, config.mqtt.topic, sizeof(config.mqtt.topic)) != UTILS_OK)
        {
            strncpy(config.mqtt.topic, default_config.http.path, sizeof(config.mqtt.topic));
        }
        if (Utils_NVS_ReadInt(CRE_NVS_NAMESPACE_SERVER, CRE_NVS_KEY_SERVERPORT, &port_val) == UTILS_OK)
        {
            config.mqtt.port = (uint32_t)port_val;
        }
        else
        {
            config.mqtt.port = default_config.http.port;
        }
        if (Utils_NVS_ReadString(CRE_NVS_NAMESPACE_SERVER, CRE_NVS_KEY_SERVERAUTH, config.mqtt.username, sizeof(config.mqtt.username)) != UTILS_OK)
        {
            config.mqtt.username[0] = '\0';
        }
        config.mqtt.password[0] = '\0';
    }
    else
    {
        Utils_LogE(TAG, "Unknown protocol: %s", protocol);
        config = default_config;
        return 0;
    }

    return 0;
}

int transport_init(void)
{
    switch (config.protocol)
    {
    case TRANSPORT_HTTP:
        return http_init(config.http.host, config.http.port, config.http.path, config.http.tls);
    case TRANSPORT_MQTT:
        Utils_LogI(TAG,
                   "MQTT host=%s port=%" PRIu32 " topic=%s tls=%d user=%s",
                   config.mqtt.host,
                   config.mqtt.port,
                   config.mqtt.topic,
                   config.mqtt.tls,
                   config.mqtt.username);
        return mqtt_init(config.mqtt.host, config.mqtt.port, config.mqtt.topic, config.mqtt.username, config.mqtt.password, config.mqtt.tls);
    default:
        Utils_LogE(TAG, "Unknown transport protocol.");
        return -1;
    }
}

int transport_publish(const char *data)
{
    switch (config.protocol)
    {
    case TRANSPORT_HTTP:
        return http_publish(data);
    case TRANSPORT_MQTT:
        return mqtt_publish(data);
    default:
        Utils_LogE(TAG, "Unknown transport protocol.");
        return -1;
    }
}

void transport_deinit(void)
{
    if (config.protocol == TRANSPORT_MQTT)
    {
        mqtt_deinit();
    }
}