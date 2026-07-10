#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stdbool.h>

/** @brief Init WiFi + network stack, starts connection task. @return 0 on success, -1 on fail. */
int  network_init(void);

/** @brief Deinit transport and free network resources. */
void network_deinit(void);

/** @brief Publish JSON payload via active transport. @return 0 on success, -1 on fail. */
int  network_send(const char *data);

/** @brief Run WiFi + transport config flow (e.g. provisioning). @return 0 on success, -1 on fail. */
int  network_config(void);

/** @brief Set callback invoked when WiFi connects. */
void network_set_wifi_connected_cb(void *cb);

/** @brief Set callback invoked when WiFi disconnects. */
void network_set_wifi_disconnected_cb(void *cb);

/**
 * @brief Start SNTP time sync.
 * @param time_is_valid Set to true once time is synced.
 */
void network_timesync(bool *time_is_valid);

/** @brief Stop SNTP time sync. */
void network_timesync_stop(void);

/**
 * @brief Block until transport is ready to publish.
 * @param timeout_ms 0 = wait forever.
 * @return true if ready, false on timeout.
 */
bool network_wait_ready(uint32_t timeout_ms);

#endif /* NETWORK_H */