#include "wifi.h"
#include "WiFiManager.h"
#include "network_credential.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include <string.h>
#include <stdlib.h>

/* Tương thích ngược */
/* NVS */
#define NVS_NAMESPACE           WSN_SERVER_NVS_NAMESPACE
#define NVS_KEY_HOST            WSN_SERVER_NVS_KEY_HOST
#define NVS_KEY_PORT            WSN_SERVER_NVS_KEY_PORT
#define NVS_KEY_TOKEN           WSN_SERVER_NVS_KEY_TOKEN
/* Default server */
#define SERVER_DEFAULT_HOST     WSN_SERVER_DEFAULT_HOST
#define SERVER_DEFAULT_PORT     WSN_SERVER_DEFAULT_PORT
#define DEVICE_TOKEN_DEFAULT    WSN_SERVER_DEFAULT_TOKEN
/* WM AP */
#define S_WM_AP_SSID            WSN_GATEWAY_AP_SSID
#define S_WM_AP_PASSWORD        WSN_GATEWAY_AP_SSID
#define S_WM_AP_MAX_CONNECTION  WSN_GATEWAY_AP_MAX_CONNECTION


static const char *TAG_WIFI = "[NETWORK][WIFI]";
static const char *TAG_NVS  = "[NETWORK][NVS]";

static char     s_nvs_host[128]  = {0};
static uint32_t s_nvs_port       = 0;
static char     s_nvs_token[128] = {0};
static bool     s_nvs_loaded     = false;



static WiFiManager_t s_wm = {
    .ap_config = {
        .ssid =             S_WM_AP_SSID,
        .password =         S_WM_AP_PASSWORD,
        .authmode =         WIFI_AUTH_WPA_WPA2_PSK,
        .max_connection =   S_WM_AP_MAX_CONNECTION
    }};

static bool s_wifi_ready = false;
static wifi_cb_t s_on_wifi_connected = NULL;
static wifi_cb_t s_on_wifi_disconnected = NULL;


/* NVS helpers */

static void s_nvs_save(const char *host, uint32_t port, const char *token)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK) {
        ESP_LOGE(TAG_NVS, "nvs_open failed");
        return;
    }

    esp_err_t err = ESP_OK;
    err |= nvs_set_str(h, NVS_KEY_HOST,  host);
    err |= nvs_set_u32(h, NVS_KEY_PORT,  port);
    err |= nvs_set_str(h, NVS_KEY_TOKEN, token);
    err |= nvs_commit(h);
    nvs_close(h);

    if (err == ESP_OK) {
        ESP_LOGI(TAG_NVS, "Saved %s:%" PRIu32, host, port);
    } else {
        ESP_LOGE(TAG_NVS, "Save failed (err=0x%x)", err);
    }
}

static void s_nvs_load_once(void)
{
    if (s_nvs_loaded) return;

    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK) {
        ESP_LOGW(TAG_NVS, "No saved config");
        s_nvs_loaded = true;
        return;
    }

    size_t host_len  = sizeof(s_nvs_host);
    size_t token_len = sizeof(s_nvs_token);
    bool ok = (nvs_get_str(h, NVS_KEY_HOST,  s_nvs_host,  &host_len)  == ESP_OK &&
               nvs_get_u32(h, NVS_KEY_PORT,  &s_nvs_port)             == ESP_OK &&
               nvs_get_str(h, NVS_KEY_TOKEN, s_nvs_token, &token_len) == ESP_OK);
    nvs_close(h);
    s_nvs_loaded = true;

    if (ok) {
        ESP_LOGI(TAG_NVS, "Loaded %s:%" PRIu32, s_nvs_host, s_nvs_port);
    } else {
        ESP_LOGW(TAG_NVS, "Keys missing, clearing cache");
        s_nvs_host[0] = s_nvs_token[0] = '\0';
        s_nvs_port = 0;
    }
}

/* Priority getters (static) */

static const char *s_get_host(void)
{
    const char *h = WiFiManagerPage_GetParam(&s_wm, "host");
    if (h && h[0]) return h;
    s_nvs_load_once();
    return (s_nvs_host[0]) ? s_nvs_host : SERVER_DEFAULT_HOST;
}

static uint32_t s_get_port(void)
{
    const char *s = WiFiManagerPage_GetParam(&s_wm, "port");
    if (s && s[0]) {
        char *end;
        uint32_t p = (uint32_t)strtoul(s, &end, 10);
        if (end != s) return p;
    }
    s_nvs_load_once();
    return s_nvs_port ? s_nvs_port : SERVER_DEFAULT_PORT;
}

static const char *s_get_token(void)
{
    const char *t = WiFiManagerPage_GetParam(&s_wm, "token");
    if (t && t[0]) return t;
    s_nvs_load_once();
    return (s_nvs_token[0]) ? s_nvs_token : DEVICE_TOKEN_DEFAULT;
}

/* WiFiManager callbacks  */

static void on_wifi_connected(void)
{
    s_wifi_ready = true;

    const char *host  = WiFiManagerPage_GetParam(&s_wm, "host");
    const char *token = WiFiManagerPage_GetParam(&s_wm, "token");
    if (host && host[0] && token && token[0]) {
        s_nvs_save(s_get_host(), s_get_port(), s_get_token());
        s_nvs_loaded = false;
    }

    if (s_on_wifi_connected){
        s_on_wifi_connected();
    }
}

static void on_wifi_disconnected(void)
{
    s_wifi_ready = false;
    if (s_on_wifi_disconnected){
        s_on_wifi_disconnected();
    }
}

/* Public getters */
const char *wifi_get_host(void)    { return s_get_host(); }
uint32_t    wifi_get_port(void)    { return s_get_port(); }
const char *wifi_get_token(void)   { return s_get_token(); }

/* Public API */

void wifi_init(void) {
    s_wm.ConnectedAP_Cb = on_wifi_connected;
    s_wm.DisconnectedAP_Cb = on_wifi_disconnected;
    s_wm.sta_retry_num = 5;

    WiFiManagerPage_Init(&s_wm);
    WiFiManagerPage_AddParam(&s_wm, "host", "Host", "e.g. broker.example.com", "", "text", false);
    WiFiManagerPage_AddParam(&s_wm, "port", "Port", "e.g. 8883", "", "number", false);
    WiFiManagerPage_AddParam(&s_wm, "token", "Token", "Bearer token", "", "password", false);
    WiFiManager_Init(&s_wm);
}

void wifi_connect(void) { WiFiManager_AutoConnect(&s_wm); }

void wifi_stop(void) {
    WiFiManager_Stop(&s_wm);
    s_wifi_ready = false;
}

void wifi_config(void) {
    // wifi_stop();
    WiFiManager_ConfigViaAP(&s_wm);
}

bool wifi_is_ready(void) { return s_wifi_ready; }
void wifi_set_connected_cb(wifi_cb_t cb) { s_on_wifi_connected = cb; }
void wifi_set_disconnected_cb(wifi_cb_t cb) { s_on_wifi_disconnected = cb; }