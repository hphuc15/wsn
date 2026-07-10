#ifndef CONFIG_CREDENTIALS_H
#define CONFIG_CREDENTIALS_H

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

#endif /* CONFIG_CREDENTIALS_H */