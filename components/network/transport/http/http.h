#ifndef HTTP_H
#define HTTP_H

#include <stdint.h>
#include <stdbool.h>

int http_init(const char *host, uint32_t port, const char *path, bool tls);
int http_publish(const char *data);

#endif /* HTTP_H */