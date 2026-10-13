idf_component_register(
    SRCS "main.c" "protocol.c" "uart_bus.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_uart
)
