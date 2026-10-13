/* Host-native tests for the real production spi_bus.c, using a mock driver. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "spi_bus.h"

static int tests_run = 0;
#define CHECK(cond) do { if (!(cond)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); exit(1); } } while (0)
#define TEST(fn) do { fn(); tests_run++; printf("PASS %s\n", #fn); } while (0)

static int initialize_calls, free_calls, add_calls, remove_calls, transmit_calls;
static esp_err_t init_result, free_result, add_result, remove_result, tx_result;
static spi_bus_config_t last_bus;
static spi_device_interface_config_t last_device;
static spi_transaction_t last_transaction;
static int last_host, last_dma;
static int dummy_device;
#define DEVICE ((spi_device_handle_t)&dummy_device)

static void reset_mocks(void) {
    initialize_calls = free_calls = add_calls = remove_calls = transmit_calls = 0;
    init_result = free_result = add_result = remove_result = tx_result = ESP_OK;
    memset(&last_bus, 0, sizeof(last_bus));
    memset(&last_device, 0, sizeof(last_device));
    memset(&last_transaction, 0, sizeof(last_transaction));
    last_host = last_dma = -1;
}

esp_err_t spi_bus_initialize(int host, const spi_bus_config_t *cfg, int dma) {
    initialize_calls++; last_host = host; last_dma = dma; last_bus = *cfg; return init_result;
}
esp_err_t spi_bus_free(int host) { free_calls++; last_host = host; return free_result; }
esp_err_t spi_bus_add_device(int host, const spi_device_interface_config_t *cfg, spi_device_handle_t *out) {
    add_calls++; last_host = host; last_device = *cfg; if (add_result == ESP_OK) *out = DEVICE; return add_result;
}
esp_err_t spi_bus_remove_device(spi_device_handle_t dev) { remove_calls++; CHECK(dev == DEVICE); return remove_result; }
esp_err_t spi_device_transmit(spi_device_handle_t dev, spi_transaction_t *t) {
    transmit_calls++; CHECK(dev == DEVICE); last_transaction = *t; return tx_result;
}

static void start(void) { reset_mocks(); CHECK(spi_bus_init() == ESP_OK); }
static void finish(void) { CHECK(spi_bus_deinit() == ESP_OK); }

static void test_uninitialized_state(void) {
    spi_device_handle_t d = NULL; uint8_t b = 1;
    CHECK(spi_bus_deinit() == ESP_ERR_INVALID_STATE);
    CHECK(spi_device_register(5, 1000000, 0, &d) == ESP_ERR_INVALID_STATE);
    CHECK(spi_device_unregister(DEVICE) == ESP_ERR_INVALID_STATE);
    CHECK(spi_bus_write(DEVICE, &b, 1) == ESP_ERR_INVALID_STATE);
    CHECK(spi_bus_read(DEVICE, &b, 1) == ESP_ERR_INVALID_STATE);
    CHECK(spi_bus_transfer(DEVICE, &b, &b, 1) == ESP_ERR_INVALID_STATE);
}
static void test_init_configuration_and_duplicate(void) {
    reset_mocks(); CHECK(spi_bus_init() == ESP_OK);
    CHECK(initialize_calls == 1 && last_host == SPI3_HOST && last_dma == SPI_DMA_CH_AUTO);
    CHECK(last_bus.mosi_io_num == 23 && last_bus.miso_io_num == 19 && last_bus.sclk_io_num == 18);
    CHECK(last_bus.quadwp_io_num == -1 && last_bus.quadhd_io_num == -1);
    CHECK(spi_bus_init() == ESP_ERR_INVALID_STATE); CHECK(initialize_calls == 1);
    finish(); CHECK(free_calls == 1);
}
static void test_init_failure_retry(void) {
    reset_mocks(); init_result = ESP_FAIL;
    CHECK(spi_bus_init() == ESP_FAIL); CHECK(initialize_calls == 1);
    init_result = ESP_OK; CHECK(spi_bus_init() == ESP_OK); finish();
}
static void test_deinit_failure_retry(void) {
    start(); free_result = ESP_FAIL;
    CHECK(spi_bus_deinit() == ESP_FAIL);
    CHECK(spi_bus_init() == ESP_ERR_INVALID_STATE);
    free_result = ESP_OK; finish(); CHECK(free_calls == 2);
}
static void test_register_invalid_arguments(void) {
    start(); spi_device_handle_t d = NULL; spi_device_handle_t used = DEVICE;
    CHECK(spi_device_register(5, 1000000, 0, NULL) == ESP_ERR_INVALID_ARG);
    CHECK(spi_device_register(5, 1000000, 0, &used) == ESP_ERR_INVALID_STATE);
    CHECK(spi_device_register(-1, 1000000, 0, &d) == ESP_ERR_INVALID_ARG);
    CHECK(spi_device_register(5, 0, 0, &d) == ESP_ERR_INVALID_ARG);
    CHECK(spi_device_register(5, -1, 0, &d) == ESP_ERR_INVALID_ARG);
    CHECK(spi_device_register(5, 1000000, 4, &d) == ESP_ERR_INVALID_ARG);
    CHECK(add_calls == 0); finish();
}
static void test_register_config_and_propagation(void) {
    start(); spi_device_handle_t d = NULL; add_result = ESP_FAIL;
    CHECK(spi_device_register(5, 1000000, 0, &d) == ESP_FAIL); CHECK(d == NULL);
    add_result = ESP_OK;
    CHECK(spi_device_register(5, 1000000, 3, &d) == ESP_OK);
    CHECK(d == DEVICE && last_host == SPI3_HOST && last_device.spics_io_num == 5);
    CHECK(last_device.clock_speed_hz == 1000000 && last_device.mode == 3 && last_device.queue_size == 1);
    CHECK(spi_device_unregister(d) == ESP_OK); finish();
}
static void test_unregister_validation_and_error(void) {
    start(); CHECK(spi_device_unregister(NULL) == ESP_ERR_INVALID_ARG);
    CHECK(remove_calls == 0); remove_result = ESP_FAIL;
    CHECK(spi_device_unregister(DEVICE) == ESP_FAIL); CHECK(remove_calls == 1);
    remove_result = ESP_OK; CHECK(spi_device_unregister(DEVICE) == ESP_OK); finish();
}
static void test_write_validation(void) {
    start(); uint8_t b[] = {0x9a, 0xbc, 0xde};
    CHECK(spi_bus_write(NULL, b, 3) == ESP_ERR_INVALID_ARG);
    CHECK(spi_bus_write(DEVICE, NULL, 3) == ESP_ERR_INVALID_ARG);
    CHECK(spi_bus_write(DEVICE, b, 0) == ESP_ERR_INVALID_ARG);
    CHECK(transmit_calls == 0);
    CHECK(spi_bus_write(DEVICE, b, 3) == ESP_OK);
    CHECK(last_transaction.length == 24 && last_transaction.tx_buffer == b);
    CHECK(last_transaction.rx_buffer == NULL && last_transaction.rxlength == 0);
    finish();
}
static void test_read_validation(void) {
    start(); uint8_t b[3] = {0};
    CHECK(spi_bus_read(NULL, b, 3) == ESP_ERR_INVALID_ARG);
    CHECK(spi_bus_read(DEVICE, NULL, 3) == ESP_ERR_INVALID_ARG);
    CHECK(spi_bus_read(DEVICE, b, 0) == ESP_ERR_INVALID_ARG);
    CHECK(transmit_calls == 0);
    CHECK(spi_bus_read(DEVICE, b, 3) == ESP_OK);
    CHECK(last_transaction.length == 24 && last_transaction.rxlength == 24);
    CHECK(last_transaction.tx_buffer == NULL && last_transaction.rx_buffer == b);
    finish();
}
static void test_transfer_validation(void) {
    start(); uint8_t tx[3] = {1,2,3}, rx[3] = {0};
    CHECK(spi_bus_transfer(NULL, tx, rx, 3) == ESP_ERR_INVALID_ARG);
    CHECK(spi_bus_transfer(DEVICE, NULL, rx, 3) == ESP_ERR_INVALID_ARG);
    CHECK(spi_bus_transfer(DEVICE, tx, NULL, 3) == ESP_ERR_INVALID_ARG);
    CHECK(spi_bus_transfer(DEVICE, tx, rx, 0) == ESP_ERR_INVALID_ARG);
    CHECK(transmit_calls == 0);
    CHECK(spi_bus_transfer(DEVICE, tx, rx, 3) == ESP_OK);
    CHECK(last_transaction.length == 24 && last_transaction.rxlength == 0);
    CHECK(last_transaction.tx_buffer == tx && last_transaction.rx_buffer == rx);
    finish();
}
static void test_transmit_error_propagation(void) {
    start(); uint8_t b = 7; tx_result = ESP_FAIL;
    CHECK(spi_bus_write(DEVICE, &b, 1) == ESP_FAIL);
    CHECK(spi_bus_read(DEVICE, &b, 1) == ESP_FAIL);
    CHECK(spi_bus_transfer(DEVICE, &b, &b, 1) == ESP_FAIL);
    CHECK(transmit_calls == 3); finish();
}
static void test_reinitialize_after_deinit(void) {
    start(); finish(); CHECK(spi_bus_init() == ESP_OK); finish();
    CHECK(initialize_calls == 2 && free_calls == 2);
}

int main(void) {
    TEST(test_uninitialized_state);
    TEST(test_init_configuration_and_duplicate);
    TEST(test_init_failure_retry);
    TEST(test_deinit_failure_retry);
    TEST(test_register_invalid_arguments);
    TEST(test_register_config_and_propagation);
    TEST(test_unregister_validation_and_error);
    TEST(test_write_validation);
    TEST(test_read_validation);
    TEST(test_transfer_validation);
    TEST(test_transmit_error_propagation);
    TEST(test_reinitialize_after_deinit);
    printf("ALL %d SPI NATIVE TEST GROUPS PASSED\n", tests_run);
    return 0;
}
