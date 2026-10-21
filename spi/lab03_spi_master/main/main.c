
/*
 * SPI Lab 03 — Pass 10.2
 *
 * Modular BMP280 SPI driver integration.
 *
 * Application responsibilities:
 * 1. Initialize SPI bus.
 * 2. Register BMP280 SPI device.
 * 3. Initialize BMP280 driver context.
 * 4. Read and verify BMP280 chip ID.
 * 5. Release SPI resources.
 */

#include <stdio.h>
#include <stdint.h>

#include "esp_err.h"

#include "spi_bus.h"
#include "bmp280_spi.h"

#define BMP280_CS_GPIO 5
#define BMP280_CLOCK_HZ 1000000
#define BMP280_SPI_MODE 0

void app_main(void)
{
    printf("\n");
    printf("SPI Lab 03 - Pass 10.2: Modular BMP280 Driver\n");
    printf("---------------------------------------------\n");

    esp_err_t result = spi_bus_init();

    if (result != ESP_OK)
    {
        printf(
            "SPI bus initialization failed: %s\n",
            esp_err_to_name(result));

        return;
    }

    printf("SPI bus initialized successfully\n");

    spi_device_handle_t device = NULL;

    result = spi_device_register(
        BMP280_CS_GPIO,
        BMP280_CLOCK_HZ,
        BMP280_SPI_MODE,
        &device);

    if (result != ESP_OK)
    {
        printf(
            "SPI device registration failed: %s\n",
            esp_err_to_name(result));

        goto cleanup_bus;
    }

    printf("BMP280 SPI device registered\n");

    bmp280_spi_t sensor = {0};

    result = bmp280_spi_init(
        &sensor,
        device);

    if (result != ESP_OK)
    {
        printf(
            "BMP280 driver initialization failed: %s\n",
            esp_err_to_name(result));

        goto cleanup_device;
    }

    printf("BMP280 driver initialized\n");

    uint8_t chip_id = 0;

    result = bmp280_spi_read_chip_id(
        &sensor,
        &chip_id);

    if (result != ESP_OK)
    {
        printf(
            "BMP280 chip ID read failed: %s\n",
            esp_err_to_name(result));

        goto cleanup_device;
    }

    printf("BMP280 chip ID: 0x%02X\n", chip_id);

    result = bmp280_spi_verify(&sensor);

    if (result == ESP_OK)
    {
        printf("BMP280 SPI verification: PASS\n");
    }
    else
    {
        printf(
            "BMP280 SPI verification: FAIL (%s)\n",
            esp_err_to_name(result));
    }

cleanup_device:

    {
        esp_err_t cleanup_result =
            spi_device_unregister(device);

        if (cleanup_result != ESP_OK)
        {
            printf(
                "SPI device cleanup failed: %s\n",
                esp_err_to_name(cleanup_result));
        }
        else
        {
            printf("SPI device unregistered\n");
        }
    }

cleanup_bus:

    {
        esp_err_t cleanup_result =
            spi_bus_deinit();

        if (cleanup_result != ESP_OK)
        {
            printf(
                "SPI bus cleanup failed: %s\n",
                esp_err_to_name(cleanup_result));
        }
        else
        {
            printf("SPI resources released\n");
        }
    }
}
