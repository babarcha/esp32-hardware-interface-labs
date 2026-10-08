/*
 * SPI Lab 03 — ESP32 SPI Master application.
 *
 * Demonstrates SPI3 bus initialization, registration of a Mode 0 device,
 * a synchronous full-duplex transfer, and orderly resource cleanup.
 * No external peripheral is connected; RX bytes are not sensor readings.
 */
#include <stdio.h>
#include <stdint.h>

#include "esp_err.h"
#include "spi_bus.h"

#define SPI_CS_GPIO 5
#define SPI_CLOCK_HZ 1000000
#define SPI_MODE 0

void app_main(void)
{
    printf("\n");
    printf("SPI Lab 03 - Pass 5: Bus Abstraction\n");
    printf("------------------------------------\n");

    /* Configure the shared physical SPI bus before registering a device. */
    esp_err_t result = spi_bus_init();

    if (result != ESP_OK)
    {
        printf(
            "SPI initialization failed: %s\n",
            esp_err_to_name(result));

        return;
    }

    printf("SPI bus initialized successfully\n");

    /* Each SPI target has its own device handle and chip-select settings. */
    spi_device_handle_t device_handle = NULL;

    result = spi_device_register(
        SPI_CS_GPIO,
        SPI_CLOCK_HZ,
        SPI_MODE,
        &device_handle);

    if (result != ESP_OK)
    {
        printf(
            "SPI device registration failed: %s\n",
            esp_err_to_name(result));

        spi_bus_deinit();
        return;
    }

    printf("SPI device registered successfully\n");

    /*
     * SPI clocks data out on MOSI while sampling MISO simultaneously.
     * This arbitrary payload exercises the driver, not a device protocol.
     */
    uint8_t tx_data[] = {
        0x9A,
        0xBC,
        0xDE};

    uint8_t rx_data[sizeof(tx_data)] = {0};

    result = spi_bus_transfer(
        device_handle,
        tx_data,
        rx_data,
        sizeof(tx_data));

    if (result == ESP_OK)
    {
        printf("SPI transaction completed successfully\n");

        printf("TX: ");

        for (size_t i = 0; i < sizeof(tx_data); i++)
        {
            printf("0x%02X ", tx_data[i]);
        }

        printf("\nRX: ");

        for (size_t i = 0; i < sizeof(rx_data); i++)
        {
            printf("0x%02X ", rx_data[i]);
        }

        printf("\n");
        printf(
            "NOTE: RX data is not meaningful without "
            "a connected SPI peripheral.\n");
    }
    else
    {
        printf(
            "SPI transaction failed: %s\n",
            esp_err_to_name(result));
    }

    /* Remove the device before freeing the shared bus resources. */
    result = spi_device_unregister(device_handle);

    if (result == ESP_OK)
    {
        printf("SPI device removed successfully\n");
    }

    result = spi_bus_deinit();

    if (result == ESP_OK)
    {
        printf("SPI bus released successfully\n");
    }
}
