idf_component_register(
    SRCS
        "main.c"
        "spi_bus.c"
    INCLUDE_DIRS "."
    REQUIRES driver
)