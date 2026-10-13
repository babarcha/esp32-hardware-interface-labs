/* Host-side unit tests compiling the production i2c_bus.c with mock ESP-IDF APIs. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "i2c_bus.h"

struct mock_bus {int tag;};
struct mock_device {int tag;};
static struct mock_bus fake_bus = {1};
static struct mock_device fake_device = {2};
static int installs, deletes, probes, adds, removes, txs, rxs, txrxs;
static int last_address, last_timeout;
static esp_err_t next_install = ESP_OK, next_delete = ESP_OK, next_transfer = ESP_OK;
static int ack_address = -1;
static i2c_master_bus_config_t last_bus_cfg;
static i2c_device_config_t last_device_cfg;

esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t *cfg, i2c_master_bus_handle_t *out) {
    installs++; last_bus_cfg=*cfg;
    if (next_install == ESP_OK) *out=&fake_bus;
    return next_install;
}
esp_err_t i2c_del_master_bus(i2c_master_bus_handle_t bus) {
    assert(bus==&fake_bus); deletes++; return next_delete;
}
esp_err_t i2c_master_probe(i2c_master_bus_handle_t bus, uint16_t address, int timeout) {
    assert(bus==&fake_bus); probes++; last_address=address; last_timeout=timeout;
    return address==ack_address ? ESP_OK : ESP_ERR_NOT_FOUND;
}
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t bus, const i2c_device_config_t *cfg, i2c_master_dev_handle_t *out) {
    assert(bus==&fake_bus); adds++; last_device_cfg=*cfg; *out=&fake_device; return ESP_OK;
}
esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t dev) {
    assert(dev==&fake_device); removes++; return ESP_OK;
}
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t dev, const uint8_t *data, size_t len, int timeout) {
    assert(dev==&fake_device && data && len); txs++; last_timeout=timeout; return next_transfer;
}
esp_err_t i2c_master_receive(i2c_master_dev_handle_t dev, uint8_t *data, size_t len, int timeout) {
    assert(dev==&fake_device && data && len); rxs++; last_timeout=timeout; return next_transfer;
}
esp_err_t i2c_master_transmit_receive(i2c_master_dev_handle_t dev, const uint8_t *w, size_t wl, uint8_t *r, size_t rl, int timeout) {
    assert(dev==&fake_device && w && wl && r && rl); txrxs++; last_timeout=timeout; return next_transfer;
}

#define TEST(fn) do { fn(); printf("PASS %s\n", #fn); passed++; } while(0)
static void test_before_init(void) {
    assert(i2c_bus_deinit()==ESP_ERR_INVALID_STATE);
    assert(i2c_bus_probe(0x20)==ESP_ERR_INVALID_STATE);
    i2c_master_dev_handle_t d=NULL;
    assert(i2c_bus_add_device(0x20,100000,&d)==ESP_ERR_INVALID_STATE);
}
static void test_install_failure(void) {
    next_install=ESP_FAIL;
    assert(i2c_bus_init()==ESP_FAIL);
    assert(i2c_bus_deinit()==ESP_ERR_INVALID_STATE);
    next_install=ESP_OK;
}
static void test_init_and_duplicate(void) {
    int before=installs;
    assert(i2c_bus_init()==ESP_OK);
    assert(installs==before+1);
    assert(last_bus_cfg.i2c_port==I2C_NUM_0);
    assert(last_bus_cfg.sda_io_num==21 && last_bus_cfg.scl_io_num==22);
    assert(last_bus_cfg.flags.enable_internal_pullup);
    assert(i2c_bus_init()==ESP_ERR_INVALID_STATE);
    assert(installs==before+1);
}
static void test_probe_boundaries(void) {
    int before=probes;
    assert(i2c_bus_probe(0x07)==ESP_ERR_INVALID_ARG);
    assert(i2c_bus_probe(0x78)==ESP_ERR_INVALID_ARG);
    assert(probes==before);
    ack_address=0x08;
    assert(i2c_bus_probe(0x08)==ESP_OK);
    assert(last_timeout==50);
    assert(i2c_bus_probe(0x77)==ESP_ERR_NOT_FOUND);
}
static void test_scan(void) {
    int before=probes;
    ack_address=0x42;
    assert(i2c_bus_scan()==1);
    assert(probes-before==112);
    assert(last_address==0x77);
    ack_address=-1;
}
static void test_add_device_arguments(void) {
    i2c_master_dev_handle_t d=NULL;
    int before=adds;
    assert(i2c_bus_add_device(0x07,100000,&d)==ESP_ERR_INVALID_ARG);
    assert(i2c_bus_add_device(0x78,100000,&d)==ESP_ERR_INVALID_ARG);
    assert(i2c_bus_add_device(0x40,0,&d)==ESP_ERR_INVALID_ARG);
    assert(i2c_bus_add_device(0x40,100000,NULL)==ESP_ERR_INVALID_ARG);
    assert(adds==before);
}
static void test_add_remove_device(void) {
    i2c_master_dev_handle_t d=NULL;
    assert(i2c_bus_add_device(0x40,400000,&d)==ESP_OK);
    assert(d==&fake_device);
    assert(last_device_cfg.device_address==0x40);
    assert(last_device_cfg.scl_speed_hz==400000);
    assert(last_device_cfg.dev_addr_length==I2C_ADDR_BIT_LEN_7);
    assert(i2c_bus_remove_device(NULL)==ESP_ERR_INVALID_ARG);
    assert(i2c_bus_remove_device(d)==ESP_OK);
}
static void test_write_arguments(void) {
    uint8_t bytes[2]={1,2}; int before=txs;
    assert(i2c_bus_write(NULL,bytes,2)==ESP_ERR_INVALID_ARG);
    assert(i2c_bus_write(&fake_device,NULL,2)==ESP_ERR_INVALID_ARG);
    assert(i2c_bus_write(&fake_device,bytes,0)==ESP_ERR_INVALID_ARG);
    assert(txs==before);
}
static void test_read_arguments(void) {
    uint8_t bytes[2]={0}; int before=rxs;
    assert(i2c_bus_read(NULL,bytes,2)==ESP_ERR_INVALID_ARG);
    assert(i2c_bus_read(&fake_device,NULL,2)==ESP_ERR_INVALID_ARG);
    assert(i2c_bus_read(&fake_device,bytes,0)==ESP_ERR_INVALID_ARG);
    assert(rxs==before);
}
static void test_write_read_arguments(void) {
    uint8_t bytes[2]={0}; int before=txrxs;
    assert(i2c_bus_write_read(NULL,bytes,1,bytes,1)==ESP_ERR_INVALID_ARG);
    assert(i2c_bus_write_read(&fake_device,NULL,1,bytes,1)==ESP_ERR_INVALID_ARG);
    assert(i2c_bus_write_read(&fake_device,bytes,0,bytes,1)==ESP_ERR_INVALID_ARG);
    assert(i2c_bus_write_read(&fake_device,bytes,1,NULL,1)==ESP_ERR_INVALID_ARG);
    assert(i2c_bus_write_read(&fake_device,bytes,1,bytes,0)==ESP_ERR_INVALID_ARG);
    assert(txrxs==before);
}
static void test_transfer_calls_and_error_propagation(void) {
    uint8_t bytes[2]={0};
    assert(i2c_bus_write(&fake_device,bytes,2)==ESP_OK);
    assert(last_timeout==100);
    assert(i2c_bus_read(&fake_device,bytes,2)==ESP_OK);
    assert(i2c_bus_write_read(&fake_device,bytes,1,bytes,2)==ESP_OK);
    next_transfer=ESP_ERR_TIMEOUT;
    assert(i2c_bus_write(&fake_device,bytes,2)==ESP_ERR_TIMEOUT);
    assert(i2c_bus_read(&fake_device,bytes,2)==ESP_ERR_TIMEOUT);
    assert(i2c_bus_write_read(&fake_device,bytes,1,bytes,2)==ESP_ERR_TIMEOUT);
    next_transfer=ESP_OK;
}
static void test_cleanup_and_reinit(void) {
    next_delete=ESP_FAIL;
    assert(i2c_bus_deinit()==ESP_FAIL);
    assert(i2c_bus_init()==ESP_ERR_INVALID_STATE);
    next_delete=ESP_OK;
    assert(i2c_bus_deinit()==ESP_OK);
    assert(i2c_bus_deinit()==ESP_ERR_INVALID_STATE);
    assert(i2c_bus_init()==ESP_OK);
    assert(i2c_bus_deinit()==ESP_OK);
}
int main(void) {
    int passed=0;
    TEST(test_before_init);
    TEST(test_install_failure);
    TEST(test_init_and_duplicate);
    TEST(test_probe_boundaries);
    TEST(test_scan);
    TEST(test_add_device_arguments);
    TEST(test_add_remove_device);
    TEST(test_write_arguments);
    TEST(test_read_arguments);
    TEST(test_write_read_arguments);
    TEST(test_transfer_calls_and_error_propagation);
    TEST(test_cleanup_and_reinit);
    printf("All %d I2C native tests passed\n",passed);
    return 0;
}
