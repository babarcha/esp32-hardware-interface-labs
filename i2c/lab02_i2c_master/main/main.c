/*
 * I2C Lab 02 — ESP32 I2C Master
 *
 * Demonstrates initialization and address scanning through the reusable
 * i2c_bus module. No external target device is required for this baseline.
 * Device-specific transactions are intentionally deferred to a later lab pass.
 */
#include <stdio.h>

#include "esp_err.h"
#include "i2c_bus.h"

void app_main(void)
{
    printf("\n");
    printf("I2C Lab 02 - Pass 5: Bus Abstraction\n");
    printf("------------------------------------\n");

    /* Establish the I2C master bus before attempting any address probes. */
    esp_err_t result = i2c_bus_init();

    if (result != ESP_OK)
    {
        printf(
            "I2C initialization failed: %s\n",
            esp_err_to_name(result));

        /* A failed initialization leaves no usable bus for scanning. */
        return;
    }

    /* Scan usable 7-bit addresses and count targets that acknowledge. */
    int devices_found = i2c_bus_scan();

    printf(
        "\nApplication: %d I2C device(s) detected\n",
        devices_found);
}
