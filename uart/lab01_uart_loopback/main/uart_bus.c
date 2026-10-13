#include "uart_bus.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"

/* USB-to-UART bridge on typical ESP32 development boards connects to UART0.
 * UART0 also carries ESP-IDF boot and console logs. */
#define LAB_UART UART_NUM_0
#define LAB_BAUD_RATE 115200
#define LAB_RX_BUFFER_SIZE 1024
#define LAB_TX_BUFFER_SIZE 1024

static bool initialized = false;

esp_err_t uart_bus_init(void)
{
    if (initialized) return ESP_ERR_INVALID_STATE;
    uart_config_t config = {
        .baud_rate = LAB_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    esp_err_t err = uart_param_config(LAB_UART, &config);
    if (err != ESP_OK) return err;
    /* Preserve the board's existing UART0 TX/RX pin routing. */
    err = uart_driver_install(LAB_UART, LAB_RX_BUFFER_SIZE,
                              LAB_TX_BUFFER_SIZE, 0, NULL, 0);
    if (err == ESP_OK) initialized = true;
    return err;
}

esp_err_t uart_bus_deinit(void)
{
    if (!initialized) return ESP_ERR_INVALID_STATE;
    esp_err_t err = uart_driver_delete(LAB_UART);
    if (err == ESP_OK) initialized = false;
    return err;
}

esp_err_t uart_bus_read_line(char *buffer, size_t capacity)
{
    if (!initialized) return ESP_ERR_INVALID_STATE;
    if (buffer == NULL || capacity < 2) return ESP_ERR_INVALID_ARG;
    size_t count = 0;
    bool overflow = false;
    while (true) {
        uint8_t byte;
        int received = uart_read_bytes(LAB_UART, &byte, 1, pdMS_TO_TICKS(100));
        if (received < 0) return ESP_FAIL;
        if (received == 0) continue;
        if (byte == '\n') {
            buffer[count] = '\0';
            return overflow ? ESP_ERR_INVALID_SIZE : ESP_OK;
        }
        if (byte == '\r') continue;
        if (!overflow) {
            if (count + 1 < capacity) buffer[count++] = (char)byte;
            else overflow = true;
        }
    }
}

esp_err_t uart_bus_write_line(const char *line)
{
    if (!initialized) return ESP_ERR_INVALID_STATE;
    if (line == NULL) return ESP_ERR_INVALID_ARG;
    size_t length = strlen(line);
    if (uart_write_bytes(LAB_UART, line, length) != (int)length) return ESP_FAIL;
    if (uart_write_bytes(LAB_UART, "\r\n", 2) != 2) return ESP_FAIL;
    return uart_wait_tx_done(LAB_UART, pdMS_TO_TICKS(1000));
}
