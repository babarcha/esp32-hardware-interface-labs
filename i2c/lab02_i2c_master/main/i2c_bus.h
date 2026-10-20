
#ifndef I2C_BUS_H
#define I2C_BUS_H

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_TIMEOUT_MS 1000

esp_err_t i2c_bus_init(void);
esp_err_t i2c_bus_probe(uint8_t address);
esp_err_t i2c_bus_scan(int *devices_found);

esp_err_t i2c_bus_read_register(
    uint8_t device_addr,
    uint8_t reg_addr,
    uint8_t *data,
    size_t len);

esp_err_t i2c_bus_write_register(
    uint8_t device_addr,
    uint8_t reg_addr,
    uint8_t value);

esp_err_t i2c_bus_deinit(void);

#endif
