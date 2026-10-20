
#include <stdio.h>
#include <stdint.h>

#include "esp_err.h"
#include "i2c_bus.h"
#include "bme280.h"

void app_main(void)
{
    printf("\n");
    printf("I2C Lab 02 - Pass 6: Sensor Identification\n");
    printf("------------------------------------------\n");

    // Step 1: Initialize the I2C controller.
    esp_err_t err = i2c_bus_init();

    if (err != ESP_OK)
    {
        printf("I2C initialization failed: %s\n",
               esp_err_to_name(err));
        return;
    }

    printf("I2C controller initialized successfully\n");
    printf("SDA = GPIO%d\n", I2C_MASTER_SDA_IO);
    printf("SCL = GPIO%d\n", I2C_MASTER_SCL_IO);

    // Step 2: Scan the bus for connected devices.
    int devices_found = 0;

    err = i2c_bus_scan(&devices_found);

    if (err != ESP_OK)
    {
        printf("I2C scan failed: %s\n",
               esp_err_to_name(err));
        goto cleanup;
    }

    printf("\nApplication: %d I2C device(s) detected\n",
           devices_found);

    if (devices_found == 0)
    {
        printf("No I2C devices detected.\n");
        goto cleanup;
    }

    // Step 3: Find the BMP280/BME280 address.
    uint8_t sensor_addr = 0;

    if (i2c_bus_probe(BME280_I2C_ADDR_LOW) == ESP_OK)
    {
        sensor_addr = BME280_I2C_ADDR_LOW;
    }
    else if (i2c_bus_probe(BME280_I2C_ADDR_HIGH) == ESP_OK)
    {
        sensor_addr = BME280_I2C_ADDR_HIGH;
    }

    if (sensor_addr == 0)
    {
        printf("BMP280/BME280 not found at 0x76 or 0x77\n");
        goto cleanup;
    }

    printf("\nSensor responding at address: 0x%02X\n",
           sensor_addr);

    // Step 4: Read the sensor identification register.
    uint8_t chip_id = 0;

    printf("\nReading sensor chip ID...\n");
    printf("Register: 0x%02X\n", BME280_REG_CHIP_ID);

    err = bme280_read_chip_id(
        sensor_addr,
        &chip_id);

    if (err != ESP_OK)
    {
        printf("Chip ID read failed: %s\n",
               esp_err_to_name(err));
        goto cleanup;
    }

    printf("Chip ID: 0x%02X\n", chip_id);

    // Step 5: Identify the sensor.
    sensor_type_t sensor_type =
        bme280_get_sensor_type(chip_id);

    printf("Sensor detected: %s\n",
           bme280_sensor_name(sensor_type));

    if (sensor_type == SENSOR_UNKNOWN)
    {
        printf("WARNING: Unexpected chip ID.\n");
    }
    else
    {
        printf("Sensor identification successful!\n");
    }

cleanup:
    // Step 6: Release the I2C controller.
    err = i2c_bus_deinit();

    if (err != ESP_OK)
    {
        printf("I2C cleanup failed: %s\n",
               esp_err_to_name(err));
    }
    else
    {
        printf("\nI2C bus released successfully\n");
    }

    printf("\nPass 6 execution complete.\n");
}
