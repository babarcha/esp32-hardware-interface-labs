
idf_component_register(
    SRCS
        "main.c"
        "i2c_bus.c"
        "bme280.c"
        "bmp280_compensation.c"
    INCLUDE_DIRS
        "."
    PRIV_REQUIRES
        esp_driver_i2c
)
