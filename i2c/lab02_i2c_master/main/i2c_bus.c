#include <stdio.h>
#include <stdint.h>

#include "i2c_bus.h"
#include "driver/i2c_master.h"

#define I2C_SDA_GPIO 21
#define I2C_SCL_GPIO 22
#define I2C_GLITCH_FILTER 7
#define I2C_PROBE_TIMEOUT_MS 50
#define I2C_TRANSFER_TIMEOUT_MS 100

static i2c_master_bus_handle_t bus_handle = NULL;

esp_err_t i2c_bus_init(void)
{
    /*
     * Prevent accidental double initialization.
     */
    if (bus_handle != NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = I2C_GLITCH_FILTER,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t result = i2c_new_master_bus(
        &bus_config,
        &bus_handle);

    if (result == ESP_OK)
    {
        printf("I2C controller initialized successfully\n");
        printf("SDA = GPIO%d\n", I2C_SDA_GPIO);
        printf("SCL = GPIO%d\n", I2C_SCL_GPIO);
    }

    return result;
}

esp_err_t i2c_bus_deinit(void)
{
    if (bus_handle == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t result = i2c_del_master_bus(bus_handle);

    if (result == ESP_OK)
    {
        bus_handle = NULL;
    }

    return result;
}

esp_err_t i2c_bus_probe(uint8_t address)
{
    if (bus_handle == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    return i2c_master_probe(
        bus_handle,
        address,
        I2C_PROBE_TIMEOUT_MS);
}

int i2c_bus_scan(void)
{
    int devices_found = 0;
    int addresses_checked = 0;

    printf("\nScanning I2C bus...\n");

    for (uint8_t address = 0x08; address <= 0x77; address++)
    {
        addresses_checked++;

        esp_err_t result = i2c_bus_probe(address);

        if (result == ESP_OK)
        {
            printf(
                "ACK  : device at 0x%02X\n",
                address);

            devices_found++;
        }
        else if (result == ESP_ERR_NOT_FOUND)
        {
            /*
             * Normal condition:
             * no target acknowledged this address.
             */
        }
        else
        {
            printf(
                "ERROR: address 0x%02X -> %s\n",
                address,
                esp_err_to_name(result));
        }
    }

    printf("\nScan complete\n");
    printf("Addresses checked : %d\n", addresses_checked);
    printf("Devices found     : %d\n", devices_found);

    return devices_found;
}

esp_err_t i2c_bus_add_device(
    uint8_t address,
    uint32_t clock_speed_hz,
    i2c_master_dev_handle_t *device_handle)
{
    if (bus_handle == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (device_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = clock_speed_hz,
    };

    return i2c_master_bus_add_device(
        bus_handle,
        &device_config,
        device_handle);
}

esp_err_t i2c_bus_remove_device(
    i2c_master_dev_handle_t device_handle)
{
    if (device_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_master_bus_rm_device(device_handle);
}

esp_err_t i2c_bus_write(
    i2c_master_dev_handle_t device_handle,
    const uint8_t *data,
    size_t data_length)
{
    if (device_handle == NULL ||
        data == NULL ||
        data_length == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_master_transmit(
        device_handle,
        data,
        data_length,
        I2C_TRANSFER_TIMEOUT_MS);
}

esp_err_t i2c_bus_read(
    i2c_master_dev_handle_t device_handle,
    uint8_t *data,
    size_t data_length)
{
    if (device_handle == NULL ||
        data == NULL ||
        data_length == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_master_receive(
        device_handle,
        data,
        data_length,
        I2C_TRANSFER_TIMEOUT_MS);
}

esp_err_t i2c_bus_write_read(
    i2c_master_dev_handle_t device_handle,
    const uint8_t *write_data,
    size_t write_length,
    uint8_t *read_data,
    size_t read_length)
{
    if (device_handle == NULL ||
        write_data == NULL ||
        write_length == 0 ||
        read_data == NULL ||
        read_length == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_master_transmit_receive(
        device_handle,
        write_data,
        write_length,
        read_data,
        read_length,
        I2C_TRANSFER_TIMEOUT_MS);
}