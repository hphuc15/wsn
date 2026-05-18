#ifndef HTTP_TRANSPORT_H
#define HTTP_TRANSPORT_H

#include "esp_err.h"
#include <stdbool.h>

esp_err_t http_transport_init(void);
esp_err_t http_transport_publish(const char *payload);
bool      http_transport_is_ready(void);
esp_err_t http_transport_stop(void);

#endif /* HTTP_TRANSPORT_H */