
#include "i2c_bus.h"

#include <stdio.h>
#include "driver/i2c_master.h"

static i2c_master_bus_handle_t s_bus_handle = NULL;

esp_err_t i2c_bus_init(void)
{
    if (s_bus_handle != NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags.enable_internal_pullup = true,
    };

    return i2c_new_master_bus(
        &bus_config,
        &s_bus_handle);
}

esp_err_t i2c_bus_probe(uint8_t address)
{
    if (s_bus_handle == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (address > 0x7F)
    {
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_master_probe(
        s_bus_handle,
        address,
        I2C_MASTER_TIMEOUT_MS);
}

esp_err_t i2c_bus_scan(int *devices_found)
{
    if (s_bus_handle == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (devices_found == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    int count = 0;
    int checked = 0;

    printf("\nScanning I2C bus...\n");

    for (uint8_t addr = 0x08; addr <= 0x77; addr++)
    {
        checked++;

        esp_err_t err = i2c_bus_probe(addr);

        if (err == ESP_OK)
        {
            printf("ACK  : device at 0x%02X\n", addr);
            count++;
        }
        else if (err != ESP_ERR_NOT_FOUND)
        {
            printf("WARN : probe 0x%02X: %s\n",
                   addr, esp_err_to_name(err));
        }
    }

    *devices_found = count;

    printf("\nScan complete\n");
    printf("Addresses checked : %d\n", checked);
    printf("Devices found     : %d\n", count);

    return ESP_OK;
}

esp_err_t i2c_bus_read_register(
    uint8_t device_addr,
    uint8_t reg_addr,
    uint8_t *data,
    size_t len)
{
    if (s_bus_handle == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (device_addr > 0x7F ||
        data == NULL ||
        len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = device_addr,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    i2c_master_dev_handle_t device_handle = NULL;

    esp_err_t err = i2c_master_bus_add_device(
        s_bus_handle,
        &device_config,
        &device_handle);

    if (err != ESP_OK)
    {
        return err;
    }

    // Write register address, repeated START,
    // then read the requested register data.
    err = i2c_master_transmit_receive(
        device_handle,
        &reg_addr,
        1,
        data,
        len,
        I2C_MASTER_TIMEOUT_MS);

    esp_err_t cleanup_err =
        i2c_master_bus_rm_device(device_handle);

    if (err != ESP_OK)
    {
        return err;
    }

    return cleanup_err;
}

esp_err_t i2c_bus_deinit(void)
{
    if (s_bus_handle == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = i2c_del_master_bus(s_bus_handle);

    if (err == ESP_OK)
    {
        s_bus_handle = NULL;
    }

    return err;
}
