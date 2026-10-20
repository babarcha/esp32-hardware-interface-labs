
#include <stdio.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "i2c_bus.h"
#include "bme280.h"

void app_main(void)
{
    printf("\n");
    printf("I2C Lab 02 - Pass 7: BMP280 Measurements\n");
    printf("----------------------------------------\n");

    esp_err_t err = i2c_bus_init();

    if (err != ESP_OK)
    {
        printf("I2C initialization failed: %s\n",
               esp_err_to_name(err));
        return;
    }

    printf("I2C controller initialized\n");
    printf("SDA = GPIO%d\n", I2C_MASTER_SDA_IO);
    printf("SCL = GPIO%d\n", I2C_MASTER_SCL_IO);

    int devices_found = 0;

    err = i2c_bus_scan(&devices_found);

    if (err != ESP_OK)
    {
        printf("I2C scan failed: %s\n",
               esp_err_to_name(err));
        goto cleanup;
    }

    uint8_t address = 0;

    if (i2c_bus_probe(BME280_I2C_ADDR_LOW) == ESP_OK)
    {
        address = BME280_I2C_ADDR_LOW;
    }
    else if (i2c_bus_probe(BME280_I2C_ADDR_HIGH) == ESP_OK)
    {
        address = BME280_I2C_ADDR_HIGH;
    }

    if (address == 0)
    {
        printf("BMP280 not detected\n");
        goto cleanup;
    }

    uint8_t chip_id = 0;

    err = bme280_read_chip_id(address, &chip_id);

    if (err != ESP_OK)
    {
        printf("Chip ID read failed: %s\n",
               esp_err_to_name(err));
        goto cleanup;
    }

    printf("\nSensor address: 0x%02X\n", address);
    printf("Chip ID: 0x%02X\n", chip_id);
    printf("Sensor: %s\n",
           bme280_sensor_name(
               bme280_get_sensor_type(chip_id)));

    bmp280_t sensor;

    err = bmp280_init(&sensor, address);

    if (err != ESP_OK)
    {
        printf("BMP280 initialization failed: %s\n",
               esp_err_to_name(err));
        goto cleanup;
    }

    printf("BMP280 calibration loaded\n");
    printf("BMP280 configured in normal mode\n");
    printf("\nStarting measurements...\n");

    // The first normal-mode conversion needs time to complete.
    vTaskDelay(pdMS_TO_TICKS(100));

    while (1)
    {
        bmp280_measurement_t measurement;

        err = bmp280_read_measurement(
            &sensor,
            &measurement);

        if (err == ESP_OK)
        {
            printf("Temperature: %.2f C | "
                   "Pressure: %.2f hPa\n",
                   measurement.temperature_c,
                   measurement.pressure_hpa);
        }
        else
        {
            printf("Measurement failed: %s\n",
                   esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }

cleanup:
    err = i2c_bus_deinit();

    if (err != ESP_OK)
    {
        printf("I2C cleanup failed: %s\n",
               esp_err_to_name(err));
    }
}
