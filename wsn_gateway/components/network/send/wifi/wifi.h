#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>
#include <stdint.h>

typedef void (*wifi_cb_t)(void);

/* Initialize WiFi */
void wifi_init(void);
/* Start WiFi */
void wifi_connect(void);
/* Stop WiFi */
void wifi_stop(void);
/* Config WiFi via portal */
void wifi_config(void);
/* Set extenal WiFi connected callback */
void wifi_set_connected_cb(wifi_cb_t cb);
/* Set extenal WiFi disconnected callback */
void wifi_set_disconnected_cb(wifi_cb_t cb);

bool wifi_is_ready(void);

const char *wifi_get_host(void);
uint32_t wifi_get_port(void);
const char *wifi_get_token(void);

#endif /* WIFI_H */