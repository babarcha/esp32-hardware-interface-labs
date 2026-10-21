
# SPI Lab 03 — HIL Verification Requirements

## Scope

This document defines requirements verified by the
Python HIL test suite for the ESP32 BMP280 SPI lab.

Simulation tests verify the modeled UART protocol.
Physical HIL tests exercise the integrated ESP32,
UART, SPI driver, and BMP280 sensor.

## Requirements

| ID | Requirement | Verification |
|---|---|---|
| SPI-REQ-001 | The device shall respond to PING with PONG. | test_ping |
| SPI-REQ-002 | The initialized device shall report STATUS=READY. | test_status |
| SPI-REQ-003 | The device shall report BMP280, SPI, and chip ID 0x58. | test_sensor_info |
| SPI-REQ-004 | The device shall return a finite temperature within -40 to 85 C. | test_temperature |
| SPI-REQ-005 | The device shall return a finite pressure within 300 to 1100 hPa. | test_pressure |
| SPI-REQ-006 | Sensor measurements shall use TEMP_C and PRESSURE_HPA fields. | test_measurement_format |
| SPI-REQ-007 | Unknown commands shall return ERROR=UNKNOWN_COMMAND. | test_unknown_command |
| SPI-REQ-008 | An empty command shall not prevent subsequent communication. | test_empty_command_does_not_break_device |
| SPI-REQ-009 | The device shall support sensor reinitialization. | test_reinitialize_sensor |
| SPI-REQ-010 | Measurements shall remain available after reinitialization. | test_measurement_after_reinitialization |
| SPI-REQ-011 | Oversized commands shall return ERROR=COMMAND_TOO_LONG. | test_command_too_long |
| SPI-REQ-012 | The device shall respond correctly to repeated PING requests. | test_repeated_ping |

## Verification environments

### SIM
Python SimulatedDevice, no hardware.

### HIL
Physical ESP32 with BMP280 connected through SPI.
UART connection through COM3 at 115200 baud.

## Limitations

Passing simulation tests does not establish physical
hardware correctness.

Passing physical HIL tests does not establish
calibrated sensor accuracy, environmental robustness,
or long-term reliability.

## Release criteria

- All 12 simulation tests pass.
- All 12 physical HIL tests pass on the intended setup.
- GitHub Actions simulation CI succeeds.
- No unresolved critical firmware defects.
- Changes are committed with test evidence recorded.
