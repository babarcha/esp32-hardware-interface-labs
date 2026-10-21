
#ifndef HIL_UART_H
#define HIL_UART_H

#include "esp_err.h"
#include "bmp280_spi.h"

/*
 * Run the blocking UART command loop.
 * sensor must remain valid for the loop lifetime.
 */
esp_err_t hil_uart_run(bmp280_spi_t *sensor);

#endif
