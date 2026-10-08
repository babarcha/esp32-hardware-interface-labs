/*
 * CAN/TWAI Lab 04: reusable ESP-IDF TWAI controller abstraction.
 * Manages install/start/stop/uninstall and validates frame boundaries.
 * This layer does not implement a physical CAN transceiver.
 */
#include <stdio.h>
#include <stdbool.h>

#include "twai_bus.h"
#include "driver/twai.h"
#include "freertos/FreeRTOS.h"

#define TWAI_TX_GPIO 21
#define TWAI_RX_GPIO 22

/* Software lifecycle flags track successful driver operations. */
static bool driver_installed = false;
static bool controller_started = false;

esp_err_t twai_bus_init(void)
{
    if (driver_installed)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /* Normal mode requires an external transceiver and a real CAN bus
     * for actual network communication; this lab tests API boundaries. */
    twai_general_config_t general_config =
        TWAI_GENERAL_CONFIG_DEFAULT(
            TWAI_TX_GPIO,
            TWAI_RX_GPIO,
            TWAI_MODE_NORMAL);

    /* All communicating CAN nodes must agree on the nominal bitrate. */
    twai_timing_config_t timing_config =
        TWAI_TIMING_CONFIG_500KBITS();

    /* Accept all identifiers; no application-level filtering here. */
    twai_filter_config_t filter_config =
        TWAI_FILTER_CONFIG_ACCEPT_ALL();

    esp_err_t result = twai_driver_install(
        &general_config,
        &timing_config,
        &filter_config);

    if (result == ESP_OK)
    {
        driver_installed = true;

        printf("TWAI driver installed successfully\n");
        printf("TX      = GPIO%d\n", TWAI_TX_GPIO);
        printf("RX      = GPIO%d\n", TWAI_RX_GPIO);
        printf("Bitrate = 500 kbit/s\n");
        printf("Mode    = NORMAL\n");
    }

    return result;
}

esp_err_t twai_bus_start(void)
{
    if (!driver_installed)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (controller_started)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /* State changes only after the underlying ESP-IDF call succeeds. */
    esp_err_t result = twai_start();

    if (result == ESP_OK)
    {
        controller_started = true;
    }

    return result;
}

esp_err_t twai_bus_stop(void)
{
    if (!driver_installed || !controller_started)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t result = twai_stop();

    if (result == ESP_OK)
    {
        controller_started = false;
    }

    return result;
}

esp_err_t twai_bus_deinit(void)
{
    if (!driver_installed)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /*
     * Do not uninstall the driver while the
     * controller is still running.
     */
    if (controller_started)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t result = twai_driver_uninstall();

    if (result == ESP_OK)
    {
        driver_installed = false;
    }

    return result;
}

esp_err_t twai_bus_transmit(
    const twai_message_t *message,
    uint32_t timeout_ms)
{
    if (!driver_installed || !controller_started)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (message == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * Classical CAN supports a maximum payload
     * of 8 bytes.
     */
    if (message->data_length_code > 8)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * Validate the identifier according to
     * standard/extended frame format.
     */
    if (!message->extd &&
        message->identifier > 0x7FF)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (message->extd &&
        message->identifier > 0x1FFFFFFF)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* Convert milliseconds to FreeRTOS ticks for the blocking API.
     * No valid-frame transmission is exercised by the test application. */
    return twai_transmit(
        message,
        pdMS_TO_TICKS(timeout_ms));
}

esp_err_t twai_bus_receive(
    twai_message_t *message,
    uint32_t timeout_ms)
{
    if (!driver_installed || !controller_started)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (message == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* Receive is meaningful only when a physical CAN peer is present. */
    return twai_receive(
        message,
        pdMS_TO_TICKS(timeout_ms));
}
