#include "hil_protocol.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define HIL_UART_PORT UART_NUM_0
#define HIL_RX_BUFFER_SIZE 256

esp_err_t hil_protocol_init(void)
{
    const uart_config_t config = {
        .baud_rate = HIL_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT};

    esp_err_t err = uart_param_config(
        HIL_UART_PORT, &config);

    if (err != ESP_OK)
    {
        return err;
    }

    // UART0 uses the board's default console TX/RX pins.
    err = uart_set_pin(
        HIL_UART_PORT,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE);

    if (err != ESP_OK)
    {
        return err;
    }

    return uart_driver_install(
        HIL_UART_PORT,
        HIL_RX_BUFFER_SIZE,
        0,
        0,
        NULL,
        0);
}

void hil_protocol_process_command(
    const char *command,
    bmp280_t *sensor,
    char *response,
    size_t response_size)
{
    if (response == NULL || response_size == 0)
    {
        return;
    }

    response[0] = '\0';

    if (command == NULL)
    {
        snprintf(response, response_size,
                 "ERROR=INVALID_COMMAND");
        return;
    }

    if (strcmp(command, "PING") == 0)
    {
        snprintf(response, response_size, "PONG");
        return;
    }

    if (strcmp(command, "GET_STATUS") == 0)
    {
        snprintf(response, response_size,
                 "STATUS=%s",
                 (sensor != NULL && sensor->initialized)
                     ? "READY"
                     : "NOT_READY");
        return;
    }

    if (strcmp(command, "GET_SENSOR_INFO") == 0)
    {
        if (sensor == NULL || !sensor->initialized)
        {
            snprintf(response, response_size,
                     "ERROR=SENSOR_NOT_READY");
            return;
        }

        uint8_t chip_id = 0;

        esp_err_t err = bme280_read_chip_id(
            sensor->address, &chip_id);

        if (err != ESP_OK)
        {
            snprintf(response, response_size,
                     "ERROR=SENSOR_IO");
            return;
        }

        if (chip_id != BMP280_CHIP_ID)
        {
            snprintf(response, response_size,
                     "ERROR=UNEXPECTED_SENSOR_ID");
            return;
        }

        snprintf(response, response_size,
                 "SENSOR=BMP280;ID=0x%02X;ADDR=0x%02X",
                 chip_id, sensor->address);
        return;
    }

    if (strcmp(command, "READ_SENSOR") == 0)
    {
        if (sensor == NULL || !sensor->initialized)
        {
            snprintf(response, response_size,
                     "ERROR=SENSOR_NOT_READY");
            return;
        }

        bmp280_measurement_t measurement;

        esp_err_t err = bmp280_read_measurement(
            sensor, &measurement);

        if (err != ESP_OK)
        {
            snprintf(response, response_size,
                     "ERROR=SENSOR_IO");
            return;
        }

        snprintf(response, response_size,
                 "TEMP_C=%.2f;PRESSURE_HPA=%.2f",
                 measurement.temperature_c,
                 measurement.pressure_hpa);
        return;
    }

    snprintf(response, response_size,
             "ERROR=UNKNOWN_COMMAND");
}

static void hil_send_response(const char *response)
{
    char line[160];

    int length = snprintf(
        line, sizeof(line), "HIL:%s\n", response);

    if (length > 0 && length < (int)sizeof(line))
    {
        uart_write_bytes(HIL_UART_PORT, line, length);
    }
}

void hil_protocol_run(bmp280_t *sensor)
{
    char command[HIL_COMMAND_MAX_LENGTH];
    char response[128];

    size_t length = 0;
    bool overflow = false;

    printf("HIL command interface ready\n");
    printf("Commands: PING, GET_STATUS, "
           "GET_SENSOR_INFO, READ_SENSOR\n");

    while (1)
    {
        uint8_t byte;

        int received = uart_read_bytes(
            HIL_UART_PORT,
            &byte,
            1,
            pdMS_TO_TICKS(100));

        if (received <= 0)
        {
            continue;
        }

        if (byte == '\r')
        {
            continue;
        }

        if (byte == '\n')
        {
            if (overflow)
            {
                hil_send_response("ERROR=COMMAND_TOO_LONG");
            }
            else
            {
                command[length] = '\0';

                hil_protocol_process_command(
                    command, sensor,
                    response, sizeof(response));

                hil_send_response(response);
            }

            length = 0;
            overflow = false;
            continue;
        }

        if (overflow)
        {
            continue;
        }

        if (length < sizeof(command) - 1)
        {
            command[length++] = (char)byte;
        }
        else
        {
            overflow = true;
        }
    }
}