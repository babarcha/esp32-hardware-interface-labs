/* Native tests for the production ESP-IDF I2C bus wrapper. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "driver/i2c_master.h"
#include "i2c_bus.h"

struct mock_bus
{
    int tag;
};
struct mock_device
{
    int tag;
};
static struct mock_bus fake_bus = {1};
static struct mock_device fake_device = {2};
static int installs, deletes, probes, adds, removes, txs, txrxs;
static int last_address, last_timeout;
static int ack_address = -1;
static esp_err_t next_install = ESP_OK;
static esp_err_t next_delete = ESP_OK;
static esp_err_t next_add = ESP_OK;
static esp_err_t next_remove = ESP_OK;
static esp_err_t next_transfer = ESP_OK;
static i2c_master_bus_config_t last_bus_cfg;
static i2c_device_config_t last_device_cfg;
static uint8_t last_tx[2];
static uint8_t last_register;

esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t *cfg, i2c_master_bus_handle_t *out)
{
    installs++;
    last_bus_cfg = *cfg;
    if (next_install == ESP_OK)
        *out = &fake_bus;
    return next_install;
}
esp_err_t i2c_del_master_bus(i2c_master_bus_handle_t bus)
{
    assert(bus == &fake_bus);
    deletes++;
    return next_delete;
}
esp_err_t i2c_master_probe(i2c_master_bus_handle_t bus, uint16_t address, int timeout)
{
    assert(bus == &fake_bus);
    probes++;
    last_address = address;
    last_timeout = timeout;
    return address == ack_address ? ESP_OK : ESP_ERR_NOT_FOUND;
}
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t bus, const i2c_device_config_t *cfg, i2c_master_dev_handle_t *out)
{
    assert(bus == &fake_bus);
    adds++;
    last_device_cfg = *cfg;
    if (next_add == ESP_OK)
        *out = &fake_device;
    return next_add;
}
esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t dev)
{
    assert(dev == &fake_device);
    removes++;
    return next_remove;
}
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t dev, const uint8_t *data, size_t len, int timeout)
{
    assert(dev == &fake_device && data && len == 2);
    txs++;
    last_timeout = timeout;
    memcpy(last_tx, data, 2);
    return next_transfer;
}
esp_err_t i2c_master_receive(i2c_master_dev_handle_t dev, uint8_t *data, size_t len, int timeout)
{
    (void)dev;
    (void)data;
    (void)len;
    (void)timeout;
    assert(!"Unexpected standalone receive");
    return ESP_FAIL;
}
esp_err_t i2c_master_transmit_receive(i2c_master_dev_handle_t dev, const uint8_t *w, size_t wl, uint8_t *r, size_t rl, int timeout)
{
    assert(dev == &fake_device && w && wl == 1 && r && rl > 0);
    txrxs++;
    last_register = w[0];
    last_timeout = timeout;
    if (next_transfer == ESP_OK)
        memset(r, 0xA5, rl);
    return next_transfer;
}

#define TEST(fn)                  \
    do                            \
    {                             \
        fn();                     \
        printf("PASS %s\n", #fn); \
        passed++;                 \
    } while (0)

static void test_before_init(void)
{
    int found = -1;
    uint8_t data = 0;
    assert(i2c_bus_deinit() == ESP_ERR_INVALID_STATE);
    assert(i2c_bus_probe(0x76) == ESP_ERR_INVALID_STATE);
    assert(i2c_bus_scan(&found) == ESP_ERR_INVALID_STATE);
    assert(i2c_bus_read_register(0x76, 0xD0, &data, 1) == ESP_ERR_INVALID_STATE);
    assert(i2c_bus_write_register(0x76, 0xF4, 0x27) == ESP_ERR_INVALID_STATE);
}
static void test_install_failure(void)
{
    next_install = ESP_FAIL;
    assert(i2c_bus_init() == ESP_FAIL);
    assert(i2c_bus_deinit() == ESP_ERR_INVALID_STATE);
    next_install = ESP_OK;
}
static void test_init_and_duplicate(void)
{
    int before = installs;
    assert(i2c_bus_init() == ESP_OK);
    assert(installs == before + 1);
    assert(last_bus_cfg.i2c_port == I2C_NUM_0);
    assert(last_bus_cfg.sda_io_num == I2C_MASTER_SDA_IO);
    assert(last_bus_cfg.scl_io_num == I2C_MASTER_SCL_IO);
    assert(last_bus_cfg.glitch_ignore_cnt == 7);
    assert(last_bus_cfg.flags.enable_internal_pullup);
    assert(i2c_bus_init() == ESP_ERR_INVALID_STATE);
}
static void test_probe_boundaries(void)
{
    int before = probes;
    assert(i2c_bus_probe(0x80) == ESP_ERR_INVALID_ARG);
    assert(probes == before);
    ack_address = 0x76;
    assert(i2c_bus_probe(0x76) == ESP_OK);
    assert(last_timeout == I2C_MASTER_TIMEOUT_MS);
    assert(i2c_bus_probe(0x77) == ESP_ERR_NOT_FOUND);
    ack_address = -1;
}
static void test_scan(void)
{
    int found = -1;
    int before = probes;
    assert(i2c_bus_scan(NULL) == ESP_ERR_INVALID_ARG);
    assert(probes == before);
    ack_address = 0x76;
    assert(i2c_bus_scan(&found) == ESP_OK);
    assert(found == 1);
    assert(probes - before == 112);
    assert(last_address == 0x77);
    ack_address = -1;
    assert(i2c_bus_scan(&found) == ESP_OK);
    assert(found == 0);
}
static void test_register_argument_validation(void)
{
    uint8_t data = 0;
    int before = adds;
    assert(i2c_bus_read_register(0x76, 0xD0, NULL, 1) == ESP_ERR_INVALID_ARG);
    assert(i2c_bus_read_register(0x76, 0xD0, &data, 0) == ESP_ERR_INVALID_ARG);
    assert(adds == before);
    assert(i2c_bus_read_register(0x80, 0xD0, &data, 1) == ESP_ERR_INVALID_ARG);
    assert(i2c_bus_write_register(0x80, 0xF4, 0x27) == ESP_ERR_INVALID_ARG);
}
static void test_read_register(void)
{
    uint8_t data[3] = {0};
    int before_add = adds, before_remove = removes;
    assert(i2c_bus_read_register(0x76, 0xD0, data, sizeof(data)) == ESP_OK);
    assert(adds == before_add + 1 && removes == before_remove + 1);
    assert(last_device_cfg.device_address == 0x76);
    assert(last_device_cfg.dev_addr_length == I2C_ADDR_BIT_LEN_7);
    assert(last_device_cfg.scl_speed_hz == I2C_MASTER_FREQ_HZ);
    assert(last_register == 0xD0);
    assert(last_timeout == I2C_MASTER_TIMEOUT_MS);
    assert(data[0] == 0xA5 && data[1] == 0xA5 && data[2] == 0xA5);
}
static void test_write_register(void)
{
    int before_add = adds, before_remove = removes;
    assert(i2c_bus_write_register(0x76, 0xF4, 0x27) == ESP_OK);
    assert(adds == before_add + 1 && removes == before_remove + 1);
    assert(last_tx[0] == 0xF4 && last_tx[1] == 0x27);
    assert(last_timeout == I2C_MASTER_TIMEOUT_MS);
}
static void test_add_failure(void)
{
    uint8_t data = 0;
    int before_remove = removes;
    next_add = ESP_FAIL;
    assert(i2c_bus_read_register(0x76, 0xD0, &data, 1) == ESP_FAIL);
    assert(i2c_bus_write_register(0x76, 0xF4, 0x27) == ESP_FAIL);
    assert(removes == before_remove);
    next_add = ESP_OK;
}
static void test_transfer_failure(void)
{
    uint8_t data = 0;
    int before_remove = removes;
    next_transfer = ESP_ERR_TIMEOUT;
    assert(i2c_bus_read_register(0x76, 0xD0, &data, 1) == ESP_ERR_TIMEOUT);
    assert(i2c_bus_write_register(0x76, 0xF4, 0x27) == ESP_ERR_TIMEOUT);
    assert(removes == before_remove + 2);
    next_transfer = ESP_OK;
}
static void test_remove_failure(void)
{
    uint8_t data = 0;
    next_remove = ESP_FAIL;
    assert(i2c_bus_read_register(0x76, 0xD0, &data, 1) == ESP_FAIL);
    assert(i2c_bus_write_register(0x76, 0xF4, 0x27) == ESP_FAIL);
    next_transfer = ESP_ERR_TIMEOUT;
    assert(i2c_bus_read_register(0x76, 0xD0, &data, 1) == ESP_ERR_TIMEOUT);
    next_transfer = ESP_OK;
    next_remove = ESP_OK;
}
static void test_cleanup_and_reinit(void)
{
    next_delete = ESP_FAIL;
    assert(i2c_bus_deinit() == ESP_FAIL);
    assert(i2c_bus_init() == ESP_ERR_INVALID_STATE);
    next_delete = ESP_OK;
    assert(i2c_bus_deinit() == ESP_OK);
    assert(i2c_bus_deinit() == ESP_ERR_INVALID_STATE);
    assert(i2c_bus_init() == ESP_OK);
    assert(i2c_bus_deinit() == ESP_OK);
}
int main(void)
{
    int passed = 0;
    TEST(test_before_init);
    TEST(test_install_failure);
    TEST(test_init_and_duplicate);
    TEST(test_probe_boundaries);
    TEST(test_scan);
    TEST(test_register_argument_validation);
    TEST(test_read_register);
    TEST(test_write_register);
    TEST(test_add_failure);
    TEST(test_transfer_failure);
    TEST(test_remove_failure);
    TEST(test_cleanup_and_reinit);
    printf("All %d I2C native test groups passed\n", passed);
    return 0;
}
