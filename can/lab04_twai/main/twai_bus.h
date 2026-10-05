#ifndef TWAI_BUS_H
#define TWAI_BUS_H

#include "esp_err.h"
#include "driver/twai.h"

esp_err_t twai_bus_init(void);
esp_err_t twai_bus_start(void);
esp_err_t twai_bus_stop(void);
esp_err_t twai_bus_deinit(void);

esp_err_t twai_bus_transmit(
    const twai_message_t *message,
    uint32_t timeout_ms);

esp_err_t twai_bus_receive(
    twai_message_t *message,
    uint32_t timeout_ms);

#endif