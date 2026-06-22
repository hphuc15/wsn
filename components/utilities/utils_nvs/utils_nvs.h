#ifndef UTILS_NVS_H
#define UTILS_NVS_H

#include "utils_types.h"

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Initialize NVS flash. Must be called once before any NVS operation.
 * @return UTILS_OK on success, UTILS_ERROR_NVS on fail.
 */
Utils_Status Utils_NVS_Init(void);

/**
 * @brief Read a 32-bit integer from NVS.
 * @return UTILS_OK, UTILS_ERROR_NVS_NOT_FOUND, or UTILS_ERROR_NVS.
 */
Utils_Status Utils_NVS_ReadInt(const char *namespace_name, const char *key, int32_t *value);

/**
 * @brief Write a 32-bit integer to NVS.
 * @return UTILS_OK or UTILS_ERROR_NVS.
 */
Utils_Status Utils_NVS_WriteInt(const char *namespace_name, const char *key, int32_t value);

/**
 * @brief Read a string from NVS.
 * @param max_len Size of the output buffer including null terminator.
 * @return UTILS_OK, UTILS_ERROR_NVS_NOT_FOUND, UTILS_ERROR_NVS_BUFFER_TOO_SMALL, or UTILS_ERROR_NVS.
 */
Utils_Status Utils_NVS_ReadString(const char *namespace_name, const char *key, char *value, size_t max_len);

/**
 * @brief Write a string to NVS.
 * @return UTILS_OK or UTILS_ERROR_NVS.
 */
Utils_Status Utils_NVS_WriteString(const char *namespace_name, const char *key, const char *value);

/**
 * @brief Read a float from NVS (stored as blob).
 * @return UTILS_OK, UTILS_ERROR_NVS_NOT_FOUND, or UTILS_ERROR_NVS.
 */
Utils_Status Utils_NVS_ReadFloat(const char *namespace_name, const char *key, float *value);

/**
 * @brief Write a float to NVS (stored as blob).
 * @return UTILS_OK or UTILS_ERROR_NVS.
 */
Utils_Status Utils_NVS_WriteFloat(const char *namespace_name, const char *key, float value);

/**
 * @brief Read a binary blob from NVS.
 * @param inout_len In: buffer size. Out: actual bytes read.
 * @return UTILS_OK, UTILS_ERROR_NVS_NOT_FOUND, or UTILS_ERROR_NVS.
 */
Utils_Status Utils_NVS_ReadBlob(const char *namespace_name, const char *key, void *value, size_t *inout_len);

/**
 * @brief Write a binary blob to NVS.
 * @return UTILS_OK or UTILS_ERROR_NVS.
 */
Utils_Status Utils_NVS_WriteBlob(const char *namespace_name, const char *key, const void *value, size_t len);

#endif /* UTILS_NVS_H */