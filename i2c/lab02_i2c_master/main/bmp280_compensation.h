
#ifndef BMP280_COMPENSATION_H
#define BMP280_COMPENSATION_H

#include <stdint.h>
#include <stdbool.h>

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
    int32_t t_fine;
} bmp280_compensation_state_t;

bool bmp280_calibration_valid(
    const bmp280_calibration_t *cal);

int32_t bmp280_compensate_temperature(
    const bmp280_calibration_t *cal,
    bmp280_compensation_state_t *state,
    int32_t adc_T);

uint32_t bmp280_compensate_pressure(
    const bmp280_calibration_t *cal,
    const bmp280_compensation_state_t *state,
    int32_t adc_P);

#endif
