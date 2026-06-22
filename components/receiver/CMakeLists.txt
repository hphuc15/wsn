idf_component_register(
    SRCS
        "receiver.c"
        "sx127x/sx127x.c"
    INCLUDE_DIRS
        "."
        "sx127x"
    REQUIRES
        driver esp_timer
        utilities config
)