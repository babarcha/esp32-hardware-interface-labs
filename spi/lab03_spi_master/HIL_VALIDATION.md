# SPI Lab 03 â€” Physical HIL Validation

**Pass:** 10.12
**Target:** ESP32 + BMP280
**Interface:** SPI Mode 0, 1 MHz
**UART:** COM3, 115200 baud
**ESP-IDF:** v5.5.5

## Hardware configuration

| Signal | ESP32 GPIO |
|---|---|
| SCLK | GPIO18 |
| MOSI | GPIO23 |
| MISO | GPIO19 |
| CS | GPIO5 |

## Verification checklist

- [ ] ESP-IDF firmware builds successfully
- [ ] Firmware flashes successfully to ESP32
- [ ] UART startup messages appear
- [ ] `PING` responds
- [ ] `GET_STATUS` responds
- [ ] `GET_SENSOR_INFO` identifies BMP280
- [ ] `READ_SENSOR` returns plausible measurements
- [ ] `REINIT_SENSOR` succeeds
- [ ] Python simulation tests pass
- [ ] Physical Python HIL tests pass
- [ ] SPI signals inspected (optional)

## Evidence

- Git commit:
- Build result:
- Python simulation result:
- Physical HIL result:
- Chip ID:
- Temperature:
- Pressure:
- Observed errors:
- Logic analyzer capture:

## Qualification decision

**Status:** Pending physical verification.

Record actual measurements and test results before marking this pass complete.
