#include "http_transport.h"
#include "wifi.h"

#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"

#include <string.h>
#include <stdio.h>

static const char *TAG_HTTP = "[NETWORK][HTTP]";

static bool s_ready = false;
static char s_url[256] = {0};

esp_err_t http_transport_init(void)
{
    const char *host  = wifi_get_host();
    uint32_t    port  = wifi_get_port();
    const char *token = wifi_get_token();

    if (!host[0] || !token[0]) {
        ESP_LOGE(TAG_HTTP, "Missing config");
        return ESP_ERR_INVALID_STATE;
    }

    const char *scheme = (port == 443) ? "https" : "http";
    snprintf(s_url, sizeof(s_url), "%s://%s:%" PRIu32 "/api/v1/%s/telemetry", scheme, host, port, token);

    ESP_LOGI(TAG_HTTP, "Ready: %s", s_url);
    s_ready = true;
    return ESP_OK;
}
esp_err_t http_transport_publish(const char *payload)
{
    char auth[160];
    snprintf(auth, sizeof(auth), "Bearer %s", wifi_get_token());

    esp_http_client_config_t cfg = {
        .url        = s_url,
        .method     = HTTP_METHOD_POST,
        .timeout_ms = 5000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    esp_http_client_set_header(client, "Authorization", auth);
    esp_http_client_set_header(client, "Content-Type",  "application/json");
    esp_http_client_set_post_field(client, payload, strlen(payload));

    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);
    return err;
}

bool http_transport_is_ready(void) { return s_ready; }

esp_err_t http_transport_stop(void)
{
    s_ready = false;
    s_url[0] = '\0';
    return ESP_OK;
}