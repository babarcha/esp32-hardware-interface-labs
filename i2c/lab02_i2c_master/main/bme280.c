
#include "bme280.h"
#include "i2c_bus.h"

#include <stddef.h>
#include <string.h>

static uint16_t read_u16_le(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static int16_t read_s16_le(const uint8_t *p)
{
    uint16_t value = read_u16_le(p);

    return (value & 0x8000U)
               ? (int16_t)((int32_t)value - 65536)
               : (int16_t)value;
}

esp_err_t bme280_read_chip_id(
    uint8_t address,
    uint8_t *chip_id)
{
    if (chip_id == NULL ||
        (address != BME280_I2C_ADDR_LOW &&
         address != BME280_I2C_ADDR_HIGH))
    {
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_bus_read_register(
        address,
        BME280_REG_CHIP_ID,
        chip_id,
        1);
}

sensor_type_t bme280_get_sensor_type(uint8_t chip_id)
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

const char *bme280_sensor_name(sensor_type_t type)
{
    switch (type)
    {
    case SENSOR_BMP280:
        return "BMP280";
    case SENSOR_BME280:
        return "BME280";
    default:
        return "Unknown";
    }
}

static esp_err_t bmp280_read_calibration(
    bmp280_t *sensor)
{
    uint8_t data[24];

    esp_err_t err = i2c_bus_read_register(
        sensor->address,
        BMP280_REG_CALIB,
        data,
        sizeof(data));

    if (err != ESP_OK)
    {
        return err;
    }

    bmp280_calibration_t *c = &sensor->calibration;

    c->dig_T1 = read_u16_le(&data[0]);
    c->dig_T2 = read_s16_le(&data[2]);
    c->dig_T3 = read_s16_le(&data[4]);

    c->dig_P1 = read_u16_le(&data[6]);
    c->dig_P2 = read_s16_le(&data[8]);
    c->dig_P3 = read_s16_le(&data[10]);
    c->dig_P4 = read_s16_le(&data[12]);
    c->dig_P5 = read_s16_le(&data[14]);
    c->dig_P6 = read_s16_le(&data[16]);
    c->dig_P7 = read_s16_le(&data[18]);
    c->dig_P8 = read_s16_le(&data[20]);
    c->dig_P9 = read_s16_le(&data[22]);

    // A zero dig_P1 would cause invalid pressure compensation.
    if (c->dig_P1 == 0)
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}

esp_err_t bmp280_init(
    bmp280_t *sensor,
    uint8_t address)
{
    if (sensor == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(sensor, 0, sizeof(*sensor));

    uint8_t chip_id = 0;

    esp_err_t err = bme280_read_chip_id(
        address, &chip_id);

    if (err != ESP_OK)
    {
        return err;
    }

    // Pass 7 is specifically for BMP280.
    if (chip_id != BMP280_CHIP_ID)
    {
        return ESP_ERR_NOT_SUPPORTED;
    }

    sensor->address = address;

    err = bmp280_read_calibration(sensor);

    if (err != ESP_OK)
    {
        return err;
    }

    // config: no filter, default standby configuration.
    err = i2c_bus_write_register(
        address,
        BMP280_REG_CONFIG,
        0x00);

    if (err != ESP_OK)
    {
        return err;
    }

    // ctrl_meas:
    // temperature oversampling x1 = 001
    // pressure oversampling x1    = 001
    // normal mode                = 11
    // binary 001 001 11 = 0x27
    err = i2c_bus_write_register(
        address,
        BMP280_REG_CTRL_MEAS,
        0x27);

    if (err != ESP_OK)
    {
        return err;
    }

    sensor->initialized = true;

    return ESP_OK;
}

esp_err_t bmp280_read_measurement(
    bmp280_t *sensor,
    bmp280_measurement_t *measurement)
{
    if (sensor == NULL || measurement == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!sensor->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t data[6];

    esp_err_t err = i2c_bus_read_register(
        sensor->address,
        BMP280_REG_PRESS_MSB,
        data,
        sizeof(data));

    if (err != ESP_OK)
    {
        return err;
    }

    int32_t adc_P =
        ((int32_t)data[0] << 12) |
        ((int32_t)data[1] << 4) |
        ((int32_t)data[2] >> 4);

    int32_t adc_T =
        ((int32_t)data[3] << 12) |
        ((int32_t)data[4] << 4) |
        ((int32_t)data[5] >> 4);

    // Detect skipped or unavailable measurements.
    if (adc_P == 0x80000 || adc_T == 0x80000)
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Temperature must be compensated first to calculate t_fine.
    int32_t temperature_100 =
        bmp280_compensate_temperature(
            &sensor->calibration,
            &sensor->compensation,
            adc_T);

    uint32_t pressure_q24_8 =
        bmp280_compensate_pressure(
            &sensor->calibration,
            &sensor->compensation,
            adc_P);
    if (pressure_q24_8 == 0)
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    measurement->temperature_c =
        temperature_100 / 100.0f;

    // Q24.8 Pa -> hPa: divide by 256 * 100.
    measurement->pressure_hpa =
        pressure_q24_8 / 25600.0f;

    return ESP_OK;
}
