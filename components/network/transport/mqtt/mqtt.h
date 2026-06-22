#ifndef MQTT_H
#define MQTT_H

#include <stdint.h>
#include <stdbool.h>

/** @brief  Init MQTT client and connect to broker. Blocks up to 10 s.
 *  @param  host      Broker hostname or IP.
 *  @param  port      Broker port.
 *  @param  topic     Publish topic (copied internally).
 *  @param  username  Auth username / device token; NULL to skip.
 *  @param  password  Auth password; NULL to skip.
 *  @param  tls       true = MQTTS (TLS), false = plain MQTT.
 *  @return 0 on success, -1 on error. */
int mqtt_init(const char *host, uint32_t port, const char *topic, const char *username, const char *password, bool tls);

/** @brief  Publish @p data to the configured topic at QoS 1.
 *  @return 0 on success, -1 on error. */
int mqtt_publish(const char *data);

/** @brief  Stop and destroy the MQTT client. Safe to call if not initialised. */
void mqtt_deinit(void);

#endif /* MQTT_H */