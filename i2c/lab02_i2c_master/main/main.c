#include <stdio.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "i2c_bus.h"
#include "bme280.h"
#include "hil_protocol.h"

void app_main(void)
{
    printf("\n");
    printf("I2C Lab 02 - Pass 9: BMP280 HIL Validation\n");
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
    }

    uint8_t address = 0;

    if (i2c_bus_probe(BME280_I2C_ADDR_LOW) == ESP_OK)
    {
        address = BME280_I2C_ADDR_LOW;
    }
    else if (
        i2c_bus_probe(BME280_I2C_ADDR_HIGH) == ESP_OK)
    {
        address = BME280_I2C_ADDR_HIGH;
    }

    bmp280_t sensor = {0};
    bmp280_t *active_sensor = NULL;

    if (address == 0)
    {
        printf("BMP280 not detected\n");
    }
    else
    {
        printf("Sensor address: 0x%02X\n", address);

        err = bmp280_init(&sensor, address);

        if (err == ESP_OK)
        {
            active_sensor = &sensor;

            printf("BMP280 initialized\n");

            // Allow the first measurement to complete.
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        else
        {
            printf("BMP280 initialization failed: %s\n",
                   esp_err_to_name(err));
        }
    }

    err = hil_protocol_init();

    if (err != ESP_OK)
    {
        printf("HIL UART initialization failed: %s\n",
               esp_err_to_name(err));
        i2c_bus_deinit();
        return;
    }

    printf("HIL protocol initialized\n");

    // The command loop does not normally return.
    hil_protocol_run(active_sensor);
}