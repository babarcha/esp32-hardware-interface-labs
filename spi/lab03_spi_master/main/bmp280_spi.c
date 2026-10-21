
/*
 * SPI Lab 03 — Pass 10.3
 * BMP280 SPI measurement driver.
 *
 * Uses Bosch BMP280 integer compensation formulas.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bmp280_spi.h"
#include "spi_bus.h"

#define BMP280_CALIB_REG 0x88
#define BMP280_STATUS_REG 0xF3
#define BMP280_CTRL_REG 0xF4
#define BMP280_DATA_REG 0xF7

#define BMP280_STATUS_BUSY 0x08
#define BMP280_STATUS_IMUPD 0x01

/* osrs_t=x1, osrs_p=x1, forced mode */
#define BMP280_FORCED_CONFIG 0x25

#define BMP280_MAX_BURST 24
#define BMP280_POLL_COUNT 100

static uint16_t get_u16(const uint8_t *p)
{
    return (uint16_t)p[0] |
           ((uint16_t)p[1] << 8);
}

static int16_t get_s16(const uint8_t *p)
{
    return (int16_t)get_u16(p);
}

esp_err_t bmp280_spi_init(
    bmp280_spi_t *sensor,
    spi_device_handle_t device)
{
    if (sensor == NULL || device == NULL)
        return ESP_ERR_INVALID_ARG;

    memset(sensor, 0, sizeof(*sensor));
    sensor->spi_device = device;

    return ESP_OK;
}

esp_err_t bmp280_spi_read_registers(
    const bmp280_spi_t *sensor,
    uint8_t address,
    uint8_t *data,
    size_t length)
{
    if (sensor == NULL ||
        sensor->spi_device == NULL ||
        data == NULL ||
        length == 0 ||
        length > BMP280_MAX_BURST)
        return ESP_ERR_INVALID_ARG;

    uint8_t tx[BMP280_MAX_BURST + 1] = {0};
    uint8_t rx[BMP280_MAX_BURST + 1] = {0};

    tx[0] = (address & 0x7F) | 0x80;

    esp_err_t err = spi_bus_transfer(
        sensor->spi_device,
        tx,
        rx,
        length + 1);

    if (err != ESP_OK)
        return err;

    memcpy(data, &rx[1], length);

    return ESP_OK;
}

esp_err_t bmp280_spi_read_register(
    const bmp280_spi_t *sensor,
    uint8_t address,
    uint8_t *value)
{
    return bmp280_spi_read_registers(
        sensor, address, value, 1);
}

esp_err_t bmp280_spi_write_register(
    const bmp280_spi_t *sensor,
    uint8_t address,
    uint8_t value)
{
    if (sensor == NULL || sensor->spi_device == NULL)
        return ESP_ERR_INVALID_ARG;

    uint8_t tx[2] = {
        address & 0x7F,
        value};

    return spi_bus_write(
        sensor->spi_device,
        tx,
        sizeof(tx));
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
    uint8_t id = 0;

    esp_err_t err = bmp280_spi_read_chip_id(
        sensor, &id);

    if (err != ESP_OK)
        return err;

    return (id == BMP280_EXPECTED_ID)
               ? ESP_OK
               : ESP_ERR_INVALID_RESPONSE;
}

esp_err_t bmp280_spi_load_calibration(
    bmp280_spi_t *sensor)
{
    if (sensor == NULL)
        return ESP_ERR_INVALID_ARG;

    sensor->calibration_loaded = false;

    uint8_t data[24] = {0};

    esp_err_t err = bmp280_spi_read_registers(
        sensor,
        BMP280_CALIB_REG,
        data,
        sizeof(data));

    if (err != ESP_OK)
        return err;

    bmp280_calibration_t *c = &sensor->calibration;

    c->dig_T1 = get_u16(&data[0]);
    c->dig_T2 = get_s16(&data[2]);
    c->dig_T3 = get_s16(&data[4]);

    c->dig_P1 = get_u16(&data[6]);
    c->dig_P2 = get_s16(&data[8]);
    c->dig_P3 = get_s16(&data[10]);
    c->dig_P4 = get_s16(&data[12]);
    c->dig_P5 = get_s16(&data[14]);
    c->dig_P6 = get_s16(&data[16]);
    c->dig_P7 = get_s16(&data[18]);
    c->dig_P8 = get_s16(&data[20]);
    c->dig_P9 = get_s16(&data[22]);

    /* dig_P1 must not be zero for pressure compensation. */
    if (c->dig_T1 == 0 || c->dig_P1 == 0 ||
        c->dig_T1 == UINT16_MAX ||
        c->dig_P1 == UINT16_MAX)
        return ESP_ERR_INVALID_RESPONSE;

    sensor->calibration_loaded = true;

    return ESP_OK;
}

