#ifndef MOCK_SPI_MASTER_H
#define MOCK_SPI_MASTER_H
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#define SPI3_HOST 2
#define SPI_DMA_CH_AUTO 3
typedef void *spi_device_handle_t;
typedef struct { int mosi_io_num, miso_io_num, sclk_io_num, quadwp_io_num, quadhd_io_num, max_transfer_sz; } spi_bus_config_t;
typedef struct { int clock_speed_hz; uint8_t mode; int spics_io_num; int queue_size; } spi_device_interface_config_t;
typedef struct { size_t length; size_t rxlength; const void *tx_buffer; void *rx_buffer; } spi_transaction_t;
esp_err_t spi_bus_initialize(int host, const spi_bus_config_t *config, int dma);
esp_err_t spi_bus_free(int host);
esp_err_t spi_bus_add_device(int host, const spi_device_interface_config_t *config, spi_device_handle_t *out);
esp_err_t spi_bus_remove_device(spi_device_handle_t device);
esp_err_t spi_device_transmit(spi_device_handle_t device, spi_transaction_t *transaction);
#endif
