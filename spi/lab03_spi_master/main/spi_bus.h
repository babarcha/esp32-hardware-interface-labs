#ifndef SPI_BUS_H
#define SPI_BUS_H

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "driver/spi_master.h"

/*
 * Initialize the physical SPI master bus.
 *
 * MOSI = GPIO23
 * MISO = GPIO19
 * SCLK = GPIO18
 * Host = SPI3_HOST
 */
esp_err_t spi_bus_init(void);

/*
 * Release the SPI master bus.
 *
 * All registered devices must be removed first.
 */
esp_err_t spi_bus_deinit(void);

/*
 * Register an SPI device.
 *
 * cs_gpio:
 *     Chip-select GPIO.
 *
 * clock_speed_hz:
 *     SPI clock frequency.
 *
 * mode:
 *     SPI mode 0-3.
 *
 * device_handle:
 *     Receives the ESP-IDF device handle.
 */
esp_err_t spi_device_register(
    int cs_gpio,
    int clock_speed_hz,
    uint8_t mode,
    spi_device_handle_t *device_handle);

/*
 * Remove a registered SPI device.
 */
esp_err_t spi_device_unregister(
    spi_device_handle_t device_handle);

/*
 * Transmit data over MOSI.
 */
esp_err_t spi_bus_write(
    spi_device_handle_t device_handle,
    const uint8_t *tx_data,
    size_t data_length);

/*
 * Receive data over MISO.
 *
 * The master still generates SCLK while receiving.
 */
esp_err_t spi_bus_read(
    spi_device_handle_t device_handle,
    uint8_t *rx_data,
    size_t data_length);

/*
 * Perform a simultaneous full-duplex SPI transfer.
 */
esp_err_t spi_bus_transfer(
    spi_device_handle_t device_handle,
    const uint8_t *tx_data,
    uint8_t *rx_data,
    size_t data_length);

#endif