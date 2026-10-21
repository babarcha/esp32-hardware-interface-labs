
#ifndef BMP280_SPI_H
#define BMP280_SPI_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"
#include "driver/spi_master.h"

#define BMP280_CHIP_ID_REG 0xD0
#define BMP280_EXPECTED_ID 0x58

typedef struct
{
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;

    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
} bmp280_calibration_t;

typedef struct
{
    float temperature_c;
    float pressure_hpa;
} bmp280_measurement_t;

typedef struct
{
    spi_device_handle_t spi_device;
    bmp280_calibration_t calibration;
    bool calibration_loaded;
} bmp280_spi_t;

/* Attach an existing SPI device handle. */
esp_err_t bmp280_spi_init(
    bmp280_spi_t *sensor,
    spi_device_handle_t device);

/* Read one register. */
esp_err_t bmp280_spi_read_register(
    const bmp280_spi_t *sensor,
    uint8_t address,
    uint8_t *value);

/* Read consecutive registers in one transaction. */
esp_err_t bmp280_spi_read_registers(
    const bmp280_spi_t *sensor,
    uint8_t address,
    uint8_t *data,
    size_t length);

/* Write one register. */
esp_err_t bmp280_spi_write_register(
    const bmp280_spi_t *sensor,
    uint8_t address,
    uint8_t value);

/* Identify and verify the sensor. */
esp_err_t bmp280_spi_read_chip_id(
    const bmp280_spi_t *sensor,
    uint8_t *chip_id);

esp_err_t bmp280_spi_verify(
    const bmp280_spi_t *sensor);

/* Load factory calibration coefficients. */
esp_err_t bmp280_spi_load_calibration(
    bmp280_spi_t *sensor);

/* Trigger, read, and compensate one measurement. */
esp_err_t bmp280_spi_measure(
    bmp280_spi_t *sensor,
    bmp280_measurement_t *measurement);

#endif
