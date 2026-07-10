#ifndef HTTP_H
#define HTTP_H

#include <stdint.h>
#include <stdbool.h>

/** @brief Build target URL from host/port/path. @return 0 on success, -1 on error. */
int http_init(const char *host, uint32_t port, const char *path, bool tls);

/** @brief POST data to the URL built by http_init(). @return 0 on success, -1 on error. */
int http_publish(const char *data);

#endif /* HTTP_H */