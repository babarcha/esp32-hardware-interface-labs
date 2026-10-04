#include <stdio.h>

#include "esp_err.h"
#include "i2c_bus.h"

void app_main(void)
{
    printf("\n");
    printf("I2C Lab 02 - Pass 5: Bus Abstraction\n");
    printf("------------------------------------\n");

    esp_err_t result = i2c_bus_init();

    if (result != ESP_OK)
    {
        printf(
            "I2C initialization failed: %s\n",
            esp_err_to_name(result));

        return;
    }

    int devices_found = i2c_bus_scan();

    printf(
        "\nApplication: %d I2C device(s) detected\n",
        devices_found);
}