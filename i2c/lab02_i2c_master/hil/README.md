# BMP280 Hardware-in-the-Loop Validation

## Overview

This project demonstrates automated validation of an ESP32-based BMP280 environmental sensor using Python, pytest, UART, and I²C.

The ESP32 firmware exposes a text-based UART command interface. A Python HIL client communicates with the firmware and validates sensor behavior, protocol correctness, robustness, and sensor recovery.

## Hardware

- ESP32 development board
- BMP280 temperature and pressure sensor
- USB cable
- Breadboard and jumper wires

### Wiring

| BMP280 | ESP32 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO21 |
| SCL | GPIO22 |
| CSB | 3V3 |
| SDO | GND |

The tested BMP280 uses I²C address `0x76` and chip ID `0x58`. Pull-up resistors may be required depending on the sensor module.

## Architecture

The validation system has three principal layers:

1. **Test layer:** pytest test cases specify the expected behavior.
2. **Device abstraction:** `SerialDevice` handles UART communication, response parsing, timeouts, and protocol errors.
3. **Firmware and hardware:** ESP32 firmware implements the HIL protocol and communicates with the BMP280 over I²C.

Simulated tests replace the serial connection with fake responses and run without hardware.

## UART Protocol

Baud rate: 115200, 8N1.

Commands are newline-terminated. Responses begin with `HIL:`.

| Command | Successful response |
|---|---|
| `PING` | `HIL:PONG` |
| `GET_STATUS` | `HIL:STATUS=READY` |
| `GET_SENSOR_INFO` | `HIL:SENSOR=BMP280;ID=0x58;ADDR=0x76` |
| `READ_SENSOR` | `HIL:TEMP_C=...;PRESSURE_HPA=...` |
| `REINIT_SENSOR` | `HIL:SENSOR_REINIT=OK` |

The firmware also returns error responses when the sensor is unavailable, an operation fails, or a command is invalid.

## Running Tests

Install the dependencies:

```powershell
python -m pip install -r requirements.txt
```

Run simulated tests:

```powershell
python -m pytest -v -m simulation
```

Run physical HIL tests:

```powershell
python -m pytest -v -m hardware --port COM3
```

Run the complete suite:

```powershell
python -m pytest -v --port COM3
```

Close the ESP-IDF serial monitor before starting hardware tests.

## Test Categories

| Category | Tests |
|---|---:|
| Connectivity | 2 |
| Measurements | 3 |
| Protocol | 3 |
| Recovery | 4 |
| Robustness | 7 |
| Sensor information | 2 |
| Serial client | 7 |
| Total | 28 |

The verified Pass 9.5 run completed with **28 passed in 6.10 seconds** on Windows 11.

## Continuous Integration

The repository uses two complementary testing workflows:

- **Native C tests:** Validate BMP280 compensation algorithms without hardware.
- **Python simulated HIL tests:** Validate host-side UART handling, protocol parsing, timeouts, and error handling without hardware.

Physical HIL tests are executed locally with the ESP32 and BMP280 connected.

## Fault Handling and Recovery

The firmware supports fault-tolerant startup when the BMP280 is absent.

`GET_STATUS` reports `NOT_READY` when the sensor is unavailable. `REINIT_SENSOR` attempts to probe and reinitialize the sensor.

Repeated reinitialization with a connected BMP280 has been verified.

### Known Limitations

- Physical hot-plug recovery without ESP32 reboot has not been verified.
- Recovery is explicitly requested rather than periodically automatic.
- Hardware-dependent tests require a locally connected ESP32.
- Native and simulated CI tests do not substitute for physical HIL validation.
- The Python client uses a simple request-response protocol without sequence identifiers or checksums.

## Development Environment

- ESP-IDF v5.5.5
- ESP32
- BMP280
- Windows 11
- Python 3.14.7
- pytest 9.1.1
- Git and GitHub Actions

## Verification Summary

Pass 9.5: 28/28 tests passed locally.

Pass 9.6: Simulation-only GitHub Actions execution is to be verified after the new workflow is pushed.