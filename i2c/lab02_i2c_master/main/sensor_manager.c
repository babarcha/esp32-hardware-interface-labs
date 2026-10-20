
#include "sensor_manager.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_bus.h"

void sensor_manager_init(sensor_manager_t *manager)
{
    if (manager == NULL)
    {
        return;
    }

    memset(manager, 0, sizeof(*manager));
}

bool sensor_manager_is_ready(const sensor_manager_t *manager)
{
    return manager != NULL && manager->ready;
}

bmp280_t *sensor_manager_get_active(sensor_manager_t *manager)
{
    if (!sensor_manager_is_ready(manager))
    {
        return NULL;
    }

    return &manager->sensor;
}

void sensor_manager_mark_unavailable(sensor_manager_t *manager)
{
    if (manager == NULL)
    {
        return;
    }

    manager->ready = false;
    manager->sensor.initialized = false;
}

esp_err_t sensor_manager_reinit(sensor_manager_t *manager)
{
    if (manager == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    sensor_manager_mark_unavailable(manager);
    memset(&manager->sensor, 0, sizeof(manager->sensor));

    const uint8_t addresses[] = {
        BME280_I2C_ADDR_LOW,
        BME280_I2C_ADDR_HIGH};

    for (size_t i = 0;
         i < sizeof(addresses) / sizeof(addresses[0]);
         ++i)
    {
        uint8_t address = addresses[i];

        if (i2c_bus_probe(address) != ESP_OK)
        {
            continue;
        }

        esp_err_t err = bmp280_init(
            &manager->sensor,
            address);

        if (err == ESP_OK)
        {
            vTaskDelay(pdMS_TO_TICKS(100));

            manager->ready = true;

            printf("BMP280 ready at 0x%02X\n", address);
            return ESP_OK;
        }

        printf(
            "BMP280 init failed at 0x%02X: %s\n",
            address,
            esp_err_to_name(err));

        memset(&manager->sensor, 0, sizeof(manager->sensor));
    }

    printf("BMP280 unavailable\n");
    return ESP_ERR_NOT_FOUND;
}
