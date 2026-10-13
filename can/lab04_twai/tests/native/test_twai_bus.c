#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "twai_bus.h"

static esp_err_t install_result, uninstall_result, start_result, stop_result, tx_result, rx_result;
static int installs, uninstalls, starts, stops, txs, rxs;
static twai_general_config_t captured_general;
static twai_timing_config_t captured_timing;
static twai_filter_config_t captured_filter;
static twai_message_t captured_tx;
static uint32_t captured_tx_ticks, captured_rx_ticks;

esp_err_t twai_driver_install(const twai_general_config_t *g, const twai_timing_config_t *t, const twai_filter_config_t *f) {
    installs++; captured_general=*g; captured_timing=*t; captured_filter=*f; return install_result;
}
esp_err_t twai_driver_uninstall(void) { uninstalls++; return uninstall_result; }
esp_err_t twai_start(void) { starts++; return start_result; }
esp_err_t twai_stop(void) { stops++; return stop_result; }
esp_err_t twai_transmit(const twai_message_t *m, uint32_t ticks) { txs++; captured_tx=*m; captured_tx_ticks=ticks; return tx_result; }
esp_err_t twai_receive(twai_message_t *m, uint32_t ticks) { rxs++; captured_rx_ticks=ticks; m->identifier=0x321; return rx_result; }

static void reset_mocks(void) {
    install_result=uninstall_result=start_result=stop_result=tx_result=rx_result=ESP_OK;
    installs=uninstalls=starts=stops=txs=rxs=0;
    memset(&captured_tx,0,sizeof(captured_tx));
}
static void cleanup(void) {
    /* Only called after a successful stop, or when the controller was never started. */
    assert(twai_bus_deinit()==ESP_OK);
}
#define RUN(test) do { test(); printf("PASS %s\n", #test); } while (0)

