#ifndef I2C_BUS_H
#define I2C_BUS_H

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "driver/i2c_master.h"

/*
 * Initialize the ESP32 I2C master bus.
 */
esp_err_t i2c_bus_init(void);

/*
 * Deinitialize the I2C master bus and release
 * its associated resources.
 */
esp_err_t i2c_bus_deinit(void);

/*
 * Probe a single 7-bit I2C address.
 */
esp_err_t i2c_bus_probe(uint8_t address);

/*
 * Scan usable 7-bit addresses (0x08-0x77).
 *
 * Returns the number of devices discovered.
 */
int i2c_bus_scan(void);

/*
 * Register an I2C device on the bus.
 */
esp_err_t i2c_bus_add_device(
    uint8_t address,
    uint32_t clock_speed_hz,
    i2c_master_dev_handle_t *device_handle);

/*
 * Remove a previously registered I2C device.
 */
esp_err_t i2c_bus_remove_device(
    i2c_master_dev_handle_t device_handle);

/*
 * Transmit bytes to a registered I2C device.
 */
esp_err_t i2c_bus_write(
    i2c_master_dev_handle_t device_handle,
    const uint8_t *data,
    size_t data_length);

/*
 * Receive bytes from a registered I2C device.
 */
esp_err_t i2c_bus_read(
    i2c_master_dev_handle_t device_handle,
    uint8_t *data,
    size_t data_length);

/*
 * Perform a write followed by a read.
 */
esp_err_t i2c_bus_write_read(
    i2c_master_dev_handle_t device_handle,
    const uint8_t *write_data,
    size_t write_length,
    uint8_t *read_data,
    size_t read_length);

#endif