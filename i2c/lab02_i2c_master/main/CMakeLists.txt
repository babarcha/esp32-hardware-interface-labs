idf_component_register(
    SRCS
        "main.c"
        "i2c_bus.c"
        "bme280.c"
        "bmp280_compensation.c"
        "sensor_manager.c"
        "hil_protocol.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES
        esp_driver_i2c
        esp_driver_uart
)