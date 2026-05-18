// network.h
#ifndef NETWORK_H
#define NETWORK_H

#include <stdbool.h>

#define HTTP_PAYLOAD_SIZE 256

typedef enum {
    NETWORK_OK = 0,
    NETWORK_ERR_INVALID_ARG,
    NETWORK_ERR_INVALID_STATE,
    NETWORK_ERR_WIFI,
    NETWORK_ERR_TRANSPORT,
} network_err_t;

typedef enum {
    NETWORK_PROTO_HTTP = 0,
    NETWORK_PROTO_MQTT,
} network_proto_t;

network_err_t network_init(network_proto_t proto);
network_err_t network_publish(const char *payload);
bool          network_is_ready(void);
network_err_t network_stop(void);
network_err_t network_reconfigure(void);

#endif /* NETWORK_H */