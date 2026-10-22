
/*
 * SPI Lab 03 — Pass 10.8
 *
 * Host-native tests for production bmp280_spi.c.
 *
 * Coverage:
 *   - BMP280 initialization and chip identification
 *   - SPI register reads and writes
 *   - Calibration parsing and validation
 *   - Reference-vector temperature/pressure compensation
 *   - Sensor busy polling and timeout
 *   - SPI fault injection and error propagation
 *   - Invalid ADC detection and recovery
 *
 * No ESP32 hardware is required.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "bmp280_spi.h"
#include "spi_bus.h"

#define CHECK(expr)                             \
    do                                          \
    {                                           \
        if (!(expr))                            \
        {                                       \
            fprintf(stderr,                     \
                    "FAIL %s:%d: %s\n",         \
                    __FILE__, __LINE__, #expr); \
            exit(EXIT_FAILURE);                 \
        }                                       \
    } while (0)

#define TEST(fn)                  \
    do                            \
    {                             \
        fn();                     \
        tests_run++;              \
        printf("PASS %s\n", #fn); \
    } while (0)

static int tests_run = 0;

static int dummy_device;
#define DEVICE ((spi_device_handle_t) & dummy_device)

/* Mock SPI return codes. */
static esp_err_t mock_read_result = ESP_OK;
static esp_err_t mock_write_result = ESP_OK;

/* Simulated BMP280 register map. */
static uint8_t mock_registers[256];

/* Last write transaction. */
static uint8_t last_write_address;
static uint8_t last_write_value;

/* Counters for verification and fault injection. */
static int read_calls;
static int write_calls;
static int delay_calls;

/* Fail exactly one selected transaction. */
static int fail_read_on_call;
static int fail_write_on_call;

/* Simulated measurement-status behavior. */
static int busy_reads_remaining;
static int always_busy;

/* -----------------------------------------------------
 * Mock state management
 * ----------------------------------------------------- */

static void reset_mock(void)
{
    memset(mock_registers, 0, sizeof(mock_registers));

    mock_read_result = ESP_OK;
    mock_write_result = ESP_OK;

    last_write_address = 0;
    last_write_value = 0;

    read_calls = 0;
    write_calls = 0;
    delay_calls = 0;

    fail_read_on_call = 0;
    fail_write_on_call = 0;

    busy_reads_remaining = 0;
    always_busy = 0;

    /* Expected BMP280 chip identification. */
    mock_registers[BMP280_CHIP_ID_REG] =
        BMP280_EXPECTED_ID;
}

/* -----------------------------------------------------
 * Mock production SPI bus dependencies
 * ----------------------------------------------------- */

esp_err_t spi_bus_transfer(
    spi_device_handle_t device,
    const uint8_t *tx,
    uint8_t *rx,
    size_t length)
{
    CHECK(device == DEVICE);
    CHECK(tx != NULL);
    CHECK(rx != NULL);
    CHECK(length >= 2);

    read_calls++;

    if (fail_read_on_call > 0 &&
        read_calls == fail_read_on_call)
    {
        return ESP_FAIL;
    }

    if (mock_read_result != ESP_OK)
    {
        return mock_read_result;
    }

    /*
     * BMP280 SPI read command:
     * bit 7 set, followed by dummy clocks.
     *
     * BMP280 registers used by this driver
     * occupy the upper half of the address map.
     */
    uint8_t reg = tx[0];

    memset(rx, 0, length);

    /* rx[0] is the command-phase dummy byte. */
    for (size_t i = 1; i < length; i++)
    {
        uint8_t value = mock_registers[reg];

        if (reg == 0xF3 &&
            (always_busy || busy_reads_remaining > 0))
        {
            value = 0x08;

            if (!always_busy)
            {
                busy_reads_remaining--;
            }
        }

        rx[i] = value;
        reg++;
    }

    return ESP_OK;
}

esp_err_t spi_bus_write(
    spi_device_handle_t device,
    const uint8_t *tx,
    size_t length)
{
    CHECK(device == DEVICE);
    CHECK(tx != NULL);
    CHECK(length == 2);

    write_calls++;

    last_write_address = tx[0];
    last_write_value = tx[1];

    if (fail_write_on_call > 0 &&
        write_calls == fail_write_on_call)
    {
        return ESP_FAIL;
    }

    return mock_write_result;
}

