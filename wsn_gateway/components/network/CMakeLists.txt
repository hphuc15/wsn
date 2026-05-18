idf_component_register(
    SRCS
        "network.c"
        "send/wifi/wifi.c" "send/http/http_transport.c"
    INCLUDE_DIRS
        "."
        "send/wifi" "send/http"                                 # Server side
    REQUIRES
        credentials                                             # Private credentials
        esp_wifi nvs_flash esp_http_server esp_event            # WiFiManager
        esp_http_client                                         # http
)