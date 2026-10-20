
#ifndef BME280_H
#define BME280_H

#include <stdint.h>
#include "esp_err.h"

#define BME280_I2C_ADDR_LOW 0x76
#define BME280_I2C_ADDR_HIGH 0x77

#define BME280_REG_CHIP_ID 0xD0

#define BMP280_CHIP_ID 0x58
#define BME280_CHIP_ID 0x60

typedef enum
{
    SENSOR_UNKNOWN = 0,
    SENSOR_BMP280,
    SENSOR_BME280
} sensor_type_t;

esp_err_t bme280_read_chip_id(
    uint8_t device_addr,
    uint8_t *chip_id);

sensor_type_t bme280_get_sensor_type(
    uint8_t chip_id);

const char *bme280_sensor_name(
    sensor_type_t sensor_type);

#endif