/*
 * Native replacement for FreeRTOS vTaskDelay.
 * Record the call without sleeping.
 */
void vTaskDelay(uint32_t ticks)
{
    (void)ticks;
    delay_calls++;
}

/* -----------------------------------------------------
 * Sensor and register helpers
 * ----------------------------------------------------- */

static bmp280_spi_t new_sensor(void)
{
    bmp280_spi_t sensor = {0};

    CHECK(bmp280_spi_init(&sensor, DEVICE) == ESP_OK);

    return sensor;
}

static void set_u16(uint8_t address, uint16_t value)
{
    mock_registers[address] =
        (uint8_t)value;

    mock_registers[(uint8_t)(address + 1)] =
        (uint8_t)(value >> 8);
}

static void set_s16(uint8_t address, int16_t value)
{
    set_u16(address, (uint16_t)value);
}

/*
 * Minimal valid calibration for basic parsing tests.
 */
static void valid_calibration(void)
{
    set_u16(0x88, 27504);
    set_s16(0x8A, 26435);
    set_s16(0x8C, 50);
    set_u16(0x8E, 36477);
}

/*
 * BMP280 reference compensation coefficients.
 */
static void reference_calibration(void)
{
    set_u16(0x88, 27504); /* dig_T1 */
    set_s16(0x8A, 26435); /* dig_T2 */
    set_s16(0x8C, -1000); /* dig_T3 */

    set_u16(0x8E, 36477);  /* dig_P1 */
    set_s16(0x90, -10685); /* dig_P2 */
    set_s16(0x92, 3024);   /* dig_P3 */
    set_s16(0x94, 2855);   /* dig_P4 */
    set_s16(0x96, 140);    /* dig_P5 */
    set_s16(0x98, -7);     /* dig_P6 */
    set_s16(0x9A, 15500);  /* dig_P7 */
    set_s16(0x9C, -14600); /* dig_P8 */
    set_s16(0x9E, 6000);   /* dig_P9 */
}

/*
 * Encode a 20-bit ADC value into three BMP280
 * register bytes (MSB, LSB, XLSB).
 */
static void set_adc20(uint8_t address, uint32_t adc)
{
    CHECK(adc <= 0xFFFFF);

    mock_registers[address] =
        (uint8_t)(adc >> 12);

    mock_registers[(uint8_t)(address + 1)] =
        (uint8_t)(adc >> 4);

    mock_registers[(uint8_t)(address + 2)] =
        (uint8_t)((adc & 0x0F) << 4);
}

static void reference_adc(void)
{
    /* Pressure registers: F7, F8, F9. */
    set_adc20(0xF7, 415148);

    /* Temperature registers: FA, FB, FC. */
    set_adc20(0xFA, 519888);
}

static bmp280_spi_t calibrated_reference_sensor(void)
{
    reference_calibration();
    reference_adc();

    bmp280_spi_t sensor = new_sensor();

    CHECK(bmp280_spi_load_calibration(&sensor) == ESP_OK);

    CHECK(sensor.calibration_loaded);

    return sensor;
}

/* -----------------------------------------------------
 * Pass 10.7 regression tests
 * ----------------------------------------------------- */

static void test_init_validation(void)
{
    reset_mock();

    bmp280_spi_t sensor = {0};

    CHECK(bmp280_spi_init(NULL, DEVICE) == ESP_ERR_INVALID_ARG);

    CHECK(bmp280_spi_init(&sensor, NULL) == ESP_ERR_INVALID_ARG);

    CHECK(bmp280_spi_init(&sensor, DEVICE) == ESP_OK);

    CHECK(sensor.spi_device == DEVICE);
    CHECK(!sensor.calibration_loaded);
}

static void test_chip_id_success(void)
{
    reset_mock();

    bmp280_spi_t sensor = new_sensor();
    uint8_t id = 0;

    CHECK(bmp280_spi_read_chip_id(&sensor, &id) == ESP_OK);

    CHECK(id == BMP280_EXPECTED_ID);

    CHECK(bmp280_spi_verify(&sensor) == ESP_OK);
}

