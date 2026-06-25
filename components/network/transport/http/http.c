#include "http.h"
#include "utilities.h"

#include "esp_http_client.h"
#include "esp_crt_bundle.h"

#include <stdbool.h>

/* ================================= Internal State =============================== */

static const char *TAG_HTTP = "[HTTP]";
static char s_url[256]      = {0};
static bool s_https         = false;


/* ================================= Internal Functions =============================== */

/**
 * @brief Initialize HTTP protocol.
 * @param host Server ip or domain.
 * @param port Endpoint port.
 * @param path Endpoint path.
 * @return 0 on success, -1 on fail.
 */
int http_init(const char *host, uint32_t port, const char *path, bool tls){
    if(!host || !path){
        Utils_LogE(TAG_HTTP, "Failed to initialize HTPP.");
        return -1;
    }
    s_https = tls;
    
    /* Create URL */
    if(s_https == true){
        snprintf(s_url, sizeof(s_url), "https://%s:%ld/%s", host, port, path);
    } else{
        snprintf(s_url, sizeof(s_url), "http://%s:%ld/%s", host, port, path);
    }
    Utils_LogI(TAG_HTTP, "URL: %s", s_url);

    return 0;
}

/**
 * @brief Publish data by url that created at http_init function.
 * @param data payload.
 * @return 0 on success, -1 on fail.
 */
int http_publish(const char *data){
    if(!data){
        Utils_LogE(TAG_HTTP, "Failed to publish data.");
        return -1;
    }

    esp_http_client_config_t cfg = {
        .url        = s_url,
        .method     = HTTP_METHOD_POST,
        .timeout_ms = 5000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        Utils_LogE(TAG_HTTP, "Failed to init HTTP client.");
        return -1;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, data, strlen(data));

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        Utils_LogE(TAG_HTTP, "HTTP POST request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return -1;
    }

    int status = esp_http_client_get_status_code(client);
    Utils_LogI(TAG_HTTP, "HTTP POST status: %d", status);

    esp_http_client_cleanup(client);
    return 0;
}