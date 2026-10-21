
/*
 * SPI Lab 03 — Pass 10.4
 * BMP280 SPI Hardware-in-the-Loop firmware.
 */

#include <stdio.h>

#include "esp_err.h"
#include "spi_bus.h"
#include "bmp280_spi.h"
#include "hil_uart.h"

#define BMP280_CS_GPIO 5
#define BMP280_CLOCK_HZ 1000000
#define BMP280_SPI_MODE 0

void app_main(void)
{
    printf("\n");
    printf("SPI Lab 03 - Pass 10.4: BMP280 UART HIL\n");
    printf("--------------------------------------\n");

    esp_err_t err = spi_bus_init();

    if (err != ESP_OK)
    {
        printf("SPI initialization failed: %s\n",
               esp_err_to_name(err));
        return;
    }

    spi_device_handle_t device = NULL;

    err = spi_device_register(
        BMP280_CS_GPIO,
        BMP280_CLOCK_HZ,
        BMP280_SPI_MODE,
        &device);

    if (err != ESP_OK)
    {
        printf("Device registration failed: %s\n",
               esp_err_to_name(err));
        goto cleanup_bus;
    }

    bmp280_spi_t sensor = {0};

    err = bmp280_spi_init(&sensor, device);

    if (err != ESP_OK)
    {
        printf("BMP280 initialization failed: %s\n",
               esp_err_to_name(err));
        goto cleanup_device;
    }

    printf("SPI device initialized\n");

    /*
     * Run the UART command server.
     * Normally this function does not return.
     */
    err = hil_uart_run(&sensor);

    if (err != ESP_OK)
    {
        printf("HIL UART failed: %s\n",
               esp_err_to_name(err));
    }

cleanup_device:
    spi_device_unregister(device);

cleanup_bus:
    spi_bus_deinit();
}
