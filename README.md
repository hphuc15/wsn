# WSN Gateway

[![Framework](https://img.shields.io/badge/Framework-ESP--IDF-blue.svg)](https://docs.espressif.com/projects/esp-idf/en/latest/)
[![Platform](https://img.shields.io/badge/Platform-ESP32-green.svg)](https://www.espressif.com/en/products/socs/esp32)
[![License](https://img.shields.io/badge/License-Apache%202.0-red.svg)](LICENSE)

## Overview

This is the central gateway in a `wsn` (Wireless Sensor Network) architecture. It bridges LoRa-based sensor nodes and cloud/on-premise servers via Wi-Fi. The gateway receives sensor data over LoRa, forwards it to a configured server using HTTP(s) or MQTT(s), and can also send beacon commands to nodes.

## Features

- `Dual‑protocol support`: switch between HTTP and MQTT protocols at runtime.
- `Secure connections`: TLS support for both HTTP(s) and MQTT(s).
- `Automatic Wi-Fi provisioning`: through a web‑based captive portal (WiFiPanel).
- `LoRa modulation`: send beacon and receive node data.
- `Credential management`: stored securely in NVS.

## Project Structure

```
📁 wsn_gateway
├── CMakeLists.txt
├── LICENSE
├── README.md
├── components
│   ├── config/                             <!-- Project credentials and configurations -->
│   ├── hardware/                           <!-- Hardware implement -->
│   ├── network                             <!-- Server side implement: http(s), mqtt(s), wifi -->
│   │   ├── CMakeLists.txt
│   │   ├── network.c
│   │   ├── network.h
│   │   ├── transport/
│   │   └── wifi
│   │       ├── WiFiPanel/                  <!-- Github: hphuc15/WiFiPanel -->
│   │       ├── wifi.c
│   │       └── wifi.h
│   ├── receiver                            <!-- Node side implement: LoRa receiver and send beacon -->
│   │   ├── CMakeLists.txt
│   │   ├── receiver.c
│   │   ├── receiver.h
│   │   └── sx127x/                         <!-- Github: hphuc15/baredrv -->
│   └── utilities
├── main                                    <!-- Main program -->
│   ├── CMakeLists.txt
│   ├── app/                                <!-- Application layer -->
│   └── main.c
└── sdkconfig
```

## Configuration

### 1. Clone the Repository

```bash
git clone --branch wsn_gateway --single-branch https://github.com/hphuc15/wsn.git wsn_gateway
cd wsn_gateway
```

### 2. Set up

Copy the example configuration file and edit it with your own credentials:

```bash
cp components/config/config_credentials_example.h components/config/config_credentials.h
```

Open `components/config/config_credentials.h` and replace the placeholder values:

```c
/* Wi‑Fi AP credentials (used for captive portal) */
#define CRE_WP_AP_SSID              "YourAccessPointSSID"
#define CRE_WP_AP_PASSWORD          "YourAccessPointPassword"

/* Server endpoint defaults - stored in NVS */
#define CRE_NVS_NAMESPACE_SERVER    "wsn_server"
#define CRE_NVS_KEY_PROTOCOL        "wsn_protocol"
#define CRE_NVS_KEY_SERVERHOST      "wsn_host"
#define CRE_NVS_KEY_SERVERPORT      "wsn_port"
#define CRE_NVS_KEY_SERVERPATH      "wsn_path"
#define CRE_NVS_KEY_SERVERAUTH      "wsn_auth"
#define CRE_NVS_KEY_MQTTTOPIC       "wsn_mqtt_topic"

/* Fallback server settings (used if NVS is empty) */
#define CRE_NETWORK_DEFAULT_HOST    "your.server.com"
#define CRE_NETWORK_DEFAULT_PORT    443
#define CRE_NETWORK_DEFAULT_PATH    "/api/ingest"
#define CRE_NETWORK_DEFAULT_TLS     true
```
`Note`: NVS key names are limited to 15 ASCII characters. Do not change the predefined keys unless you update the source code accordingly.

## Build
Build and flash this firmware to `ESP32`. This program was written in `ESP-IDF` framework.

## License
This project is licensed under the Apache-2.0 License.

## Contributing
Contributions are welcome! Please open an issue or submit a pull request for any improvements, bug fixes, or new features.