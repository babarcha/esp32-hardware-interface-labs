#include <stdio.h>
#include "esp_err.h"
#include "protocol.h"
#include "uart_bus.h"

#define COMMAND_CAPACITY 128
#define RESPONSE_CAPACITY 128

void app_main(void)
{
    esp_err_t err = uart_bus_init();
    if (err != ESP_OK) {
        printf("UART initialization failed: %s\n", esp_err_to_name(err));
        return;
    }
    char command[COMMAND_CAPACITY];
    char response[RESPONSE_CAPACITY];
    while (1) {
        err = uart_bus_read_line(command, sizeof(command));
        if (err == ESP_ERR_INVALID_SIZE) {
            uart_bus_write_line("ERROR=COMMAND_TOO_LONG");
            continue;
        }
        if (err != ESP_OK) {
            printf("UART read failed: %s\n", esp_err_to_name(err));
            continue;
        }
        /* Existing host-side tests synchronize on this debug line. */
        printf("CMD: %s\n", command);
        fflush(stdout);
        protocol_process_command(command, response, sizeof(response));
        err = uart_bus_write_line(response);
        if (err != ESP_OK) printf("UART write failed: %s\n", esp_err_to_name(err));
    }
}
