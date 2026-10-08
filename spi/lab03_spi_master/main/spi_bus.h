/*
 * SPI Lab 03 — public SPI master abstraction API.
 *
 * A single SPI3_HOST bus supports separately registered device handles.
 * All operations return ESP-IDF status codes; transaction sizes are bytes.
 */
#ifndef SPI_BUS_H
#define SPI_BUS_H

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "driver/spi_master.h"

/*
 * Initialize SPI3_HOST with MOSI=GPIO23, MISO=GPIO19, SCLK=GPIO18.
 * Returns ESP_ERR_INVALID_STATE if this module already owns the bus.
 */
esp_err_t spi_bus_init(void);

/* Release the bus after unregistering all devices. */
esp_err_t spi_bus_deinit(void);

/*
 * Register a target on the initialized bus.
 * cs_gpio: target chip-select pin.
 * clock_speed_hz: positive target clock frequency.
 * mode: SPI mode 0-3 (CPOL/CPHA).
 * device_handle: pointer to a NULL handle, populated on success.
 */
esp_err_t spi_device_register(
    int cs_gpio,
    int clock_speed_hz,
    uint8_t mode,
    spi_device_handle_t *device_handle);

/* Unregister a device before releasing the bus. */
esp_err_t spi_device_unregister(
    spi_device_handle_t device_handle);

/* Transmit data_length bytes on MOSI using a blocking transaction. */
esp_err_t spi_bus_write(
    spi_device_handle_t device_handle,
    const uint8_t *tx_data,
    size_t data_length);

/* Receive data_length bytes on MISO while generating SCLK. */
esp_err_t spi_bus_read(
    spi_device_handle_t device_handle,
    uint8_t *rx_data,
    size_t data_length);

/* Simultaneously transmit and receive data_length bytes (full duplex). */
esp_err_t spi_bus_transfer(
    spi_device_handle_t device_handle,
    const uint8_t *tx_data,
    uint8_t *rx_data,
    size_t data_length);

#endif
