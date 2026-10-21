
/*
 * SPI Lab 03 — Pass 10.2
 *
 * BMP280 sensor-specific SPI driver.
 *
 * Uses the existing reusable spi_bus abstraction.
 */

#include <stddef.h>
#include <stdint.h>

#include "bmp280_spi.h"
#include "spi_bus.h"

#define BMP280_SPI_READ_BIT 0x80
#define BMP280_SPI_ADDR_MASK 0x7F

esp_err_t bmp280_spi_init(
    bmp280_spi_t *sensor,
    spi_device_handle_t device)
{
    if (sensor == NULL || device == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    sensor->spi_device = device;

    return ESP_OK;
}

esp_err_t bmp280_spi_read_register(
    const bmp280_spi_t *sensor,
    uint8_t register_address,
    uint8_t *value)
{
    if (sensor == NULL ||
        sensor->spi_device == NULL ||
        value == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * BMP280 SPI register protocol:
     *
     * Bit 7 = 1 for read operations.
     * Bits 6:0 select the register address.
     *
     * The second byte generates the clock
     * pulses needed to receive the data.
     */

    uint8_t command =
        (register_address & BMP280_SPI_ADDR_MASK) |
        BMP280_SPI_READ_BIT;

    uint8_t tx_data[2] = {
        command,
        0x00};

    uint8_t rx_data[2] = {0};

    esp_err_t result = spi_bus_transfer(
        sensor->spi_device,
        tx_data,
        rx_data,
        sizeof(tx_data));

    if (result != ESP_OK)
    {
        return result;
    }

    /*
     * The first received byte corresponds
     * to the address/command phase.
     *
     * The second received byte contains
     * the register value.
     */
    *value = rx_data[1];

    return ESP_OK;
}

esp_err_t bmp280_spi_read_chip_id(
    const bmp280_spi_t *sensor,
    uint8_t *chip_id)
{
    return bmp280_spi_read_register(
        sensor,
        BMP280_CHIP_ID_REG,
        chip_id);
}

esp_err_t bmp280_spi_verify(
    const bmp280_spi_t *sensor)
{
    uint8_t chip_id = 0;

    esp_err_t result = bmp280_spi_read_chip_id(
        sensor,
        &chip_id);

    if (result != ESP_OK)
    {
        return result;
    }

    if (chip_id != BMP280_EXPECTED_ID)
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}
