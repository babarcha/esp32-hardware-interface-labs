idf_component_register(
    SRCS
        "main.c"
        "twai_bus.c"
    INCLUDE_DIRS "."
    REQUIRES driver
)