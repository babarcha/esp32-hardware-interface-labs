
idf_component_register(
    SRCS
        "main.c"
        "spi_bus.c"
        "bmp280_spi.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        driver
)
