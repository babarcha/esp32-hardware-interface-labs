
/*
 * SPI Lab 03 — Pass 10.1
 *
 * BMP280 SPI chip identification.
 *
 * Wiring:
 *   ESP32 GPIO18 -> BMP280 SCL  (SCLK)
 *   ESP32 GPIO23 -> BMP280 SDA  (MOSI)
 *   ESP32 GPIO19 -> BMP280 SDO  (MISO)
 *   ESP32 GPIO5  -> BMP280 CSB  (CS)
 *   ESP32 3V3    -> BMP280 VCC
 *   ESP32 GND    -> BMP280 GND
 */

#include <stdio.h>
#include <stdint.h>

#include "esp_err.h"
#include "spi_bus.h"

#define BMP280_CS_GPIO 5
#define BMP280_SPI_CLOCK_HZ 1000000
#define BMP280_SPI_MODE 0

#define BMP280_CHIP_ID_REG 0xD0
#define BMP280_EXPECTED_ID 0x58

static esp_err_t bmp280_read_chip_id(
    spi_device_handle_t device,
    uint8_t *chip_id)
{
    if (device == NULL || chip_id == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * BMP280 SPI:
     *   bit 7 = 1: read
     *   bits 6:0: register address
     *
     * Register 0xD0 becomes command 0xD0.
     *
     * The second byte generates clocks for
     * the sensor to return its chip ID.
     */
    uint8_t tx_data[2] = {
        (BMP280_CHIP_ID_REG & 0x7F) | 0x80,
        0x00};

    uint8_t rx_data[2] = {0};

    esp_err_t result = spi_bus_transfer(
        device,
        tx_data,
        rx_data,
        sizeof(tx_data));

    if (result != ESP_OK)
    {
        return result;
    }

    /*
     * RX[0] corresponds to the command phase.
     * RX[1] contains the requested register value.
     */
    *chip_id = rx_data[1];

    return ESP_OK;
}

void app_main(void)
{
    printf("\n");
    printf("SPI Lab 03 - Pass 10.1: BMP280 Chip ID\n");
    printf("-------------------------------------\n");

    esp_err_t result = spi_bus_init();

    if (result != ESP_OK)
    {
        printf(
            "SPI bus initialization failed: %s\n",
            esp_err_to_name(result));
        return;
    }

    printf("SPI bus initialized successfully\n");

    spi_device_handle_t bmp280 = NULL;

    result = spi_device_register(
        BMP280_CS_GPIO,
        BMP280_SPI_CLOCK_HZ,
        BMP280_SPI_MODE,
        &bmp280);

    if (result != ESP_OK)
    {
        printf(
            "BMP280 registration failed: %s\n",
            esp_err_to_name(result));

        spi_bus_deinit();
        return;
    }

    printf("BMP280 SPI device registered\n");

    uint8_t chip_id = 0;

    result = bmp280_read_chip_id(
        bmp280,
        &chip_id);

    if (result == ESP_OK)
    {
        printf("BMP280 chip ID: 0x%02X\n", chip_id);

        if (chip_id == BMP280_EXPECTED_ID)
        {
            printf("BMP280 SPI verification: PASS\n");
        }
        else
        {
            printf(
                "BMP280 SPI verification: FAIL "
                "(expected 0x%02X)\n",
                BMP280_EXPECTED_ID);
        }
    }
    else
    {
        printf(
            "BMP280 SPI transaction failed: %s\n",
            esp_err_to_name(result));
    }

    result = spi_device_unregister(bmp280);

    if (result != ESP_OK)
    {
        printf(
            "SPI device cleanup failed: %s\n",
            esp_err_to_name(result));
        return;
    }

    result = spi_bus_deinit();

    if (result != ESP_OK)
    {
        printf(
            "SPI bus cleanup failed: %s\n",
            esp_err_to_name(result));
    }
    else
    {
        printf("SPI resources released\n");
    }
}
