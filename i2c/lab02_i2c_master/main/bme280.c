
#include "bme280.h"
#include "i2c_bus.h"

esp_err_t bme280_read_chip_id(
    uint8_t device_addr,
    uint8_t *chip_id)
{
    if (chip_id == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (device_addr != BME280_I2C_ADDR_LOW &&
        device_addr != BME280_I2C_ADDR_HIGH)
    {
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_bus_read_register(
        device_addr,
        BME280_REG_CHIP_ID,
        chip_id,
        1);
}

sensor_type_t bme280_get_sensor_type(
    uint8_t chip_id)
{
    switch (chip_id)
    {
    case BMP280_CHIP_ID:
        return SENSOR_BMP280;

    case BME280_CHIP_ID:
        return SENSOR_BME280;

    default:
        return SENSOR_UNKNOWN;
    }
}

const char *bme280_sensor_name(
    sensor_type_t sensor_type)
{
    switch (sensor_type)
    {
    case SENSOR_BMP280:
        return "BMP280";

    case SENSOR_BME280:
        return "BME280";

    default:
        return "Unknown sensor";
    }
}
