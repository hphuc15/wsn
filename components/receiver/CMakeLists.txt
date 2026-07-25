idf_component_register(
    SRCS
        "receiver.c"
    INCLUDE_DIRS
        "."
        "sx127x"
    REQUIRES
        driver esp_timer
        utilities config
)

add_subdirectory(sx127x)
target_link_libraries(${COMPONENT_LIB} PRIVATE sx127x)