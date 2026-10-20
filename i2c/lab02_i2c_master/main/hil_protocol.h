#ifndef HIL_PROTOCOL_H
#define HIL_PROTOCOL_H

#include <stddef.h>
#include "bme280.h"
#include "esp_err.h"

#define HIL_UART_BAUD_RATE 115200
#define HIL_COMMAND_MAX_LENGTH 64

// Initialize UART0 for command/response communication.
esp_err_t hil_protocol_init(void);

// Read and execute commands indefinitely.
// sensor may be NULL if sensor initialization failed.
void hil_protocol_run(bmp280_t *sensor);

// Process one command; useful for future protocol tests.
void hil_protocol_process_command(
    const char *command,
    bmp280_t *sensor,
    char *response,
    size_t response_size);

#endif