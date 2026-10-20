
#include "bmp280_compensation.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * BMP280 Compensation Module
 *
 * Converts raw ADC readings into calibrated temperature
 * and pressure values using calibration coefficients
 * read from the BMP280 sensor.
 *
 * This module is hardware-independent:
 * - No ESP-IDF dependencies
 * - No I2C driver dependencies
 * - Can be tested natively on Windows/Linux
 *
 * Temperature output:
 *   Integer value in 0.01 degrees Celsius
 *   Example: 2508 = 25.08 C
 *
 * Pressure output:
 *   Unsigned Q24.8 fixed-point value in Pa
 *   Example: 25767233 / 256 = 100653.25 Pa
 *
 * Important:
 *   Temperature compensation updates t_fine.
 *   Pressure compensation uses the resulting t_fine.
 */

/* ---------------------------------------------------------
 * Validate BMP280 calibration coefficients
 * --------------------------------------------------------- */

bool bmp280_calibration_valid(const bmp280_calibration_t *cal)
{
    if (cal == NULL)
    {
        return false;
    }

    /*
     * dig_P1 is an unsigned pressure coefficient.
     * A zero value would cause division by zero
     * in the pressure compensation algorithm.
     */
    if (cal->dig_P1 == 0)
    {
        return false;
    }

    return true;
}

/* ---------------------------------------------------------
 * Temperature compensation
 * --------------------------------------------------------- */

int32_t bmp280_compensate_temperature(
    const bmp280_calibration_t *cal,
    bmp280_compensation_state_t *state,
    int32_t adc_T)
{
    if (!bmp280_calibration_valid(cal) || state == NULL)
    {
        return 0;
    }

    /*
     * Bosch BMP280 integer temperature compensation.
     *
     * Intermediate calculations generate t_fine,
     * which is required for pressure compensation.
     */

    int32_t var1;
    int32_t var2;

    var1 =
        ((((adc_T >> 3) -
           ((int32_t)cal->dig_T1 * 2)) *
          (int32_t)cal->dig_T2) >>
         11);

    int32_t delta =
        (adc_T >> 4) - (int32_t)cal->dig_T1;

    var2 =
        (((delta * delta) >> 12) *
         (int32_t)cal->dig_T3) >>
        14;

    state->t_fine = var1 + var2;

    /*
     * Result is temperature multiplied by 100.
     *
     * Example:
     * 2508 -> 25.08 degrees Celsius
     */
    return (state->t_fine * 5 + 128) >> 8;
}

/* ---------------------------------------------------------
 * Pressure compensation
 * --------------------------------------------------------- */

uint32_t bmp280_compensate_pressure(
    const bmp280_calibration_t *cal,
    const bmp280_compensation_state_t *state,
    int32_t adc_P)
{
    if (!bmp280_calibration_valid(cal) || state == NULL)
    {
        return 0;
    }

    /*
     * Bosch BMP280 64-bit integer pressure compensation.
     *
     * Signed left shifts have been replaced with
     * multiplication by powers of two where appropriate.
     *
     * The result uses Q24.8 fixed-point representation.
     */

    int64_t var1;
    int64_t var2;
    int64_t p;

    var1 = (int64_t)state->t_fine - 128000;

    var2 = var1 * var1 * (int64_t)cal->dig_P6;

    var2 +=
        (var1 * (int64_t)cal->dig_P5) *
        (INT64_C(1) << 17);

    var2 +=
        ((int64_t)cal->dig_P4) *
        (INT64_C(1) << 35);

    var1 =
        ((var1 * var1 * (int64_t)cal->dig_P3) >> 8) +
        ((var1 * (int64_t)cal->dig_P2) *
         (INT64_C(1) << 12));

    var1 =
        (((INT64_C(1) << 47) + var1) *
         (int64_t)cal->dig_P1) >>
        33;

    /*
     * Avoid division by zero.
     */
    if (var1 == 0)
    {
        return 0;
    }

    p = INT64_C(1048576) - (int64_t)adc_P;

    p = (((p << 31) - var2) * 3125) / var1;

    var1 =
        ((int64_t)cal->dig_P9 *
         (p >> 13) *
         (p >> 13)) >>
        25;

    var2 =
        ((int64_t)cal->dig_P8 * p) >> 19;

    p =
        ((p + var1 + var2) >> 8) +
        ((int64_t)cal->dig_P7 *
         (INT64_C(1) << 4));

    return (uint32_t)p;
}
