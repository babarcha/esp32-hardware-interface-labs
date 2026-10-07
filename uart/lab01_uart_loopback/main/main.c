#include <stdio.h>
#include <string.h>

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define UART_PORT UART_NUM_0
#define UART_BAUD_RATE 115200
#define RX_BUFFER_SIZE 128

static int counter = 0;

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

static void process_command(const char *command)
{
    char response[64];

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
        send_response("ERROR=UNKNOWN_COMMAND");
    }
}

void app_main(void)
{
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

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

    char command[RX_BUFFER_SIZE];
    int command_length = 0;

    while (1)
    {
        uint8_t byte;

        int length = uart_read_bytes(
            UART_PORT,
            &byte,
            1,
            pdMS_TO_TICKS(100));

        if (length <= 0)
        {
            continue;
        }

        if (byte == '\n' || byte == '\r')
        {
            if (command_length > 0)
            {
                command[command_length] = '\0';

                process_command(command);

                command_length = 0;
            }
        }
        else if (command_length < RX_BUFFER_SIZE - 1)
        {
            command[command_length++] = (char)byte;
        }
    }
}