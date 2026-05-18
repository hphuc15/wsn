idf_component_register(
    SRCS
        "button/button.c" "led/led.c"
    INCLUDE_DIRS
        "." "button" "led"
    REQUIRES
        driver esp_attr
)