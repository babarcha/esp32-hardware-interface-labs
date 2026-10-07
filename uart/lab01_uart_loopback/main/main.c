/*
 * UART Lab 01
 *
 * Implements a simple newline-delimited command/response protocol
 * between a host PC and the ESP32 over UART0.
 *
 * Supported commands:
 *   PING        -> PONG
 *   GET_INFO    -> MODEL=ESP32;FW=1.0.0
 *   GET_COUNTER -> COUNTER=n
 *   other       -> ERROR=UNKNOWN_COMMAND
 */

#include <stdio.h>
#include <string.h>

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
 * UART0 is connected to the development board's USB-to-serial
 * interface, allowing the host Python application to use COM3.
 */
#define UART_PORT UART_NUM_0
#define UART_BAUD_RATE 115200
#define RX_BUFFER_SIZE 128

/* Application state used by the GET_COUNTER command. */
static int counter = 0;

/*
 * Send a newline-terminated response to the host.
 */
static void send_response(const char *response)
{
    uart_write_bytes(
        UART_PORT,
        response,
        strlen(response));

    uart_write_bytes(
        UART_PORT,
        "\r\n",
        2);
}

/*
 * Process one complete command received from the host.
 */
static void process_command(const char *command)
{
    char response[64];

    /* Diagnostic echo used by the host-side tests for synchronization. */
    printf("CMD: %s\n", command);

    if (strcmp(command, "PING") == 0)
    {
        send_response("PONG");
    }
    else if (strcmp(command, "GET_INFO") == 0)
    {
        send_response("MODEL=ESP32;FW=1.0.0");
    }
    else if (strcmp(command, "GET_COUNTER") == 0)
    {
        snprintf(
            response,
            sizeof(response),
            "COUNTER=%d",
            counter++);

        send_response(response);
    }
    else
    {
        /* Unknown commands are handled explicitly rather than ignored. */
        send_response("ERROR=UNKNOWN_COMMAND");
    }
}

void app_main(void)
{
    /*
     * Configure UART0 as 115200 8N1 with no hardware flow control.
     */
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    /*
     * Install the UART driver with an RX ring buffer.
     * No TX ring buffer or UART event queue is required for this lab.
     */
    uart_driver_install(
        UART_PORT,
        RX_BUFFER_SIZE * 2,
        0,
        0,
        NULL,
        0);

    uart_param_config(
        UART_PORT,
        &uart_config);

    printf("UART Lab 01 ready\n");

    /*
     * Commands are assembled one byte at a time until CR or LF
     * marks the end of the command.
     */
    char command[RX_BUFFER_SIZE];
    int command_length = 0;

    while (1)
    {
        uint8_t byte;

        /*
         * Read one byte at a time. The timeout prevents the task from
         * blocking indefinitely while no UART data is available.
         */
        int length = uart_read_bytes(
            UART_PORT,
            &byte,
            1,
            pdMS_TO_TICKS(100));

        if (length <= 0)
        {
            continue;
        }

        /*
         * Treat either CR or LF as a command delimiter.
         * Ignore empty delimiters, such as the LF following CR in CRLF.
         */
        if (byte == '\n' || byte == '\r')
        {
            if (command_length > 0)
            {
                /* Convert the accumulated bytes into a C string. */
                command[command_length] = '\0';

                process_command(command);

                /* Start collecting the next command. */
                command_length = 0;
            }
        }
        /*
         * Reserve one byte for the terminating '\0' to prevent
         * writing beyond the command buffer.
         */
        else if (command_length < RX_BUFFER_SIZE - 1)
        {
            command[command_length++] = (char)byte;
        }
    }
}