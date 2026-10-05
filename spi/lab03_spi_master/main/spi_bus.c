#include <stdbool.h>
#include <string.h>

#include "spi_bus.h"

#define SPI_MOSI_GPIO 23
#define SPI_MISO_GPIO 19
#define SPI_SCLK_GPIO 18

#define SPI_HOST_USED SPI3_HOST

/*
 * Track the lifecycle of the physical SPI bus.
 *
 * This prevents accidental operations such as:
 *
 *   init -> init
 *
 * or:
 *
 *   deinit -> deinit
 */
static bool bus_initialized = false;

esp_err_t spi_bus_init(void)
{
    /*
     * Do not initialize the same bus twice through
     * this abstraction.
     */
    if (bus_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    spi_bus_config_t bus_config = {
        .mosi_io_num = SPI_MOSI_GPIO,
        .miso_io_num = SPI_MISO_GPIO,
        .sclk_io_num = SPI_SCLK_GPIO,

        .quadwp_io_num = -1,
        .quadhd_io_num = -1,

        .max_transfer_sz = 0,
    };

    esp_err_t result = spi_bus_initialize(
        SPI_HOST_USED,
        &bus_config,
        SPI_DMA_CH_AUTO);

    if (result == ESP_OK)
    {
        bus_initialized = true;
    }

    return result;
}

esp_err_t spi_bus_deinit(void)
{
    /*
     * Reject deinitialization if this abstraction
     * has not initialized the bus.
     */
    if (!bus_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /*
     * ESP-IDF will reject freeing the bus if
     * registered devices still remain.
     */
    esp_err_t result = spi_bus_free(
        SPI_HOST_USED);

    if (result == ESP_OK)
    {
        bus_initialized = false;
    }

    return result;
}

esp_err_t spi_device_register(
    int cs_gpio,
    int clock_speed_hz,
    uint8_t mode,
    spi_device_handle_t *device_handle)
{
    /*
     * Device registration requires an initialized
     * physical SPI bus.
     */
    if (!bus_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (device_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * Avoid accidentally overwriting an existing
     * handle supplied by the caller.
     */
    if (*device_handle != NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /*
     * Basic device configuration validation.
     */
    if (cs_gpio < 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (clock_speed_hz <= 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (mode > 3)
    {
        return ESP_ERR_INVALID_ARG;
    }

    spi_device_interface_config_t device_config = {
        .clock_speed_hz = clock_speed_hz,
        .mode = mode,
        .spics_io_num = cs_gpio,
        .queue_size = 1,
    };

    return spi_bus_add_device(
        SPI_HOST_USED,
        &device_config,
        device_handle);
}

esp_err_t spi_device_unregister(
    spi_device_handle_t device_handle)
{
    if (!bus_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (device_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    return spi_bus_remove_device(
        device_handle);
}

esp_err_t spi_bus_write(
    spi_device_handle_t device_handle,
    const uint8_t *tx_data,
    size_t data_length)
{
    if (!bus_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (device_handle == NULL ||
        tx_data == NULL ||
        data_length == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    spi_transaction_t transaction;

    memset(
        &transaction,
        0,
        sizeof(transaction));

    transaction.length =
        data_length * 8;

    transaction.tx_buffer = tx_data;

    return spi_device_transmit(
        device_handle,
        &transaction);
}

esp_err_t spi_bus_read(
    spi_device_handle_t device_handle,
    uint8_t *rx_data,
    size_t data_length)
{
    if (!bus_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (device_handle == NULL ||
        rx_data == NULL ||
        data_length == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    spi_transaction_t transaction;

    memset(
        &transaction,
        0,
        sizeof(transaction));

    transaction.length =
        data_length * 8;

    transaction.rxlength =
        data_length * 8;

    transaction.rx_buffer =
        rx_data;

    return spi_device_transmit(
        device_handle,
        &transaction);
}

esp_err_t spi_bus_transfer(
    spi_device_handle_t device_handle,
    const uint8_t *tx_data,
    uint8_t *rx_data,
    size_t data_length)
{
    if (!bus_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (device_handle == NULL ||
        tx_data == NULL ||
        rx_data == NULL ||
        data_length == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    spi_transaction_t transaction;

    memset(
        &transaction,
        0,
        sizeof(transaction));

    transaction.length =
        data_length * 8;

    transaction.tx_buffer =
        tx_data;

    transaction.rx_buffer =
        rx_data;

    return spi_device_transmit(
        device_handle,
        &transaction);
}