/*
 * Bosch BMP280 temperature compensation.
 * Returns temperature in hundredths of a degree C.
 * Also produces t_fine for pressure compensation.
 */
static int32_t compensate_temperature(
    const bmp280_calibration_t *c,
    int32_t adc_t,
    int32_t *t_fine)
{
    int32_t var1 =
        ((((adc_t >> 3) -
           ((int32_t)c->dig_T1 << 1))) *
         (int32_t)c->dig_T2) >>
        11;

    int32_t delta =
        (adc_t >> 4) - (int32_t)c->dig_T1;

    int32_t var2 =
        (((delta * delta) >> 12) *
         (int32_t)c->dig_T3) >>
        14;

    *t_fine = var1 + var2;

    return (*t_fine * 5 + 128) >> 8;
}

/*
 * Bosch BMP280 64-bit pressure compensation.
 * Returns pressure in Q24.8 Pa.
 */
static uint32_t compensate_pressure(
    const bmp280_calibration_t *c,
    int32_t adc_p,
    int32_t t_fine)
{
    int64_t var1 = (int64_t)t_fine - 128000;
    int64_t var2 = var1 * var1 *
                   (int64_t)c->dig_P6;

    var2 += (var1 * (int64_t)c->dig_P5) << 17;
    var2 += ((int64_t)c->dig_P4) << 35;

    var1 = ((var1 * var1 *
             (int64_t)c->dig_P3) >>
            8) +
           ((var1 * (int64_t)c->dig_P2) << 12);

    var1 = (((((int64_t)1 << 47) + var1)) *
            (int64_t)c->dig_P1) >>
           33;

    if (var1 == 0)
        return 0;

    int64_t p = 1048576 - adc_p;

    p = (((p << 31) - var2) * 3125) / var1;

    var1 = ((int64_t)c->dig_P9 *
            (p >> 13) * (p >> 13)) >>
           25;

    var2 = ((int64_t)c->dig_P8 * p) >> 19;

    p = ((p + var1 + var2) >> 8) +
        (((int64_t)c->dig_P7) << 4);

    if (p <= 0 || p > UINT32_MAX)
        return 0;

    return (uint32_t)p;
}

esp_err_t bmp280_spi_measure(
    bmp280_spi_t *sensor,
    bmp280_measurement_t *measurement)
{
    if (sensor == NULL ||
        measurement == NULL ||
        !sensor->calibration_loaded)
        return ESP_ERR_INVALID_STATE;

    /*
     * Trigger a new forced measurement.
     */
    esp_err_t err = bmp280_spi_write_register(
        sensor,
        BMP280_CTRL_REG,
        BMP280_FORCED_CONFIG);

    if (err != ESP_OK)
        return err;

    /*
     * Wait for the measurement to finish.
     * The initial delay prevents accepting a
     * status sample before conversion starts.
     */
    vTaskDelay(pdMS_TO_TICKS(10));

    bool ready = false;

    for (int i = 0; i < BMP280_POLL_COUNT; ++i)
    {
        uint8_t status = 0;

        err = bmp280_spi_read_register(
            sensor,
            BMP280_STATUS_REG,
            &status);

        if (err != ESP_OK)
            return err;

        if ((status &
             (BMP280_STATUS_BUSY | BMP280_STATUS_IMUPD)) == 0)
        {
            ready = true;
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(2));
    }

    if (!ready)
        return ESP_ERR_TIMEOUT;

    /*
     * Pressure and temperature occupy six
     * consecutive bytes starting at 0xF7.
     */
    uint8_t data[6] = {0};

    err = bmp280_spi_read_registers(
        sensor,
        BMP280_DATA_REG,
        data,
        sizeof(data));

    if (err != ESP_OK)
        return err;

    int32_t adc_p =
        ((int32_t)data[0] << 12) |
        ((int32_t)data[1] << 4) |
        ((int32_t)data[2] >> 4);

    int32_t adc_t =
        ((int32_t)data[3] << 12) |
        ((int32_t)data[4] << 4) |
        ((int32_t)data[5] >> 4);

    /* All-one ADC values indicate invalid/skipped data. */
    if (adc_p == 0x80000 || adc_t == 0x80000)
        return ESP_ERR_INVALID_RESPONSE;

    int32_t t_fine = 0;

    int32_t temperature_x100 =
        compensate_temperature(
            &sensor->calibration,
            adc_t,
            &t_fine);

    uint32_t pressure_q24_8 =
        compensate_pressure(
            &sensor->calibration,
            adc_p,
            t_fine);

    if (pressure_q24_8 == 0)
        return ESP_ERR_INVALID_RESPONSE;

    measurement->temperature_c =
        temperature_x100 / 100.0f;

    /* Q24.8 Pa -> hPa */
    measurement->pressure_hpa =
        pressure_q24_8 / 25600.0f;

    return ESP_OK;
}
