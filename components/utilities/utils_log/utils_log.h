#ifndef UTILS_LOG_H
#define UTILS_LOG_H

#include "esp_log.h"

#define Utils_LogI(tag, fmt, ...)  ESP_LOGI(tag, fmt, ##__VA_ARGS__)
#define Utils_LogW(tag, fmt, ...)  ESP_LOGW(tag, fmt, ##__VA_ARGS__)
#define Utils_LogE(tag, fmt, ...)  ESP_LOGE(tag, fmt, ##__VA_ARGS__)
#define Utils_LogD(tag, fmt, ...)  ESP_LOGD(tag, fmt, ##__VA_ARGS__)

#endif /* UTILS_LOG_H */