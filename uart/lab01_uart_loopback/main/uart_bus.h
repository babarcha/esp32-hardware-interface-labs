#ifndef UART_BUS_H
#define UART_BUS_H
#include <stddef.h>
#include "esp_err.h"

esp_err_t uart_bus_init(void);
esp_err_t uart_bus_deinit(void);
/* Blocks until a newline-delimited command arrives. Oversized lines are discarded.
 * Returns ESP_ERR_INVALID_SIZE for an oversized line. */
esp_err_t uart_bus_read_line(char *buffer, size_t capacity);
esp_err_t uart_bus_write_line(const char *line);
#endif
