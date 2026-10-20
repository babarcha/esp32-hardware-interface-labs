
#include "hil_protocol.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bme280.h"
#include "sensor_manager.h"

#define HIL_UART_PORT UART_NUM_0
#define HIL_UART_RX_BUFFER_SIZE 256
#define HIL_RESPONSE_MAX_LENGTH 128

static void hil_send_response(const char *response)
{
    char output[HIL_RESPONSE_MAX_LENGTH + 8];

    int length = snprintf(
        output,
        sizeof(output),
        "HIL:%s\n",
        response);

    if (length <= 0)
    {
        return;
    }

    size_t bytes = (size_t)length;

    if (bytes >= sizeof(output))
    {
        bytes = sizeof(output) - 1;
    }

    uart_write_bytes(
        HIL_UART_PORT,
        output,
        bytes);
}

esp_err_t hil_protocol_init(void)
{
    uart_config_t config = {
        .baud_rate = HIL_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT};

    esp_err_t err = uart_param_config(
        HIL_UART_PORT,
        &config);

    if (err != ESP_OK)
    {
        return err;
    }

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
        HIL_UART_RX_BUFFER_SIZE,
        0,
        0,
        NULL,
        0);
}

void hil_protocol_process_command(
    const char *command,
    sensor_manager_t *manager,
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
        snprintf(
            response,
            response_size,
            "ERROR=UNKNOWN_COMMAND");
        return;
    }

    if (strcmp(command, "PING") == 0)
    {
        snprintf(
            response,
            response_size,
            "PONG");
        return;
    }

    if (strcmp(command, "GET_STATUS") == 0)
    {
        snprintf(
            response,
            response_size,
            "STATUS=%s",
            sensor_manager_is_ready(manager)
                ? "READY"
                : "NOT_READY");
        return;
    }

    if (strcmp(command, "REINIT_SENSOR") == 0)
    {
        esp_err_t err = sensor_manager_reinit(manager);

        snprintf(
            response,
            response_size,
            "%s",
            err == ESP_OK
                ? "SENSOR_REINIT=OK"
                : "ERROR=SENSOR_REINIT_FAILED");
        return;
    }

    bmp280_t *sensor = sensor_manager_get_active(manager);

    if (strcmp(command, "GET_SENSOR_INFO") == 0)
    {
        if (sensor == NULL)
        {
            snprintf(
                response,
                response_size,
                "ERROR=SENSOR_NOT_READY");
            return;
        }

        uint8_t chip_id = 0;

        esp_err_t err = bme280_read_chip_id(
            sensor->address,
            &chip_id);

        if (err != ESP_OK)
        {
            sensor_manager_mark_unavailable(manager);

            snprintf(
                response,
                response_size,
                "ERROR=SENSOR_IO");
            return;
        }

        if (chip_id != BMP280_CHIP_ID)
        {
            sensor_manager_mark_unavailable(manager);

            snprintf(
                response,
                response_size,
                "ERROR=UNEXPECTED_CHIP_ID");
            return;
        }

        snprintf(
            response,
            response_size,
            "SENSOR=BMP280;ID=0x%02X;ADDR=0x%02X",
            chip_id,
            sensor->address);
        return;
    }

    if (strcmp(command, "READ_SENSOR") == 0)
    {
        if (sensor == NULL)
        {
            snprintf(
                response,
                response_size,
                "ERROR=SENSOR_NOT_READY");
            return;
        }

        bmp280_measurement_t measurement = {0};

        esp_err_t err = bmp280_read_measurement(
            sensor,
            &measurement);

        if (err != ESP_OK)
        {
            sensor_manager_mark_unavailable(manager);

            snprintf(
                response,
                response_size,
                "ERROR=SENSOR_IO");
            return;
        }

        snprintf(
            response,
            response_size,
            "TEMP_C=%.2f;PRESSURE_HPA=%.2f",
            measurement.temperature_c,
            measurement.pressure_hpa);
        return;
    }

    snprintf(
        response,
        response_size,
        "ERROR=UNKNOWN_COMMAND");
}

void hil_protocol_run(sensor_manager_t *manager)
{
    char command[HIL_COMMAND_MAX_LENGTH];
    size_t command_length = 0;
    bool overflow = false;

    printf("HIL command interface ready\n");
    printf(
        "Commands: PING, GET_STATUS, GET_SENSOR_INFO, "
        "READ_SENSOR, REINIT_SENSOR\n");

    while (true)
    {
        uint8_t byte = 0;

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
                command[command_length] = '\0';

                char response[HIL_RESPONSE_MAX_LENGTH];

                hil_protocol_process_command(
                    command,
                    manager,
                    response,
                    sizeof(response));

                hil_send_response(response);
            }

            command_length = 0;
            overflow = false;
            continue;
        }

        if (overflow)
        {
            continue;
        }

        if (command_length < sizeof(command) - 1)
        {
            command[command_length++] = (char)byte;
        }
        else
        {
            overflow = true;
        }
    }
}
