
#include <stdio.h>

#include "esp_err.h"
#include "i2c_bus.h"
#include "sensor_manager.h"
#include "hil_protocol.h"

void app_main(void)
{
    printf("\n");
    printf("I2C Lab 02 - Pass 9.5: Sensor Recovery\n");
    printf("--------------------------------------\n");

    sensor_manager_t manager;
    sensor_manager_init(&manager);

    esp_err_t err = i2c_bus_init();

    if (err != ESP_OK)
    {
        printf(
            "I2C initialization failed: %s\n",
            esp_err_to_name(err));
        return;
    }

    printf("I2C controller initialized\n");
    printf("SDA = GPIO%d\n", I2C_MASTER_SDA_IO);
    printf("SCL = GPIO%d\n", I2C_MASTER_SCL_IO);

    err = sensor_manager_reinit(&manager);

    if (err != ESP_OK)
    {
        printf("Sensor unavailable at startup\n");
    }

    err = hil_protocol_init();

    if (err != ESP_OK)
    {
        printf(
            "HIL UART initialization failed: %s\n",
            esp_err_to_name(err));

        i2c_bus_deinit();
        return;
    }

    printf("HIL protocol initialized\n");

    // This loop normally never returns.
    hil_protocol_run(&manager);
}
