#include "utils_nvs.h"
#include "utilities.h"

#include "nvs_flash.h"
#include "esp_err.h"

Utils_Status Utils_NVS_Init(void)
{
    /* Initialize NVS */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* NVS partition was truncated and needs to be erased. Retry nvs_flash_init */
        if(nvs_flash_erase() != ESP_OK || nvs_flash_init() != ESP_OK){
            return UTILS_ERROR_NVS;
        }
    }

    return UTILS_OK;
}

Utils_Status Utils_NVS_ReadInt(const char *namespace_name, const char *key, int32_t *value)
{
    esp_err_t err;

    /* Open NVS */
    nvs_handle_t nvs_handle;
    err = nvs_open(namespace_name, NVS_READONLY, &nvs_handle);
    if(err == ESP_ERR_NVS_NOT_FOUND){
        return UTILS_ERROR_NVS_NOT_FOUND;
    } else if (err != ESP_OK) {
        return UTILS_ERROR_NVS;
    }

    /* Read */
    err = nvs_get_i32(nvs_handle, key, value);
    if(err == ESP_ERR_NVS_NOT_FOUND){
        nvs_close(nvs_handle);
        return UTILS_ERROR_NVS_NOT_FOUND;
    } else if(err != ESP_OK){
        nvs_close(nvs_handle);
        return UTILS_ERROR_NVS;
    }

    /* Close NVS */
    nvs_close(nvs_handle);
    return UTILS_OK;
}

Utils_Status Utils_NVS_WriteInt(const char *namespace_name, const char *key, int32_t value)
{
    esp_err_t err;

    /* Open NVS */
    nvs_handle_t nvs_handle;
    err = nvs_open(namespace_name, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return UTILS_ERROR_NVS;
    }

    /* Write */
    err = nvs_set_i32(nvs_handle, key, value);
    if(err != ESP_OK){
        nvs_close(nvs_handle);
        return UTILS_ERROR_NVS;
    }
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return UTILS_ERROR_NVS;
    }

    /* Close NVS */
    nvs_close(nvs_handle);
    return UTILS_OK;
}

Utils_Status Utils_NVS_ReadString(const char *namespace_name, const char *key, char *value, size_t max_len)
{
    esp_err_t err;
    nvs_handle_t nvs_handle;

    err = nvs_open(namespace_name, NVS_READONLY, &nvs_handle);
    if(err == ESP_ERR_NVS_NOT_FOUND){
        return UTILS_ERROR_NVS_NOT_FOUND;
    } else if (err != ESP_OK) {
        return UTILS_ERROR_NVS;
    }

    /* Check required size first */
    size_t required_size = 0;
    err = nvs_get_str(nvs_handle, key, NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(nvs_handle);
        return UTILS_ERROR_NVS_NOT_FOUND;
    } else if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return UTILS_ERROR_NVS;
    }

    if (required_size > max_len) {
        nvs_close(nvs_handle);
        return UTILS_ERROR_NVS_BUFFER_TOO_SMALL;
    }

    /* Read */
    err = nvs_get_str(nvs_handle, key, value, &max_len);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return UTILS_ERROR_NVS;
    }

    nvs_close(nvs_handle);
    return UTILS_OK;
}

Utils_Status Utils_NVS_WriteString(const char *namespace_name, const char *key, const char *value)
{
    esp_err_t err;
    nvs_handle_t nvs_handle;

    err = nvs_open(namespace_name, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return UTILS_ERROR_NVS;
    }

    err = nvs_set_str(nvs_handle, key, value);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return UTILS_ERROR_NVS;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return UTILS_ERROR_NVS;
    }

    nvs_close(nvs_handle);
    return UTILS_OK;
}

Utils_Status Utils_NVS_ReadFloat(const char *namespace_name, const char *key, float *value)
{
    esp_err_t err;
    nvs_handle_t nvs_handle;

    err = nvs_open(namespace_name, NVS_READONLY, &nvs_handle);
    if(err == ESP_ERR_NVS_NOT_FOUND){
        return UTILS_ERROR_NVS_NOT_FOUND;
    } else if (err != ESP_OK) {
        return UTILS_ERROR_NVS;
    }

    size_t required_size = sizeof(float);
    err = nvs_get_blob(nvs_handle, key, value, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(nvs_handle);
        return UTILS_ERROR_NVS_NOT_FOUND;
    } else if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return UTILS_ERROR_NVS;
    }

    nvs_close(nvs_handle);
    return UTILS_OK;
}

Utils_Status Utils_NVS_WriteFloat(const char *namespace_name, const char *key, float value)
{
    esp_err_t err;
    nvs_handle_t nvs_handle;

    err = nvs_open(namespace_name, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return UTILS_ERROR_NVS;
    }

    err = nvs_set_blob(nvs_handle, key, &value, sizeof(float));
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return UTILS_ERROR_NVS;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return UTILS_ERROR_NVS;
    }

    nvs_close(nvs_handle);
    return UTILS_OK;
}

Utils_Status Utils_NVS_ReadBlob(const char *namespace_name, const char *key, void *value, size_t *inout_len)
{
    esp_err_t err;
    nvs_handle_t nvs_handle;

    err = nvs_open(namespace_name, NVS_READONLY, &nvs_handle);
    if(err == ESP_ERR_NVS_NOT_FOUND){
        return UTILS_ERROR_NVS_NOT_FOUND;
    } else if (err != ESP_OK) {
        return UTILS_ERROR_NVS;
    }

    err = nvs_get_blob(nvs_handle, key, value, inout_len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(nvs_handle);
        return UTILS_ERROR_NVS_NOT_FOUND;
    } else if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return UTILS_ERROR_NVS;
    }

    nvs_close(nvs_handle);
    return UTILS_OK;
}

Utils_Status Utils_NVS_WriteBlob(const char *namespace_name, const char *key, const void *value, size_t len)
{
    esp_err_t err;
    nvs_handle_t nvs_handle;

    err = nvs_open(namespace_name, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return UTILS_ERROR_NVS;
    }

    err = nvs_set_blob(nvs_handle, key, value, len);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return UTILS_ERROR_NVS;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return UTILS_ERROR_NVS;
    }

    nvs_close(nvs_handle);
    return UTILS_OK;
}