
/*
 * SPI Lab 03 — Pass 10.3
 *
 * BMP280 temperature and pressure measurement.
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
    printf("SPI Lab 03 - Pass 10.3: BMP280 Measurements\n");
    printf("-------------------------------------------\n");

    esp_err_t err = spi_bus_init();

    if (err != ESP_OK)
    {
        printf("SPI bus initialization failed: %s\n",
               esp_err_to_name(err));
        return;
    }

    printf("SPI bus initialized successfully\n");

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
        printf("Driver initialization failed: %s\n",
               esp_err_to_name(err));
        goto cleanup_device;
    }

    uint8_t chip_id = 0;

    err = bmp280_spi_read_chip_id(&sensor, &chip_id);

    if (err != ESP_OK)
    {
        printf("Chip ID read failed: %s\n",
               esp_err_to_name(err));
        goto cleanup_device;
    }

    printf("BMP280 chip ID: 0x%02X\n", chip_id);

    if (chip_id != BMP280_EXPECTED_ID)
    {
        printf("BMP280 verification: FAIL\n");
        goto cleanup_device;
    }

    printf("BMP280 verification: PASS\n");

    err = bmp280_spi_load_calibration(&sensor);

    if (err != ESP_OK)
    {
        printf("Calibration read failed: %s\n",
               esp_err_to_name(err));
        goto cleanup_device;
    }

    printf("BMP280 calibration loaded successfully\n");

    bmp280_measurement_t measurement = {0};

    err = bmp280_spi_measure(
        &sensor,
        &measurement);

    if (err == ESP_OK)
    {
        printf("\n");
        printf("BMP280 Measurement Results\n");
        printf("--------------------------\n");
        printf("Temperature: %.2f C\n",
               measurement.temperature_c);
        printf("Pressure:    %.2f hPa\n",
               measurement.pressure_hpa);
        printf("BMP280 measurement: PASS\n");
    }
    else
    {
        printf("BMP280 measurement: FAIL (%s)\n",
               esp_err_to_name(err));
    }

cleanup_device:
    {
        esp_err_t cleanup_err =
            spi_device_unregister(device);

        if (cleanup_err != ESP_OK)
            printf("Device cleanup failed: %s\n",
                   esp_err_to_name(cleanup_err));
    }

cleanup_bus:
    {
        esp_err_t cleanup_err = spi_bus_deinit();

        if (cleanup_err == ESP_OK)
            printf("SPI resources released\n");
        else
            printf("SPI cleanup failed: %s\n",
                   esp_err_to_name(cleanup_err));
    }
}
