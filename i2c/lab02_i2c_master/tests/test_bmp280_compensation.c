
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "bmp280_compensation.h"

static bmp280_calibration_t reference_calibration(void)
{
    return (bmp280_calibration_t){
        .dig_T1 = 27504,
        .dig_T2 = 26435,
        .dig_T3 = -1000,
        .dig_P1 = 36477,
        .dig_P2 = -10685,
        .dig_P3 = 3024,
        .dig_P4 = 2855,
        .dig_P5 = 140,
        .dig_P6 = -7,
        .dig_P7 = 15500,
        .dig_P8 = -14600,
        .dig_P9 = 6000};
}

static void test_valid_calibration(void)
{
    bmp280_calibration_t cal =
        reference_calibration();

    assert(bmp280_calibration_valid(&cal));

    cal.dig_P1 = 0;
    assert(!bmp280_calibration_valid(&cal));

    assert(!bmp280_calibration_valid(NULL));

    printf("PASS: calibration validation\n");
}

static void test_temperature(void)
{
    bmp280_calibration_t cal =
        reference_calibration();

    bmp280_compensation_state_t state = {0};

    int32_t temperature =
        bmp280_compensate_temperature(
            &cal, &state, 519888);

    assert(temperature == 2508);
    assert(state.t_fine == 128422);

    printf("PASS: temperature = %.2f C\n",
           temperature / 100.0);
}

static void test_pressure(void)
{
    bmp280_calibration_t cal =
        reference_calibration();

    bmp280_compensation_state_t state = {0};

    (void)bmp280_compensate_temperature(
        &cal, &state, 519888);

    uint32_t pressure =
        bmp280_compensate_pressure(
            &cal, &state, 415148);

    // Q24.8 pressure in Pa.
    assert(pressure == 25767233U);

    printf("PASS: pressure = %.2f hPa\n",
           pressure / 25600.0);
}

static void test_invalid_inputs(void)
{
    bmp280_calibration_t cal =
        reference_calibration();

    bmp280_compensation_state_t state = {0};

    assert(bmp280_compensate_temperature(
               NULL, &state, 519888) == 0);

    assert(bmp280_compensate_temperature(
               &cal, NULL, 519888) == 0);

    assert(bmp280_compensate_pressure(
               NULL, &state, 415148) == 0);

    assert(bmp280_compensate_pressure(
               &cal, NULL, 415148) == 0);

    printf("PASS: invalid input handling\n");
}

int main(void)
{
    printf("BMP280 Native Unit Tests\n");
    printf("------------------------\n");

    test_valid_calibration();
    test_temperature();
    test_pressure();
    test_invalid_inputs();

    printf("\nAll BMP280 tests passed!\n");

    return 0;
}
