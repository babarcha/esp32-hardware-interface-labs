
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

    i2c_master_bus_config_t config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    return i2c_new_master_bus(&config, &s_bus_handle);
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

    printf("\nScanning I2C bus...\n");

    for (uint8_t addr = 0x08; addr <= 0x77; addr++)
    {
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

    printf("Scan complete\n");
    printf("Addresses checked : 112\n");
    printf("Devices found     : %d\n", count);

    return ESP_OK;
}

static esp_err_t add_device(
    uint8_t address,
    i2c_master_dev_handle_t *device)
{
    if (s_bus_handle == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (address > 0x7F || device == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_device_config_t config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    return i2c_master_bus_add_device(
        s_bus_handle, &config, device);
}

esp_err_t i2c_bus_read_register(
    uint8_t device_addr,
    uint8_t reg_addr,
    uint8_t *data,
    size_t len)
{
    if (data == NULL || len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_master_dev_handle_t device = NULL;

    esp_err_t err = add_device(device_addr, &device);

    if (err != ESP_OK)
    {
        return err;
    }

    err = i2c_master_transmit_receive(
        device,
        &reg_addr,
        1,
        data,
        len,
        I2C_MASTER_TIMEOUT_MS);

    esp_err_t cleanup = i2c_master_bus_rm_device(device);

    return (err != ESP_OK) ? err : cleanup;
}

esp_err_t i2c_bus_write_register(
    uint8_t device_addr,
    uint8_t reg_addr,
    uint8_t value)
{
    i2c_master_dev_handle_t device = NULL;

    esp_err_t err = add_device(device_addr, &device);

    if (err != ESP_OK)
    {
        return err;
    }

    uint8_t buffer[2] = {reg_addr, value};

    err = i2c_master_transmit(
        device,
        buffer,
        sizeof(buffer),
        I2C_MASTER_TIMEOUT_MS);

    esp_err_t cleanup = i2c_master_bus_rm_device(device);

    return (err != ESP_OK) ? err : cleanup;
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
