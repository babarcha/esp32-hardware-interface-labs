
/*
 * SPI Lab 03 — Pass 10.4
 * UART HIL command protocol for BMP280.
 *
 * UART0, 115200 baud, 8N1.
 * Requests: newline-terminated ASCII commands.
 * Responses: RSP:<payload>\n
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "hil_uart.h"

#define HIL_UART_PORT UART_NUM_0
#define HIL_UART_BAUD 115200
#define HIL_RX_BUFFER 256
#define HIL_COMMAND_MAX 96

static bool sensor_ready = false;

static void send_response(const char *response)
{
    printf("RSP:%s\n", response);
    fflush(stdout);
}

static esp_err_t initialize_sensor(
    bmp280_spi_t *sensor)
{
    sensor_ready = false;

    esp_err_t err = bmp280_spi_verify(sensor);

    if (err != ESP_OK)
        return err;

    err = bmp280_spi_load_calibration(sensor);

    if (err != ESP_OK)
        return err;

    sensor_ready = true;
    return ESP_OK;
}

static void handle_command(
    bmp280_spi_t *sensor,
    const char *command)
{
    if (strcmp(command, "PING") == 0)
    {
        send_response("PONG");
    }
    else if (strcmp(command, "GET_STATUS") == 0)
    {
        send_response(sensor_ready
                          ? "STATUS=READY"
                          : "STATUS=NOT_READY");
    }
    else if (strcmp(command, "GET_SENSOR_INFO") == 0)
    {
        uint8_t id = 0;

        esp_err_t err =
            bmp280_spi_read_chip_id(sensor, &id);

        if (err != ESP_OK)
        {
            send_response("ERROR=SENSOR_READ_FAILED");
            return;
        }

        char response[96];

        snprintf(
            response,
            sizeof(response),
            "SENSOR=BMP280;BUS=SPI;CHIP_ID=0x%02X",
            id);

        send_response(response);
    }
    else if (strcmp(command, "READ_SENSOR") == 0)
    {
        if (!sensor_ready)
        {
            send_response("ERROR=SENSOR_NOT_READY");
            return;
        }

        bmp280_measurement_t measurement = {0};

        esp_err_t err = bmp280_spi_measure(
            sensor,
            &measurement);

        if (err != ESP_OK)
        {
            send_response("ERROR=MEASUREMENT_FAILED");
            return;
        }

        char response[128];

        snprintf(
            response,
            sizeof(response),
            "TEMP_C=%.2f;PRESSURE_HPA=%.2f",
            measurement.temperature_c,
            measurement.pressure_hpa);

        send_response(response);
    }
    else if (strcmp(command, "REINIT_SENSOR") == 0)
    {
        esp_err_t err = initialize_sensor(sensor);

        send_response(
            err == ESP_OK
                ? "REINIT=OK"
                : "ERROR=REINIT_FAILED");
    }
    else
    {
        send_response("ERROR=UNKNOWN_COMMAND");
    }
}

esp_err_t hil_uart_run(bmp280_spi_t *sensor)
{
    if (sensor == NULL || sensor->spi_device == NULL)
        return ESP_ERR_INVALID_ARG;

    /*
     * UART0 is already used by ESP-IDF console.
     * Install the UART driver for buffered reads.
     */
    esp_err_t err = uart_driver_install(
        HIL_UART_PORT,
        HIL_RX_BUFFER,
        0,
        0,
        NULL,
        0);

    if (err != ESP_OK)
        return err;

    uart_flush_input(HIL_UART_PORT);

    err = initialize_sensor(sensor);

    printf("SPI BMP280 HIL protocol initialized\n");
    printf("Commands: PING, GET_STATUS, "
           "GET_SENSOR_INFO, READ_SENSOR, "
           "REINIT_SENSOR\n");

    if (err != ESP_OK)
        printf("WARNING: BMP280 not ready\n");

    char command[HIL_COMMAND_MAX] = {0};
    size_t position = 0;
    bool overflow = false;

    while (true)
    {
        uint8_t byte = 0;

        int count = uart_read_bytes(
            HIL_UART_PORT,
            &byte,
            1,
            pdMS_TO_TICKS(100));

        if (count <= 0)
            continue;

        if (byte == '\r' || byte == '\n')
        {
            if (overflow)
            {
                send_response("ERROR=COMMAND_TOO_LONG");
            }
            else if (position > 0)
            {
                command[position] = '\0';
                handle_command(sensor, command);
            }

            position = 0;
            overflow = false;
            continue;
        }

        if (overflow)
            continue;

        if (position < sizeof(command) - 1)
        {
            command[position++] = (char)byte;
        }
        else
        {
            overflow = true;
        }
    }

    /* Unreachable during normal operation. */
    return ESP_OK;
}
