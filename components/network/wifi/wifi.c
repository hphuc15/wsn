#include "wifi.h"
#include "utilities.h"
#include "config.h"
#include "WiFiPanel.h"
#include <stdlib.h>

/* WiFi portal credentials */
#define WP_AP_SSID              CRE_WP_AP_SSID
#define WP_AP_PASSWORD          CRE_WP_AP_PASSWORD
/* Server endpoint */
#define NVS_NAMESPACE_SERVER    CRE_NVS_NAMESPACE_SERVER
#define NVS_KEY_PROTOCOL        CRE_NVS_KEY_PROTOCOL
#define NVS_KEY_SERVERHOST      CRE_NVS_KEY_SERVERHOST
#define NVS_KEY_SERVERPORT      CRE_NVS_KEY_SERVERPORT
#define NVS_KEY_SERVERPATH      CRE_NVS_KEY_SERVERPATH
#define NVS_KEY_SERVERAUTH      CRE_NVS_KEY_SERVERAUTH

static const char *TAG = "[NETWORK][WIFI]";

typedef void (*wifi_cb_t)(void);

static wifi_cb_t s_on_connected = NULL;
static wifi_cb_t s_on_disconnected = NULL;

static void on_connected(void)
{
    if (s_on_connected)
    {
        s_on_connected();
    }
}

static void on_disconnected(void)
{
    if (s_on_disconnected)
    {
        s_on_disconnected();
    }
}

static WiFiPanel s_wp = {
    .ap_config = {
        .ssid = WP_AP_SSID,
        .password = WP_AP_PASSWORD,
        .max_connection = 1,
        .authmode = WIFI_AUTH_WPA2_PSK,
    },
    .ConnectedAP_Cb = on_connected,
    .DisconnectedAP_Cb = on_disconnected,
    .sta_retry_num = 20,
    .page = (WiFiPanelPage_t)WP_PAGE_COUNT(WP_PAGE_PARAM("protocol", "Protocol", "http,https,mqtt,mqtts", "mqtt", "select", true), WP_PAGE_PARAM("host", "Host", "e.g. 192.168.1.1", "", "text", true), WP_PAGE_PARAM("port", "Port", "e.g. 1883", "1883", "number", true), WP_PAGE_PARAM("path", "Path", "e.g. api/v1/telemetry", "", "text", false), WP_PAGE_PARAM("auth", "Token", "Device token", "", "password", false))};

void wifi_set_connected_cb(void *cb)
{
    s_on_connected = (wifi_cb_t)cb;
}

void wifi_set_disconnected_cb(void *cb)
{
    s_on_disconnected = (wifi_cb_t)cb;
}

int wifi_init(void)
{
    return (WiFiPanel_Init(&s_wp) == WP_OK) ? 0 : -1;
}

int wifi_start(void)
{
    return (WiFiPanel_AutoConnect(&s_wp) == WP_OK) ? 0 : -1;
}

int wifi_config(void)
{
    WiFiPanel_Status status;

    /* Stop current WiFi */
    status = WiFiPanel_Stop(&s_wp);
    if (status != WP_OK)
    {
        Utils_LogE(TAG, "Failed to stop WiFi in config mode.");
        return -1;
    }

    status = WiFiPanel_ConfigViaAP(&s_wp);
    if (status == WP_ERR_TIMEOUT)
    {
        Utils_LogW(TAG, "Config mode timed out, back to AutoConnect mode.");
        status = WiFiPanel_AutoConnect(&s_wp);
    }
    else if (status != WP_OK)
    {
        Utils_LogE(TAG, "Failed to start config mode.");
        return -1;
    }

    /* Save credentials to NVS */
    if (Utils_NVS_Init() != UTILS_OK)
    {
        Utils_LogE(TAG, "Failed to initialize NVS.");
        return -1;
    }

    const char *protocol = WiFiPanelPage_GetParam(&s_wp, NVS_KEY_PROTOCOL);
    const char *host = WiFiPanelPage_GetParam(&s_wp, NVS_KEY_SERVERHOST);
    const char *port = WiFiPanelPage_GetParam(&s_wp, NVS_KEY_SERVERPORT);
    const char *path = WiFiPanelPage_GetParam(&s_wp, NVS_KEY_SERVERPATH);
    const char *auth = WiFiPanelPage_GetParam(&s_wp, NVS_KEY_SERVERAUTH);

    if (protocol != NULL)
    {
        Utils_NVS_WriteString(NVS_NAMESPACE_SERVER, NVS_KEY_PROTOCOL, protocol);
    }
    if (host != NULL)
    {
        Utils_NVS_WriteString(NVS_NAMESPACE_SERVER, NVS_KEY_SERVERHOST, host);
    }
    if (port != NULL)
    {
        char *end;
        long val = strtol(port, &end, 10);
        if (end != port)
        {
            Utils_NVS_WriteInt(NVS_NAMESPACE_SERVER, NVS_KEY_SERVERPORT, (uint32_t)val);
        }
    }
    if (path != NULL)
    {
        Utils_NVS_WriteString(NVS_NAMESPACE_SERVER, NVS_KEY_SERVERPATH, path);
    }
    if (auth != NULL)
    {
        Utils_NVS_WriteString(NVS_NAMESPACE_SERVER, NVS_KEY_SERVERAUTH, auth);
    }
    Utils_LogI(TAG, "Credentials saved to NVS.");

    return 0;
}

int wifi_stop(void)
{
    return (WiFiPanel_Stop(&s_wp) == WP_OK) ? 0 : -1;
}