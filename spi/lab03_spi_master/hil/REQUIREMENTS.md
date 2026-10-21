
# SPI Lab 03 — Verification Requirements

## 1. Scope

This document defines verification requirements for
the ESP32 BMP280 SPI hardware-interface lab.

Verification is divided into three environments:

1. SIM — Python protocol simulation.
2. NATIVE — Production C code compiled with mocked
   hardware dependencies.
3. HIL — Physical ESP32 and BMP280 integration tests.

These environments provide complementary evidence.

## 2. Python Protocol and HIL Requirements

| ID | Requirement | Verification |
|---|---|---|
| SPI-REQ-001 | PING shall return PONG. | test_ping |
| SPI-REQ-002 | Initialized device shall report STATUS=READY. | test_status |
| SPI-REQ-003 | Device shall report BMP280, SPI and chip ID 0x58. | test_sensor_info |
| SPI-REQ-004 | Temperature shall be finite and within -40 to 85 C. | test_temperature |
| SPI-REQ-005 | Pressure shall be finite and within 300 to 1100 hPa. | test_pressure |
| SPI-REQ-006 | Measurements shall contain TEMP_C and PRESSURE_HPA. | test_measurement_format |
| SPI-REQ-007 | Unknown commands shall return ERROR=UNKNOWN_COMMAND. | test_unknown_command |
| SPI-REQ-008 | Empty commands shall not disrupt later communication. | test_empty_command_does_not_break_device |
| SPI-REQ-009 | Sensor reinitialization shall be supported. | test_reinitialize_sensor |
| SPI-REQ-010 | Measurements shall work after reinitialization. | test_measurement_after_reinitialization |
| SPI-REQ-011 | Oversized commands shall return ERROR=COMMAND_TOO_LONG. | test_command_too_long |
| SPI-REQ-012 | Repeated PING commands shall receive valid responses. | test_repeated_ping |

## 3. Native SPI and BMP280 Requirements

| ID | Requirement | Verification |
|---|---|---|
| SPI-REQ-013 | SPI bus shall reject invalid states and arguments. | test_spi_bus |
| SPI-REQ-014 | SPI bus shall propagate driver errors. | test_spi_bus |
| SPI-REQ-015 | BMP280 driver shall reject incorrect chip IDs. | test_wrong_chip_id |
| SPI-REQ-016 | BMP280 driver shall reject invalid calibration. | test_calibration_failure |
| SPI-REQ-017 | BMP280 driver shall propagate SPI read/write failures. | test_spi_read_failure, test_write_failure |
| SPI-REQ-018 | Measurement shall require loaded calibration. | test_measurement_requires_calibration |
| SPI-REQ-019 | Temperature compensation shall match a reference vector. | test_reference_compensation |
| SPI-REQ-020 | Pressure compensation shall match a reference vector. | test_reference_compensation |
| SPI-REQ-021 | Driver shall wait for measurement readiness. | test_sensor_busy_then_ready |
| SPI-REQ-022 | Driver shall time out if the sensor remains busy. | test_sensor_busy_timeout |
| SPI-REQ-023 | Measurement SPI failures shall be propagated. | test_status_read_failure, test_measurement_data_read_failure |
| SPI-REQ-024 | Invalid ADC readings shall be rejected. | test_invalid_adc |
| SPI-REQ-025 | A new measurement may succeed after a transient SPI failure. | test_recovery_after_spi_failure |

## 4. Verification Environments

### 4.1 SIM — Python simulation

Run:

    python -m pytest --device sim -v

Expected test count:

    12 Python tests

This verifies the modeled UART command protocol.

### 4.2 NATIVE — Host C unit testing

Compile production source files:

- main/spi_bus.c
- main/bmp280_spi.c

Hardware dependencies are replaced by mocks.

Run:

    cmake --build spi/lab03_spi_master/build-native

    ctest --test-dir spi/lab03_spi_master/build-native --output-on-failure

Expected:

- 12 SPI bus native test groups
- 19 BMP280 native test groups
- 2 passing CTest targets

### 4.3 HIL — Physical hardware testing

Hardware:

- ESP32
- BMP280 sensor
- SPI3 bus
- UART0 / COM3
- 115200 baud

Run:

    python -m pytest --device serial --port COM3 -v

Expected:

    12 Python HIL tests

## 5. Reference Compensation Vector

Calibration coefficients:

| Coefficient | Value |
|---|---:|
| dig_T1 | 27504 |
| dig_T2 | 26435 |
| dig_T3 | -1000 |
| dig_P1 | 36477 |
| dig_P2 | -10685 |
| dig_P3 | 3024 |
| dig_P4 | 2855 |
| dig_P5 | 140 |
| dig_P6 | -7 |
| dig_P7 | 15500 |
| dig_P8 | -14600 |
| dig_P9 | 6000 |

Raw ADC values:

- adc_T = 519888
- adc_P = 415148

Expected compensated results:

- Temperature approximately 25.08 C
- Pressure approximately 1006.53 hPa

These are reference algorithm values, not live
environmental measurements.

## 6. Fault-Injection Coverage

The native BMP280 tests inject:

- Incorrect chip identification
- SPI read failure
- SPI write failure
- Invalid calibration data
- Calibration read failure
- Sensor busy followed by ready
- Sensor permanently busy
- Status-register read failure
- ADC data read failure
- Invalid raw ADC value
- Transient SPI failure followed by recovery

Fault injection occurs through mocked SPI
functions and register values.

## 7. Limitations

Passing SIM tests does not establish physical
hardware correctness.

Passing NATIVE tests does not establish physical
SPI timing, signal integrity, or sensor accuracy.

Passing HIL tests does not establish calibrated
measurement accuracy across environmental
conditions or long-term reliability.

Recovery testing confirms that a subsequent
caller-initiated measurement can succeed.
It does not imply automatic driver retries.

## 8. Release Criteria

- All 12 Python simulation tests pass.
- Both native CTest targets pass.
- All 31 native C test groups pass.
- All 12 physical HIL tests pass.
- ESP-IDF firmware build succeeds.
- GitHub Actions simulation and native jobs pass.
- No unresolved critical firmware defects.
- Test evidence and limitations are recorded.
- Changes are committed to version control.