static void test_wrong_chip_id(void)
{
    reset_mock();

    bmp280_spi_t sensor = new_sensor();

    mock_registers[BMP280_CHIP_ID_REG] = 0x00;

    CHECK(bmp280_spi_verify(&sensor) == ESP_ERR_INVALID_RESPONSE);
}

static void test_spi_read_failure(void)
{
    reset_mock();

    bmp280_spi_t sensor = new_sensor();

    mock_read_result = ESP_FAIL;

    CHECK(bmp280_spi_verify(&sensor) == ESP_FAIL);
}

static void test_read_argument_validation(void)
{
    reset_mock();

    bmp280_spi_t sensor = new_sensor();
    uint8_t data[25] = {0};

    CHECK(bmp280_spi_read_registers(
              &sensor, 0x88, NULL, 1) == ESP_ERR_INVALID_ARG);

    CHECK(bmp280_spi_read_registers(
              &sensor, 0x88, data, 0) == ESP_ERR_INVALID_ARG);

    CHECK(bmp280_spi_read_registers(
              &sensor, 0x88, data, 25) == ESP_ERR_INVALID_ARG);
}

static void test_write_register(void)
{
    reset_mock();

    bmp280_spi_t sensor = new_sensor();

    CHECK(bmp280_spi_write_register(
              &sensor, 0xF4, 0x25) == ESP_OK);

    CHECK(write_calls == 1);
    CHECK(last_write_address == 0x74);
    CHECK(last_write_value == 0x25);
}

static void test_write_failure(void)
{
    reset_mock();

    bmp280_spi_t sensor = new_sensor();

    mock_write_result = ESP_FAIL;

    CHECK(bmp280_spi_write_register(
              &sensor, 0xF4, 0x25) == ESP_FAIL);
}

static void test_calibration_success(void)
{
    reset_mock();
    valid_calibration();

    bmp280_spi_t sensor = new_sensor();

    CHECK(bmp280_spi_load_calibration(&sensor) == ESP_OK);

    CHECK(sensor.calibration_loaded);

    CHECK(sensor.calibration.dig_T1 == 27504);
    CHECK(sensor.calibration.dig_P1 == 36477);
}

static void test_calibration_failure(void)
{
    reset_mock();

    bmp280_spi_t sensor = new_sensor();

    CHECK(bmp280_spi_load_calibration(&sensor) == ESP_ERR_INVALID_RESPONSE);

    CHECK(!sensor.calibration_loaded);
}

static void test_calibration_bus_failure(void)
{
    reset_mock();

    bmp280_spi_t sensor = new_sensor();

    mock_read_result = ESP_FAIL;

    CHECK(bmp280_spi_load_calibration(&sensor) == ESP_FAIL);

    CHECK(!sensor.calibration_loaded);
}

static void test_measurement_requires_calibration(void)
{
    reset_mock();

    bmp280_spi_t sensor = new_sensor();
    bmp280_measurement_t result = {0};

    CHECK(bmp280_spi_measure(&sensor, &result) == ESP_ERR_INVALID_STATE);
}

static void test_measurement_write_failure(void)
{
    reset_mock();
    valid_calibration();

    bmp280_spi_t sensor = new_sensor();
    bmp280_measurement_t result = {0};

    CHECK(bmp280_spi_load_calibration(&sensor) == ESP_OK);

    mock_write_result = ESP_FAIL;

    CHECK(bmp280_spi_measure(&sensor, &result) == ESP_FAIL);
}

/* -----------------------------------------------------
 * Pass 10.8 reference-vector and fault tests
 * ----------------------------------------------------- */

static void test_reference_compensation(void)
{
    reset_mock();

    bmp280_spi_t sensor =
        calibrated_reference_sensor();

    bmp280_measurement_t result = {0};

    CHECK(bmp280_spi_measure(&sensor, &result) == ESP_OK);

    /*
     * Reference: approximately 25.08 C,
     * 1006.53 hPa.
     */
    CHECK(result.temperature_c > 25.07f);
    CHECK(result.temperature_c < 25.09f);

    CHECK(result.pressure_hpa > 1006.48f);
    CHECK(result.pressure_hpa < 1006.58f);

    /* Forced mode, x1 temperature and pressure. */
    CHECK(last_write_address == 0x74);
    CHECK(last_write_value == 0x25);

    CHECK(delay_calls >= 1);
}

