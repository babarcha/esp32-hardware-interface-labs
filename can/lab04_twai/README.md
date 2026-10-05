# ESP32 CAN/TWAI Lab 04

ESP32 CAN/TWAI interface lab using ESP-IDF.

This lab demonstrates initialization, lifecycle management, CAN frame
construction, reusable driver abstraction, API validation, and preparation
for physical CAN bus testing.

## Platform

- ESP32
- ESP-IDF v5.5.5
- TWAI controller
- 500 kbit/s
- TX: GPIO21
- RX: GPIO22

## Architecture

Application (`main.c`)
        |
        v
TWAI abstraction (`twai_bus.c/.h`)
        |
        v
ESP-IDF TWAI driver
        |
        v
ESP32 TWAI controller
        |
        v
External CAN transceiver
        |
        v
CANH / CANL
        |
        v
CAN bus

## Implemented Features

### Controller lifecycle

- Driver installation
- Controller start
- Controller stop
- Driver uninstall

### CAN frame support

The lab uses ESP-IDF `twai_message_t`.

Example frame:

- Identifier: `0x123`
- Format: Standard 11-bit
- Type: Data frame
- DLC: 3
- Payload: `AA BB CC`

### TWAI abstraction

The application uses:

```c
twai_bus_init();
twai_bus_start();

twai_bus_transmit(&message, timeout_ms);
twai_bus_receive(&message, timeout_ms);

twai_bus_stop();
twai_bus_deinit();