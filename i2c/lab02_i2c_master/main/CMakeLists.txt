idf_component_register(
    SRCS
        "main.c"
        "i2c_bus.c"
    INCLUDE_DIRS "."
    REQUIRES driver
)