# ESP32 SPI Lab 03 â€” BMP280 Sensor and HIL Validation

## Overview

An embedded firmware and verification lab built using ESP32, ESP-IDF, C, Python, CMake, pytest and GitHub Actions.

The project demonstrates SPI master communication with a BMP280 environmental sensor, modular firmware development, native unit testing, simulated Hardware-in-the-Loop (HIL) testing, physical HIL test support, fault injection and automated code coverage qualification.

**Hardware:** ESP32 + BMP280 SPI breakout
**Framework:** ESP-IDF v5.5.5
**Language:** C11 firmware and Python validation
**SPI:** SPI3_HOST, Mode 0, 1 MHz
**UART:** 115200 baud, 8N1

## 1. Learning objectives

- Configure an ESP32 SPI master and register a peripheral
- Implement synchronous SPI read, write and full-duplex transfers
- Separate bus-level and sensor-level responsibilities
- Read and validate BMP280 identification and calibration data
- Implement temperature and pressure compensation
- Handle SPI failures, invalid responses and conversion timeouts
- Build host-native tests using mocked hardware dependencies
- Implement a UART protocol for automated HIL testing
- Enforce source coverage thresholds through CI/CD

## 2. Firmware architecture

```text
main.c
  |
  +-- spi_bus.c
  |     +-- ESP-IDF SPI master driver
  |             +-- ESP32 SPI3_HOST
  |                     +-- BMP280
  |
  +-- bmp280_spi.c
  |     +-- Chip identification
  |     +-- Calibration
  |     +-- Forced measurement
  |     +-- Compensation
  |
  +-- hil_uart.c
        +-- UART commands
        +-- Sensor status
        +-- Measurements
        +-- Reinitialization
```

The BMP280 driver uses the SPI abstraction instead of directly constructing ESP-IDF SPI transactions.

## 3. Hardware configuration

| Signal | ESP32 | BMP280 |
|---|---|---|
| MOSI | GPIO23 | SDI |
| MISO | GPIO19 | SDO |
| SCLK | GPIO18 | SCK |
| CS | GPIO5 | CSB |
| Power | 3V3 | 3.3 V-compatible supply |
| Ground | GND | GND |

Use a BMP280 breakout that supports four-wire SPI. Confirm the module's electrical specifications before connecting power.

## 4. SPI protocol

The BMP280 uses register-based SPI communication.

For register reads, the firmware sets the read bit in the command byte, clocks additional bytes and discards the command-phase received byte.

For writes, it clears the read bit and transmits the register address followed by the value.

SPI transactions use ESP-IDF's synchronous device-transmit API.

## 5. Sensor operation

The measurement workflow is:

1. Verify BMP280 chip ID (`0x58`).
2. Read calibration coefficients.
3. Validate essential calibration values.
4. Trigger a forced measurement.
5. Poll sensor status until ready or timeout.
6. Read raw pressure and temperature registers.
7. Reject invalid ADC markers.
8. Apply Bosch compensation formulas.
9. Return temperature in Â°C and pressure in hPa.

## 6. UART HIL protocol

Commands are newline-terminated ASCII strings. Responses begin with `RSP:`.

| Command | Example response |
|---|---|
| PING | RSP:PONG |
| GET_STATUS | RSP:STATUS=READY |
| GET_SENSOR_INFO | RSP:SENSOR=BMP280;BUS=SPI;CHIP_ID=0x58 |
| READ_SENSOR | RSP:TEMP_C=25.08;PRESSURE_HPA=1006.53 |
| REINIT_SENSOR | RSP:REINIT=OK |

Example measurement values are illustrative, not actual hardware readings.

Error responses include sensor-not-ready, measurement-failed, reinitialization-failed and unknown-command cases.

## 7. Build and flash

Load the ESP-IDF environment, then run:

```powershell
cd spi/lab03_spi_master
idf.py build
idf.py -p COM3 flash
idf.py -p COM3 monitor
```

Exit the serial monitor with `Ctrl+]` before running serial HIL tests.

## 8. Native C testing

Native tests compile production C source files with mocked hardware dependencies.

The tests cover SPI bus lifecycle, argument validation, sensor initialization, register transactions, calibration, compensation, conversion polling, timeouts, fault injection and recovery.

The project uses CMake and CTest to run the native test executables.

## 9. Coverage qualification

On the configured Windows development environment, run from the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File `
  .\spi\lab03_spi_master\scripts\check_coverage.ps1
```

Minimum accepted production coverage:

| Metric | Gate |
|---|---:|
| Line coverage | 90% |
| Branch coverage | 80% |

Pass 10.10 measured 97.8% line coverage, 87.1% branch coverage and 100% function coverage. Consult the latest CI artifact for current measurements.

## 10. Python validation

The Python validation framework supports simulated and physical serial-device execution.

From the `hil` directory:

```powershell
python -m pip install -r requirements.txt
python -m pytest --device sim -v
python -m pytest --device serial --port COM3 -v
```

The serial tests require connected ESP32 hardware and an available serial port.

## 11. GitHub Actions

The SPI Lab 03 CI workflow runs:

- Python simulation tests
- Native C tests
- Production code coverage qualification

Coverage and test reports are retained as GitHub Actions artifacts.

Physical hardware tests are not executed by the standard cloud runner.

## 12. Release qualification

See `RELEASE_QUALIFICATION.md` and `HIL_VALIDATION.md`.

Automated CI is successful, but physical HIL qualification remains pending until actual hardware results have been recorded.

## 13. Engineering limitations

- The SPI bus initialization flag is not a concurrency lock.
- UART sensor readiness is based on initialization state.
- Native mocks do not reproduce all physical timing and electrical conditions.
- Coverage alone does not establish functional correctness.
- Physical SPI waveform verification requires hardware instrumentation.

## 14. Skills demonstrated

Embedded C, ESP-IDF, SPI, sensor drivers, error propagation, calibration and fixed-point compensation, CMake, native unit testing, fault injection, pytest, HIL testing, GitHub Actions, GCOV and GCOVR.
