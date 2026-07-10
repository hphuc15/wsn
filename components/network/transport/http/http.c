#include "http.h"
#include "utilities.h"

#include "esp_http_client.h"
#include "esp_crt_bundle.h"

#include <string.h>
#include <stdio.h>

static const char *TAG = "[NW][TRANSPORT][HTTP]";
static char        s_url[256] = {0};

int http_init(const char *host, uint32_t port, const char *path, bool tls){
    if (!host || !path) {
        Utils_LogE(TAG, "Invalid host/path.");
        return -1;
    }

    snprintf(s_url, sizeof(s_url), "%s://%s:%lu/%s.", tls ? "https" : "http", host, (unsigned long)port, path);

    Utils_LogI(TAG, "URL: %s.", s_url);
    return 0;
}

int http_publish(const char *data){
    if (!data) {
        Utils_LogE(TAG, "Invalid data.");
        return -1;
    }

    esp_http_client_config_t cfg = {
        .url               = s_url,
        .method            = HTTP_METHOD_POST,
        .timeout_ms        = 5000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        Utils_LogE(TAG, "Failed to init HTTP client.");
        return -1;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, data, strlen(data));

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        Utils_LogE(TAG, "POST failed: %s.", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return -1;
    }

    Utils_LogI(TAG, "POST status: %d.", esp_http_client_get_status_code(client));

    esp_http_client_cleanup(client);
    return 0;
}