#include <stdio.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

#define I2C_SDA_GPIO 21
#define I2C_SCL_GPIO 22
#define I2C_GLITCH_FILTER 7

void app_main(void)
{
    printf("\n");
    printf("I2C Lab 02 - Pass 2\n");
    printf("-------------------\n");

    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = I2C_GLITCH_FILTER,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;

    esp_err_t result = i2c_new_master_bus(
        &bus_config,
        &bus_handle);

    if (result == ESP_OK)
    {
        printf("I2C controller initialized successfully\n");
        printf("SDA = GPIO%d\n", I2C_SDA_GPIO);
        printf("SCL = GPIO%d\n", I2C_SCL_GPIO);
    }
    else
    {
        printf(
            "I2C initialization failed: %s\n",
            esp_err_to_name(result));
    }
}