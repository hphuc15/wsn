#ifndef UTILS_TYPES_H
#define UTILS_TYPES_H

typedef enum {
    UTILS_OK,
    UTILS_ERROR_INVALID_ARGS,
    UTILS_ERROR_NVS,
    UTILS_ERROR_NVS_NOT_FOUND,          /**< The namespace is not initialized yet */
    UTILS_ERROR_NVS_BUFFER_TOO_SMALL,
} Utils_Status;

#endif /* UTILS_TYPES_H */