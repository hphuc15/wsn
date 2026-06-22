/**
 * You will need to replace these field by your setup credentials.
 * Then change this file name from config_credentials_example.h to
 * config_credentials.h
 */
#ifndef CONFIG_CREDENTIALS_H
#define CONFIG_CREDENTIALS_H

/* WiFi portal credentials */
#define CRE_WP_AP_SSID              "HelloWorld"
#define CRE_WP_AP_PASSWORD          "12345678"
/* Server credentials */
#define CRE_NVS_NAMESPACE_SERVER    "wsn_server_np"
#define CRE_NVS_KEY_PROTOCOL        "wsn_communication_protocol_nvskey"
#define CRE_NVS_KEY_SERVERHOST      "wsn_endpoint_host_nvskey"
#define CRE_NVS_KEY_SERVERPORT      "wsn_endpoint_port_nvskey"
#define CRE_NVS_KEY_SERVERPATH      "wsn_endpoint_path_nvskey"
#define CRE_NVS_KEY_SERVERAUTH      "wsn_endpoint_auth_nvskey"
#define CRE_NVS_KEY_MQTTTOPIC       "wsn_endpoint_mqtt_topic_nvskey"
/* Default server endpoint */
#define CRE_NETWORK_DEFAULT_HOST    "wsn_default_server_host"
#define CRE_NETWORK_DEFAULT_PORT    443
#define CRE_NETWORK_DEFAULT_PATH    "wsn/example/ingest/data"
#define CRE_NETWORK_DEFAULT_TLS     true

#endif /* CONFIG_CREDENTIALS_H */