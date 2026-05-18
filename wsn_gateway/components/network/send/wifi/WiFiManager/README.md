# WiFiManager

WiFi Manager component for ESP32 with captive portal configuration.

## Features

- Configure WiFi credentials via captive portal (no hardcoded SSID/password)
- Built-in DNS server for captive portal redirect
- Customizable extra input fields on the portal form
- Callbacks for WiFi connected/disconnected events

## Requirements

- ESP-IDF >= 5.5.1

## Installation

Add to your project's `idf_component.yml`:

```yaml
dependencies:
  hphuc15/WiFiManager: ">=1.0.0"
```

Or clone directly into your `components/` folder:

```bash
git clone https://github.com/hphuc15/WiFiManager components/WiFiManager
```

## Usage

### Basic

```c
#include "WiFiManager.h"

static WiFiManager_t wm = {0};

static void on_connected(void) {
    printf("WiFi connected!\n");
}

static void on_disconnected(void) {
    printf("WiFi disconnected!\n");
}

void app_main(void) {
    esp_netif_init();

    wm.config.ap = WM_AP_CONFIG_DEFAULT();
    wm.ConnectedAP_Cb    = on_connected;
    wm.DisconnectedAP_Cb = on_disconnected;

    WiFiManager_Init(&wm);
    WiFiManager_ConfigViaAP(&wm);
}
```

### With extra form fields

```c
#include "WiFiManager.h"

static WiFiManager_t wm = {0};

void app_main(void) {
    esp_netif_init();

    wm.config.ap = WM_AP_CONFIG_DEFAULT();
    WiFiManager_Init(&wm);

    WiFiManagerPage_AddParam(&wm.page, "mqtt_host", "MQTT Broker",
                             "e.g. broker.hivemq.com", "", "text", false);
    WiFiManagerPage_AddParam(&wm.page, "mqtt_port", "MQTT Port",
                             "1883", "1883", "number", false);
    WiFiManagerPage_AddParam(&wm.page, "api_token", "API Token",
                             "your-secret-token", "", "password", true);

    WiFiManager_ConfigViaAP(&wm);

    const char *host  = WiFiManagerPage_GetParam(&wm.page, "mqtt_host");
    const char *port  = WiFiManagerPage_GetParam(&wm.page, "mqtt_port");
    const char *token = WiFiManagerPage_GetParam(&wm.page, "api_token");
}
```

## How it works

1. ESP32 starts in AP mode with the default SSID (`ESP32_Config`)
2. User connects to the AP and is redirected to the captive portal
3. User fills in WiFi credentials (and any extra fields) and submits
4. ESP32 switches to STA mode and connects to the configured WiFi network
5. `ConnectedAP_Cb` is called when the connection is established

## API

See [`include/WiFiManager.h`](include/WiFiManager.h) for full API documentation.

### WiFi Lifecycle

| Function | Description |
|---|---|
| `WiFiManager_Init` | Initialize event group, NVS and WiFi driver |
| `WiFiManager_StartSTA` | Start WiFi in Station mode |
| `WiFiManager_StartAP` | Start WiFi in Access Point mode |
| `WiFiManager_ConfigViaAP` | Collect credentials via captive portal then switch to STA |
| `WiFiManager_Stop` | Stop WiFi and unregister event handlers |
| `WiFiManager_Deinit` | Deinitialize WiFi driver, NVS and event loop |
| `WiFiManager_GetMode` | Get current WiFi mode |

### Captive Portal Page

| Function | Description |
|---|---|
| `WiFiManagerPage_Init` | Zero-initialize a page instance |
| `WiFiManagerPage_AddParam` | Add an extra input field to the portal form |
| `WiFiManagerPage_GetParam` | Get the submitted value of a field by id |
| `WiFiManagerPage_Build` | Build the full HTML page (caller must `free()`) |

## Configuration

Default AP settings can be overridden before calling `WiFiManager_Init`:

```c
// Default values (defined in WiFiManager.h)
#define WM_AP_SSID_DEFAULT    "ESP32_Config"
#define WM_AP_PSW_DEFAULT     "ESP32_Config"
#define WM_AP_MAX_STA_CONN    1
#define WM_MAX_PARAMS         5
#define WM_FIELD_LEN          128
#define WM_PORTAL_BODY_SIZE   512
```

## License

MIT