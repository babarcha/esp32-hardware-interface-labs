
#ifndef BMP280_SPI_H
#define BMP280_SPI_H

#include <stdint.h>

#include "esp_err.h"
#include "driver/spi_master.h"

#define BMP280_CHIP_ID_REG 0xD0
#define BMP280_EXPECTED_ID 0x58

typedef struct
{
    spi_device_handle_t spi_device;
} bmp280_spi_t;

/*
 * Bind a previously registered SPI device to
 * a BMP280 driver context.
 *
 * Does not take ownership of the SPI device.
 */
esp_err_t bmp280_spi_init(
    bmp280_spi_t *sensor,
    spi_device_handle_t device);

/*
 * Read one BMP280 register over four-wire SPI.
 */
esp_err_t bmp280_spi_read_register(
    const bmp280_spi_t *sensor,
    uint8_t register_address,
    uint8_t *value);

/*
 * Read the BMP280 chip identification register.
 */
esp_err_t bmp280_spi_read_chip_id(
    const bmp280_spi_t *sensor,
    uint8_t *chip_id);

/*
 * Check that the physical device is a BMP280.
 *
 * Returns ESP_ERR_INVALID_RESPONSE when the
 * chip ID differs from 0x58.
 */
esp_err_t bmp280_spi_verify(
    const bmp280_spi_t *sensor);

#endif
