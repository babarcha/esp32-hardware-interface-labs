
#ifndef HIL_PROTOCOL_H
#define HIL_PROTOCOL_H

#include <stddef.h>
#include "esp_err.h"
#include "sensor_manager.h"

#define HIL_UART_BAUD_RATE 115200
#define HIL_COMMAND_MAX_LENGTH 64

esp_err_t hil_protocol_init(void);

void hil_protocol_run(sensor_manager_t *manager);

void hil_protocol_process_command(
    const char *command,
    sensor_manager_t *manager,
    char *response,
    size_t response_size);

#endif
