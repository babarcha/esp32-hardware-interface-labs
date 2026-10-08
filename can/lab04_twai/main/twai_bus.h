/*
 * CAN/TWAI Lab 04 public interface.
 * API errors use esp_err_t; message layout is ESP-IDF twai_message_t.
 * Call order: init -> start -> transmit/receive -> stop -> deinit.
 */
#ifndef TWAI_BUS_H
#define TWAI_BUS_H

#include "esp_err.h"
#include "driver/twai.h"

/* Install the driver: ESP_ERR_INVALID_STATE if already installed. */
esp_err_t twai_bus_init(void);
/* Start an installed, stopped controller. */
esp_err_t twai_bus_start(void);
/* Stop a running controller. */
esp_err_t twai_bus_stop(void);
/* Uninstall the driver only after the controller is stopped. */
esp_err_t twai_bus_deinit(void);

/*
 * Validate and enqueue a classical CAN frame for transmission.
 * message: non-NULL frame; DLC <= 8 and identifier must fit its format.
 * timeout_ms: queue wait converted to FreeRTOS ticks.
 * Requires an installed and started controller.
 */
esp_err_t twai_bus_transmit(
    const twai_message_t *message,
    uint32_t timeout_ms);

/*
 * Receive a frame from the driver queue into non-NULL message.
 * timeout_ms: receive wait converted to FreeRTOS ticks.
 * Requires an installed and started controller.
 */
esp_err_t twai_bus_receive(
    twai_message_t *message,
    uint32_t timeout_ms);

#endif
