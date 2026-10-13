#ifndef MOCK_DRIVER_TWAI_H
#define MOCK_DRIVER_TWAI_H
#include <stdint.h>
#include "esp_err.h"
typedef struct { int tx_io; int rx_io; int mode; } twai_general_config_t;
typedef struct { int bitrate; } twai_timing_config_t;
typedef struct { int accept_all; } twai_filter_config_t;
typedef struct {
    uint32_t identifier;
    uint8_t extd;
    uint8_t rtr;
    uint8_t data_length_code;
    uint8_t data[8];
} twai_message_t;
#define TWAI_MODE_NORMAL 0
#define TWAI_GENERAL_CONFIG_DEFAULT(tx, rx, mode_) ((twai_general_config_t){(tx), (rx), (mode_)})
#define TWAI_TIMING_CONFIG_500KBITS() ((twai_timing_config_t){500000})
#define TWAI_FILTER_CONFIG_ACCEPT_ALL() ((twai_filter_config_t){1})
esp_err_t twai_driver_install(const twai_general_config_t *, const twai_timing_config_t *, const twai_filter_config_t *);
esp_err_t twai_driver_uninstall(void);
esp_err_t twai_start(void);
esp_err_t twai_stop(void);
esp_err_t twai_transmit(const twai_message_t *, uint32_t);
esp_err_t twai_receive(twai_message_t *, uint32_t);
#endif
