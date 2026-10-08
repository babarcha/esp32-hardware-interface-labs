/*
 * SPI Lab 03 — reusable SPI master bus abstraction.
 *
 * Keeps ESP-IDF host/pin configuration and transaction construction
 * separate from the application. Operations return esp_err_t so callers
 * can distinguish invalid state, invalid arguments, and driver failures.
 */
#include <stdbool.h>
#include <string.h>

#include "spi_bus.h"

#define SPI_MOSI_GPIO 23
#define SPI_MISO_GPIO 19
#define SPI_SCLK_GPIO 18

#define SPI_HOST_USED SPI3_HOST

/* Tracks bus ownership within this module; not a multi-threaded lock. */
static bool bus_initialized = false;

esp_err_t spi_bus_init(void)
{
    /* Reject duplicate initialization through this abstraction. */
    if (bus_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /* Standard single-bit SPI wiring; unused quad-data pins are disabled. */
    spi_bus_config_t bus_config = {
        .mosi_io_num = SPI_MOSI_GPIO,
        .miso_io_num = SPI_MISO_GPIO,
        .sclk_io_num = SPI_SCLK_GPIO,

        .quadwp_io_num = -1,
        .quadhd_io_num = -1,

        .max_transfer_sz = 0,
    };

    /* ESP-IDF selects an available DMA channel for this SPI host. */
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
    /* A bus cannot be freed before it has been initialized. */
    if (!bus_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /* ESP-IDF requires registered devices to be removed first. */
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
    /* Registering a target requires an initialized physical bus. */
    if (!bus_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (device_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* The caller must provide an empty handle to avoid overwriting it. */
    if (*device_handle != NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /* Validate basic device configuration before invoking ESP-IDF. */
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

    /* Chip select, frequency, and CPOL/CPHA mode are per-device settings. */
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

    /* Release the logical target before deinitializing the physical bus. */
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

    /* Clear all optional transaction fields before setting required ones. */
    spi_transaction_t transaction;

    memset(
        &transaction,
        0,
        sizeof(transaction));

    /* ESP-IDF expresses SPI transaction length in bits, not bytes. */
    transaction.length =
        data_length * 8;

    transaction.tx_buffer = tx_data;

    /* Synchronous call: blocks until the transaction is completed. */
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

    /* The master still generates clock pulses when receiving data. */
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

    /* Full duplex: every clocked byte transmits and receives together. */
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
