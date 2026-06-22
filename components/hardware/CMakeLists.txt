# hardware
idf_component_register(
    SRCS
        "hardware.c"

        # Power management
        "power/power.c"
        "io/button/button.c"

    INCLUDE_DIRS
        "."
        "io"
        "power"
            "io/button"

    REQUIRES
        driver esp_timer esp_adc
        utilities
        config
        
)