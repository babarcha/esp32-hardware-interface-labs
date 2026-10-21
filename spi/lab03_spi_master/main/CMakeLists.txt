
idf_component_register(
    SRCS
        "main.c"
        "spi_bus.c"
        "bmp280_spi.c"
        "hil_uart.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        esp_driver_spi
        esp_driver_uart
        freertos
)
