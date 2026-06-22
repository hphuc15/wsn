#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TRANSPORT_HTTP = 0,
    TRANSPORT_MQTT,
} transport_protocol_e;

typedef struct {
    char     host[64];
    uint32_t port;
    char     path[128];
    bool     tls;           /**< true = HTTPS */
} http_config_t;

typedef struct {
    char     host[64];
    uint32_t port;
    char     topic[128];
    char     username[64];  /**< device token / username */
    char     password[64];  /**< leave empty if token-based */
    bool     tls;           /**< true = MQTT over TLS */
} mqtt_config_t;

typedef struct {
    transport_protocol_e protocol;
    union {
        http_config_t http;
        mqtt_config_t mqtt;
    };
} transport_config_t;

/**
 * @brief Load transport config from NVS.
 *        Falls back to default (credentials.h) if key missing.
 *        Must be called before transport_init().
 * @return 0 on success, -1 on fatal error.
 */
int transport_load_config(void);

/**
 * @brief Initialize transport layer using the loaded config.
 *        Must be called after transport_load_config().
 * @return 0 on success, -1 on fail.
 */
int transport_init(void);

/**
 * @brief Publish data using the active transport protocol.
 * @param data JSON string payload.
 * @return 0 on success, -1 on fail.
 */
int transport_publish(const char *data);

/** @brief Deinitialize transport layer and free resources. */
void transport_deinit(void);

#endif /* TRANSPORT_H */