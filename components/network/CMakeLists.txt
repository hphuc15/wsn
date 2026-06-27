# network
idf_component_register(
    SRCS
        "network.c"
            "wifi/wifi.c"
            "wifi/WiFiPanel/src/WiFiPanel.c"
            "wifi/WiFiPanel/src/WiFiPanel_Portal.c"
            "wifi/WiFiPanel/src/WiFiPanel_Page.c"
            "transport/transport.c"
            "transport/http/http.c"
            "transport/mqtt/mqtt.c"

    INCLUDE_DIRS
        "."
            "wifi"
            "wifi/WiFiPanel/include"
            "wifi/WiFiPanel/ui"
            "transport"
            "transport/http"
            "transport/mqtt"

    REQUIRES
        utilities config
        esp_wifi nvs_flash esp_http_server esp_event app_update # WiFiPanel
        esp_http_client mbedtls
        mqtt
)