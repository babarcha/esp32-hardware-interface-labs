#ifndef I2C_BUS_H
#define I2C_BUS_H

/*
 * I2C Lab 02 — Public bus abstraction API
 *
 * Encapsulates ESP-IDF I2C master bus management and blocking transactions.
 * Call i2c_bus_init() before probing or registering devices. A registered
 * device handle is required for read/write operations. Callers own device
 * handles and should remove devices before deinitializing the bus.
 */
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "driver/i2c_master.h"

/* Initialize I2C0 as master. Returns ESP_ERR_INVALID_STATE if already active. */
esp_err_t i2c_bus_init(void);

/* Release the master bus; returns ESP_ERR_INVALID_STATE if not initialized. */
esp_err_t i2c_bus_deinit(void);

/* Probe a 7-bit target address; ESP_OK means a target acknowledged. */
esp_err_t i2c_bus_probe(uint8_t address);

/* Scan addresses 0x08–0x77 and return the number of responding targets. */
int i2c_bus_scan(void);

/* Register a target address and its SCL speed; write its handle to output. */
esp_err_t i2c_bus_add_device(
    uint8_t address,
    uint32_t clock_speed_hz,
    i2c_master_dev_handle_t *device_handle);

/* Remove a previously registered device handle. */
esp_err_t i2c_bus_remove_device(
    i2c_master_dev_handle_t device_handle);

/* Send a nonempty byte buffer to a registered target (blocking). */
esp_err_t i2c_bus_write(
    i2c_master_dev_handle_t device_handle,
    const uint8_t *data,
    size_t data_length);

/* Receive a nonempty byte buffer from a registered target (blocking). */
esp_err_t i2c_bus_read(
    i2c_master_dev_handle_t device_handle,
    uint8_t *data,
    size_t data_length);

/* Send bytes then receive bytes in one combined transaction (blocking). */
esp_err_t i2c_bus_write_read(
    i2c_master_dev_handle_t device_handle,
    const uint8_t *write_data,
    size_t write_length,
    uint8_t *read_data,
    size_t read_length);

#endif
