#ifndef WIFI_H
#define WIFI_H

#include <stdlib.h>

/** @brief Set WiFi connected callback */
void wifi_set_connected_cb(void *cb);
/** @brief Set WiFi disconnected callback */
void wifi_set_disconnected_cb(void *cb);
/** @brief Initial WiFi */
int wifi_init(void);
/** @brief Start WiFi with mode AutoConnect */
int wifi_start(void);
/** @brief Stop WiFi */
int wifi_stop(void);
/** @brief Start WiFi config mode, set up through WiFi AP */
int wifi_config(void);

#endif /* WIFI_H */