static void test_preinit_rejections(void) {
    twai_message_t m={0};
    assert(twai_bus_start()==ESP_ERR_INVALID_STATE);
    assert(twai_bus_stop()==ESP_ERR_INVALID_STATE);
    assert(twai_bus_deinit()==ESP_ERR_INVALID_STATE);
    assert(twai_bus_transmit(&m,100)==ESP_ERR_INVALID_STATE);
    assert(twai_bus_receive(&m,100)==ESP_ERR_INVALID_STATE);
}
static void test_install_failure_recovery(void) {
    reset_mocks(); install_result=ESP_FAIL;
    assert(twai_bus_init()==ESP_FAIL);
    assert(twai_bus_start()==ESP_ERR_INVALID_STATE);
    install_result=ESP_OK;
    assert(twai_bus_init()==ESP_OK);
    assert(installs==2);
    cleanup();
}
static void test_install_configuration_and_duplicate(void) {
    reset_mocks(); assert(twai_bus_init()==ESP_OK);
    assert(captured_general.tx_io==21 && captured_general.rx_io==22 && captured_general.mode==TWAI_MODE_NORMAL);
    assert(captured_timing.bitrate==500000 && captured_filter.accept_all==1);
    assert(twai_bus_init()==ESP_ERR_INVALID_STATE && installs==1);
    cleanup();
}
static void test_start_failure_and_retry(void) {
    reset_mocks(); assert(twai_bus_init()==ESP_OK);
    start_result=ESP_FAIL; assert(twai_bus_start()==ESP_FAIL);
    assert(twai_bus_stop()==ESP_ERR_INVALID_STATE);
    start_result=ESP_OK; assert(twai_bus_start()==ESP_OK);
    assert(starts==2);
    assert(twai_bus_stop()==ESP_OK); cleanup();
}
static void test_start_twice(void) {
    reset_mocks(); assert(twai_bus_init()==ESP_OK);
    assert(twai_bus_start()==ESP_OK);
    assert(twai_bus_start()==ESP_ERR_INVALID_STATE && starts==1);
    assert(twai_bus_stop()==ESP_OK); cleanup();
}
static void test_deinit_while_running(void) {
    reset_mocks(); assert(twai_bus_init()==ESP_OK); assert(twai_bus_start()==ESP_OK);
    assert(twai_bus_deinit()==ESP_ERR_INVALID_STATE && uninstalls==0);
    assert(twai_bus_stop()==ESP_OK); cleanup();
}
static void test_null_message(void) {
    reset_mocks(); assert(twai_bus_init()==ESP_OK); assert(twai_bus_start()==ESP_OK);
    assert(twai_bus_transmit(NULL,10)==ESP_ERR_INVALID_ARG);
    assert(twai_bus_receive(NULL,10)==ESP_ERR_INVALID_ARG);
    assert(txs==0 && rxs==0);
    assert(twai_bus_stop()==ESP_OK); cleanup();
}
static void test_dlc_bounds(void) {
    reset_mocks(); assert(twai_bus_init()==ESP_OK); assert(twai_bus_start()==ESP_OK);
    twai_message_t m={0}; m.identifier=0x123; m.data_length_code=9;
    assert(twai_bus_transmit(&m,10)==ESP_ERR_INVALID_ARG && txs==0);
    m.data_length_code=8;
    assert(twai_bus_transmit(&m,10)==ESP_OK && txs==1);
    assert(twai_bus_stop()==ESP_OK); cleanup();
}
static void test_standard_identifier_bounds(void) {
    reset_mocks(); assert(twai_bus_init()==ESP_OK); assert(twai_bus_start()==ESP_OK);
    twai_message_t m={0}; m.identifier=0x800;
    assert(twai_bus_transmit(&m,10)==ESP_ERR_INVALID_ARG && txs==0);
    m.identifier=0x7ff; assert(twai_bus_transmit(&m,10)==ESP_OK);
    assert(twai_bus_stop()==ESP_OK); cleanup();
}
static void test_extended_identifier_bounds(void) {
    reset_mocks(); assert(twai_bus_init()==ESP_OK); assert(twai_bus_start()==ESP_OK);
    twai_message_t m={0}; m.extd=1; m.identifier=0x20000000U;
    assert(twai_bus_transmit(&m,10)==ESP_ERR_INVALID_ARG && txs==0);
    m.identifier=0x1fffffffU; assert(twai_bus_transmit(&m,10)==ESP_OK);
    assert(twai_bus_stop()==ESP_OK); cleanup();
}
static void test_transmit_forwarding_and_error(void) {
    reset_mocks(); assert(twai_bus_init()==ESP_OK); assert(twai_bus_start()==ESP_OK);
    twai_message_t m={0}; m.identifier=0x123; m.data_length_code=3; m.data[0]=0xaa;
    tx_result=ESP_FAIL;
    assert(twai_bus_transmit(&m,150)==ESP_FAIL);
    assert(txs==1 && captured_tx.identifier==0x123 && captured_tx.data[0]==0xaa);
    assert(captured_tx_ticks==15);
    assert(twai_bus_stop()==ESP_OK); cleanup();
}
static void test_receive_forwarding_and_error(void) {
    reset_mocks(); assert(twai_bus_init()==ESP_OK); assert(twai_bus_start()==ESP_OK);
    twai_message_t m={0}; rx_result=ESP_FAIL;
    assert(twai_bus_receive(&m,250)==ESP_FAIL);
    assert(rxs==1 && captured_rx_ticks==25);
    rx_result=ESP_OK; assert(twai_bus_receive(&m,0)==ESP_OK);
    assert(m.identifier==0x321 && captured_rx_ticks==0);
    assert(twai_bus_stop()==ESP_OK); cleanup();
}
static void test_stop_failure_and_retry(void) {
    reset_mocks(); assert(twai_bus_init()==ESP_OK); assert(twai_bus_start()==ESP_OK);
    stop_result=ESP_FAIL; assert(twai_bus_stop()==ESP_FAIL);
    assert(twai_bus_deinit()==ESP_ERR_INVALID_STATE);
    stop_result=ESP_OK; assert(twai_bus_stop()==ESP_OK);
    assert(stops==2); cleanup();
}
static void test_double_stop_and_uninstall_failure(void) {
    reset_mocks(); assert(twai_bus_init()==ESP_OK); assert(twai_bus_start()==ESP_OK);
    assert(twai_bus_stop()==ESP_OK);
    assert(twai_bus_stop()==ESP_ERR_INVALID_STATE && stops==1);
    uninstall_result=ESP_FAIL; assert(twai_bus_deinit()==ESP_FAIL);
    assert(twai_bus_init()==ESP_ERR_INVALID_STATE);
    uninstall_result=ESP_OK; cleanup();
    assert(uninstalls==2);
    assert(twai_bus_deinit()==ESP_ERR_INVALID_STATE);
}
int main(void) {
    reset_mocks();
    RUN(test_preinit_rejections);
    RUN(test_install_failure_recovery);
    RUN(test_install_configuration_and_duplicate);
    RUN(test_start_failure_and_retry);
    RUN(test_start_twice);
    RUN(test_deinit_while_running);
    RUN(test_null_message);
    RUN(test_dlc_bounds);
    RUN(test_standard_identifier_bounds);
    RUN(test_extended_identifier_bounds);
    RUN(test_transmit_forwarding_and_error);
    RUN(test_receive_forwarding_and_error);
    RUN(test_stop_failure_and_retry);
    RUN(test_double_stop_and_uninstall_failure);
    puts("14 CAN/TWAI native test groups passed");
    return 0;
}