static void test_sensor_busy_then_ready(void)
{
    reset_mock();

    bmp280_spi_t sensor =
        calibrated_reference_sensor();

    bmp280_measurement_t result = {0};

    busy_reads_remaining = 3;

    CHECK(bmp280_spi_measure(&sensor, &result) == ESP_OK);

    /* Initial delay + 3 busy-poll delays. */
    CHECK(delay_calls >= 4);
}

static void test_sensor_busy_timeout(void)
{
    reset_mock();

    bmp280_spi_t sensor =
        calibrated_reference_sensor();

    bmp280_measurement_t result = {0};

    always_busy = 1;

    CHECK(bmp280_spi_measure(&sensor, &result) == ESP_ERR_TIMEOUT);

    /* One initial delay plus polling delays. */
    CHECK(delay_calls >= 100);
}

static void test_status_read_failure(void)
{
    reset_mock();

    bmp280_spi_t sensor =
        calibrated_reference_sensor();

    bmp280_measurement_t result = {0};

    /* Fail the next SPI read: status register. */
    fail_read_on_call = read_calls + 1;

    CHECK(bmp280_spi_measure(&sensor, &result) == ESP_FAIL);
}

static void test_measurement_data_read_failure(void)
{
    reset_mock();

    bmp280_spi_t sensor =
        calibrated_reference_sensor();

    bmp280_measurement_t result = {0};

    /*
     * First read: status.
     * Second read: pressure/temperature data.
     */
    fail_read_on_call = read_calls + 2;

    CHECK(bmp280_spi_measure(&sensor, &result) == ESP_FAIL);
}

static void test_invalid_adc(void)
{
    reset_mock();

    bmp280_spi_t sensor =
        calibrated_reference_sensor();

    bmp280_measurement_t result = {0};

    /* Invalid/skipped pressure ADC marker. */
    set_adc20(0xF7, 0x80000);

    CHECK(bmp280_spi_measure(&sensor, &result) == ESP_ERR_INVALID_RESPONSE);
}

static void test_recovery_after_spi_failure(void)
{
    reset_mock();

    bmp280_spi_t sensor =
        calibrated_reference_sensor();

    bmp280_measurement_t result = {0};

    /* First measurement encounters a read failure. */
    fail_read_on_call = read_calls + 1;

    CHECK(bmp280_spi_measure(&sensor, &result) == ESP_FAIL);

    /* Restore normal communication. */
    fail_read_on_call = 0;

    CHECK(bmp280_spi_measure(&sensor, &result) == ESP_OK);

    CHECK(result.temperature_c > 25.07f);
    CHECK(result.temperature_c < 25.09f);
}

/* -----------------------------------------------------
 * Pass 10.11 — Fault injection and data integrity
 * ----------------------------------------------------- */

/*
 * Reject all four invalid calibration conditions.
 * Calibration must remain marked as unavailable.
 */
static void test_invalid_calibration_boundaries(void)
{
    const uint16_t invalid_values[] = {
        0,
        UINT16_MAX};

    for (size_t i = 0; i < 2; ++i)
    {
        /* Invalid temperature coefficient. */
        reset_mock();
        valid_calibration();

        set_u16(0x88, invalid_values[i]);

        bmp280_spi_t sensor = new_sensor();

        CHECK(bmp280_spi_load_calibration(&sensor) ==
              ESP_ERR_INVALID_RESPONSE);

        CHECK(!sensor.calibration_loaded);

        /* Invalid pressure coefficient. */
        reset_mock();
        valid_calibration();

        set_u16(0x8E, invalid_values[i]);

        sensor = new_sensor();

        CHECK(bmp280_spi_load_calibration(&sensor) ==
              ESP_ERR_INVALID_RESPONSE);

        CHECK(!sensor.calibration_loaded);
    }
}

/*
 * Reject invalid temperature ADC values.
 * The existing Pass 10.8 test covers pressure.
 */
