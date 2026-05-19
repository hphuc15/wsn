# WSN Gateway

## Structure
```text
📁 components
├── credentials
│   ├── CMakeLists.txt
│   └── network_credential.h
├── hardware
│   ├── button
│   │   ├── button.c
│   │   └── button.h
│   ├── led
│   │   ├── led.c
│   │   └── led.h
│   ├── CMakeLists.txt
│   ├── status.c
│   └── status.h
├── network
│   ├── recv
│   ├── send
│   │   ├── http
│   │   │   ├── http_transport.c
│   │   │   └── http_transport.h
│   │   └── wifi
│   │       ├── WiFiManager
│   │       ├── wifi.c
│   │       └── wifi.h
│   ├── CMakeLists.txt
│   ├── network.c
│   └── network.h
└── .gitignore

📁 main
├── app
│   ├── app.c
│   └── app.h
├── CMakeLists.txt
└── main.c
```


## Components
- hphuc15/WiFiManager

## Setup
On Windows:
```powershell
git clone https://github.com/hphuc15/wsn.git
cd wsn_gateway/components/
mkdir credentials
cd credentials/
```

Create `CMakeLists.txt` with content:
```txt
idf_component_register(
    INCLUDE_DIRS "."
)
```

Create `network_credential.h` with this template:
```c
#ifndef NETWORK_CREDENTIALS_H
#define NETWORK_CREDENTIALS_H

/* WiFi Credentials - Captive Portal */
#define WSN_GATEWAY_AP_SSID                 "<wsn_ap_ssid>"
#define WSN_GATEWAY_AP_PASSWORD             "<wsn_ap_password>"
#define WSN_GATEWAY_AP_MAX_CONNECTION       <wsn_ap_max_connection>
/* Server Credentials */
#define WSN_SERVER_DEFAULT_HOST             "<server_host>"
#define WSN_SERVER_DEFAULT_PORT             <server_port>
#define WSN_SERVER_DEVICE_TOKEN_DEFAULT     "<device_token>"
/* NVS Namespace */
#define WSN_SERVER_NVS_NAMESPACE            "<server_nvs_namespace>"
#define WSN_SERVER_NVS_KEY_HOST             "<nvs_key_server_host>"
#define WSN_SERVER_NVS_KEY_PORT             "<nvs_key_server_port>"
#define WSN_SERVER_NVS_KEY_TOKEN            "<nvs_key_device_token>"

#endif /* NETWORK_CREDENTIALS_H */
```
and replay `<field>` with your credentials.