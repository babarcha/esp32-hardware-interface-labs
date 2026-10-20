
#ifndef BME280_H
#define BME280_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#define BME280_I2C_ADDR_LOW 0x76
#define BME280_I2C_ADDR_HIGH 0x77

#define BME280_REG_CHIP_ID 0xD0
#define BMP280_REG_CALIB 0x88
#define BMP280_REG_CTRL_MEAS 0xF4
#define BMP280_REG_CONFIG 0xF5
#define BMP280_REG_PRESS_MSB 0xF7

#define BMP280_CHIP_ID 0x58
#define BME280_CHIP_ID 0x60

typedef enum
{
    SENSOR_UNKNOWN = 0,
    SENSOR_BMP280,
    SENSOR_BME280
} sensor_type_t;

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
    uint8_t address;
    bmp280_calibration_t calibration;
    int32_t t_fine;
    bool initialized;
} bmp280_t;

typedef struct
{
    float temperature_c;
    float pressure_hpa;
} bmp280_measurement_t;

esp_err_t bme280_read_chip_id(
    uint8_t device_addr,
    uint8_t *chip_id);

sensor_type_t bme280_get_sensor_type(uint8_t chip_id);
const char *bme280_sensor_name(sensor_type_t type);

esp_err_t bmp280_init(
    bmp280_t *sensor,
    uint8_t address);

esp_err_t bmp280_read_measurement(
    bmp280_t *sensor,
    bmp280_measurement_t *measurement);

#endif
