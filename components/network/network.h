#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stdbool.h>

int  network_init(void);
void network_deinit(void);

int  network_send(const char *data);
int  network_config(void);

void network_set_wifi_connected_cb(void *cb);
void network_set_wifi_disconnected_cb(void *cb);

void network_timesync(bool *time_is_valid);
void network_timesync_stop(void);

/**
 * @brief Block until the transport (HTTP URL / MQTT client) is ready to publish.
 * @param timeout_ms 0 = wait forever.
 * @return true if ready, false on timeout.
 */
bool network_wait_ready(uint32_t timeout_ms);

#endif /* NETWORK_H */