static void test_invalid_temperature_adc(void)
{
    reset_mock();

    bmp280_spi_t sensor =
        calibrated_reference_sensor();

    bmp280_measurement_t result = {0};

    set_adc20(0xFA, 0x80000);

    CHECK(bmp280_spi_measure(&sensor, &result) ==
          ESP_ERR_INVALID_RESPONSE);
}

/*
 * Verify that failed measurements do not overwrite
 * a previously valid result.
 */
static void test_measurement_preserves_output_on_failure(void)
{
    reset_mock();

    bmp280_spi_t sensor =
        calibrated_reference_sensor();

    bmp280_measurement_t result = {
        .temperature_c = 42.5f,
        .pressure_hpa = 999.0f};

    /* Inject a failure during status-register reading. */
    fail_read_on_call = read_calls + 1;

    CHECK(bmp280_spi_measure(&sensor, &result) ==
          ESP_FAIL);

    CHECK(result.temperature_c == 42.5f);
    CHECK(result.pressure_hpa == 999.0f);

    /* Restore communication and verify recovery. */
    fail_read_on_call = 0;

    CHECK(bmp280_spi_measure(&sensor, &result) ==
          ESP_OK);

    CHECK(result.temperature_c > 25.07f);
    CHECK(result.temperature_c < 25.09f);
}

/*
 * A failed calibration reload must invalidate
 * previously loaded calibration.
 */
static void test_calibration_reload_failure(void)
{
    reset_mock();

    bmp280_spi_t sensor =
        calibrated_reference_sensor();

    CHECK(sensor.calibration_loaded);

    /* Simulate an SPI failure during recalibration. */
    fail_read_on_call = read_calls + 1;

    CHECK(bmp280_spi_load_calibration(&sensor) ==
          ESP_FAIL);

    CHECK(!sensor.calibration_loaded);

    bmp280_measurement_t result = {0};

    CHECK(bmp280_spi_measure(&sensor, &result) ==
          ESP_ERR_INVALID_STATE);

    /* Restore the bus and reload calibration. */
    fail_read_on_call = 0;

    CHECK(bmp280_spi_load_calibration(&sensor) ==
          ESP_OK);

    CHECK(sensor.calibration_loaded);

    CHECK(bmp280_spi_measure(&sensor, &result) ==
          ESP_OK);
}

/*
 * Exercise NULL sensor and NULL output validation.
 */
static void test_measurement_null_arguments(void)
{
    reset_mock();

    bmp280_spi_t sensor =
        calibrated_reference_sensor();

    bmp280_measurement_t result = {0};

    CHECK(bmp280_spi_measure(NULL, &result) ==
          ESP_ERR_INVALID_STATE);

    CHECK(bmp280_spi_measure(&sensor, NULL) ==
          ESP_ERR_INVALID_STATE);
}

/* -----------------------------------------------------
 * Test runner
 * ----------------------------------------------------- */

int main(void)
{
    TEST(test_init_validation);
    TEST(test_chip_id_success);
    TEST(test_wrong_chip_id);
    TEST(test_spi_read_failure);
    TEST(test_read_argument_validation);
    TEST(test_write_register);
    TEST(test_write_failure);
    TEST(test_calibration_success);
    TEST(test_calibration_failure);
    TEST(test_calibration_bus_failure);
    TEST(test_measurement_requires_calibration);
    TEST(test_measurement_write_failure);

    TEST(test_reference_compensation);
    TEST(test_sensor_busy_then_ready);
    TEST(test_sensor_busy_timeout);
    TEST(test_status_read_failure);
    TEST(test_measurement_data_read_failure);
    TEST(test_invalid_adc);
    TEST(test_recovery_after_spi_failure);

    /* Pass 10.11 */
    TEST(test_invalid_calibration_boundaries);
    TEST(test_invalid_temperature_adc);
    TEST(test_measurement_preserves_output_on_failure);
    TEST(test_calibration_reload_failure);
    TEST(test_measurement_null_arguments);

    printf(
        "ALL %d BMP280 NATIVE TEST GROUPS PASSED\n",
        tests_run);

    return EXIT_SUCCESS;
}
