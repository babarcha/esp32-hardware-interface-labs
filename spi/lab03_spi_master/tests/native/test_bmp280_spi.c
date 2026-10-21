
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
            fprintf(stderr, "FAIL %s:%d: %s\n", \
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

static esp_err_t mock_read_result = ESP_OK;
static esp_err_t mock_write_result = ESP_OK;

static uint8_t mock_registers[256];
static uint8_t last_write_address;
static uint8_t last_write_value;

static int read_calls;
static int write_calls;
static int delay_calls;

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

    mock_registers[0xD0] = 0x58;
}

/* Mock the production SPI bus interface. */
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

    if (mock_read_result != ESP_OK)
        return mock_read_result;

    uint8_t address = (uint8_t)(tx[0] | 0x80);
    uint8_t reg = address;

    memset(rx, 0, length);

    for (size_t i = 1; i < length; i++)
    {
        rx[i] = mock_registers[reg++];
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

    return mock_write_result;
}

void vTaskDelay(uint32_t ticks)
{
    (void)ticks;
    delay_calls++;
}

static bmp280_spi_t new_sensor(void)
{
    bmp280_spi_t sensor = {0};

    CHECK(bmp280_spi_init(&sensor, DEVICE) == ESP_OK);

    return sensor;
}

static void set_u16(uint8_t address, uint16_t value)
{
    mock_registers[address] = (uint8_t)value;
    mock_registers[(uint8_t)(address + 1)] =
        (uint8_t)(value >> 8);
}

static void valid_calibration(void)
{
    /* Sufficient for calibration parsing tests. */
    set_u16(0x88, 27504); /* dig_T1 */
    set_u16(0x8A, 26435); /* dig_T2 */
    set_u16(0x8C, 50);    /* dig_T3 */
    set_u16(0x8E, 36477); /* dig_P1 */
}

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
    CHECK(id == 0x58);
    CHECK(bmp280_spi_verify(&sensor) == ESP_OK);
}

static void test_wrong_chip_id(void)
{
    reset_mock();
    bmp280_spi_t sensor = new_sensor();

    mock_registers[0xD0] = 0x00;

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

    printf("ALL %d BMP280 NATIVE TEST GROUPS PASSED\n",
           tests_run);

    return EXIT_SUCCESS;
}
