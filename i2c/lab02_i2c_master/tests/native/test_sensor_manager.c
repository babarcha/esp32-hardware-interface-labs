#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "sensor_manager.h"
#include "i2c_bus.h"

static int available_low, available_high;
static int fail_low, fail_high;
static int probe_low, probe_high, init_low, init_high;
static int delay_calls, last_delay;

static void reset_mocks(void) {
    available_low = available_high = 0;
    fail_low = fail_high = 0;
    probe_low = probe_high = init_low = init_high = 0;
    delay_calls = last_delay = 0;
}

esp_err_t i2c_bus_probe(uint8_t address) {
    if (address == BME280_I2C_ADDR_LOW) {
        probe_low++;
        return available_low ? ESP_OK : ESP_ERR_NOT_FOUND;
    }
    if (address == BME280_I2C_ADDR_HIGH) {
        probe_high++;
        return available_high ? ESP_OK : ESP_ERR_NOT_FOUND;
    }
    return ESP_ERR_INVALID_ARG;
}

esp_err_t bmp280_init(bmp280_t *sensor, uint8_t address) {
    assert(sensor != NULL);
    if (address == BME280_I2C_ADDR_LOW) {
        init_low++;
        if (fail_low) return ESP_FAIL;
    } else if (address == BME280_I2C_ADDR_HIGH) {
        init_high++;
        if (fail_high) return ESP_FAIL;
    } else {
        return ESP_ERR_INVALID_ARG;
    }
    sensor->address = address;
    sensor->initialized = true;
    return ESP_OK;
}

void vTaskDelay(int ticks) {
    delay_calls++;
    last_delay = ticks;
}

#define TEST(fn) do { reset_mocks(); fn(); printf("PASS %s\n", #fn); passed++; } while (0)

static void test_null_inputs(void) {
    sensor_manager_init(NULL);
    sensor_manager_mark_unavailable(NULL);
    assert(!sensor_manager_is_ready(NULL));
    assert(sensor_manager_get_active(NULL) == NULL);
    assert(sensor_manager_reinit(NULL) == ESP_ERR_INVALID_ARG);
}
static void test_initial_state(void) {
    sensor_manager_t m;
    memset(&m, 0xA5, sizeof(m));
    sensor_manager_init(&m);
    assert(!sensor_manager_is_ready(&m));
    assert(sensor_manager_get_active(&m) == NULL);
    assert(!m.sensor.initialized);
}
static void test_no_sensor(void) {
    sensor_manager_t m;
    sensor_manager_init(&m);
    assert(sensor_manager_reinit(&m) == ESP_ERR_NOT_FOUND);
    assert(probe_low == 1 && probe_high == 1);
    assert(init_low == 0 && init_high == 0);
    assert(delay_calls == 0);
    assert(!sensor_manager_is_ready(&m));
}
static void test_low_address_success(void) {
    sensor_manager_t m;
    sensor_manager_init(&m);
    available_low = available_high = 1;
    assert(sensor_manager_reinit(&m) == ESP_OK);
    assert(probe_low == 1 && probe_high == 0);
    assert(init_low == 1 && init_high == 0);
    assert(delay_calls == 1 && last_delay == 100);
    assert(sensor_manager_is_ready(&m));
    assert(sensor_manager_get_active(&m) == &m.sensor);
    assert(m.sensor.initialized && m.sensor.address == 0x76);
}
static void test_high_address_fallback(void) {
    sensor_manager_t m;
    sensor_manager_init(&m);
    available_high = 1;
    assert(sensor_manager_reinit(&m) == ESP_OK);
    assert(probe_low == 1 && probe_high == 1);
    assert(init_low == 0 && init_high == 1);
    assert(m.sensor.address == 0x77 && sensor_manager_is_ready(&m));
}
static void test_init_failure_both(void) {
    sensor_manager_t m;
    sensor_manager_init(&m);
    available_low = available_high = 1;
    fail_low = fail_high = 1;
    assert(sensor_manager_reinit(&m) == ESP_ERR_NOT_FOUND);
    assert(init_low == 1 && init_high == 1);
    assert(!sensor_manager_is_ready(&m));
    assert(!m.sensor.initialized);
    assert(delay_calls == 0);
}
static void test_init_failure_then_fallback(void) {
    sensor_manager_t m;
    sensor_manager_init(&m);
    available_low = available_high = 1;
    fail_low = 1;
    assert(sensor_manager_reinit(&m) == ESP_OK);
    assert(init_low == 1 && init_high == 1);
    assert(m.sensor.address == 0x77);
    assert(sensor_manager_is_ready(&m));
}
static void test_mark_unavailable(void) {
    sensor_manager_t m;
    sensor_manager_init(&m);
    available_low = 1;
    assert(sensor_manager_reinit(&m) == ESP_OK);
    sensor_manager_mark_unavailable(&m);
    assert(!sensor_manager_is_ready(&m));
    assert(!m.sensor.initialized);
    assert(sensor_manager_get_active(&m) == NULL);
}
static void test_recovery_after_missing(void) {
    sensor_manager_t m;
    sensor_manager_init(&m);
    assert(sensor_manager_reinit(&m) == ESP_ERR_NOT_FOUND);
    available_low = 1;
    assert(sensor_manager_reinit(&m) == ESP_OK);
    assert(sensor_manager_is_ready(&m));
    assert(m.sensor.address == 0x76);
}
static void test_recovery_after_loss(void) {
    sensor_manager_t m;
    sensor_manager_init(&m);
    available_low = 1;
    assert(sensor_manager_reinit(&m) == ESP_OK);
    available_low = 0;
    assert(sensor_manager_reinit(&m) == ESP_ERR_NOT_FOUND);
    assert(!sensor_manager_is_ready(&m));
    available_high = 1;
    assert(sensor_manager_reinit(&m) == ESP_OK);
    assert(sensor_manager_is_ready(&m));
    assert(m.sensor.address == 0x77);
}
int main(void) {
    int passed = 0;
    TEST(test_null_inputs);
    TEST(test_initial_state);
    TEST(test_no_sensor);
    TEST(test_low_address_success);
    TEST(test_high_address_fallback);
    TEST(test_init_failure_both);
    TEST(test_init_failure_then_fallback);
    TEST(test_mark_unavailable);
    TEST(test_recovery_after_missing);
    TEST(test_recovery_after_loss);
    printf("All %d sensor-manager native test groups passed\n", passed);
    return 0;
}
