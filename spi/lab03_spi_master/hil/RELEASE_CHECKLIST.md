
# SPI Lab 03 — Release Verification Checklist

## Build
- [ ] ESP-IDF firmware builds successfully.
- [ ] Firmware flashes successfully to the ESP32.
- [ ] No unexpected runtime errors.

## Simulation
- [ ] All 12 simulation pytest tests pass.
- [ ] JUnit XML report generated.
- [ ] GitHub Actions CI succeeds.

## Physical HIL
- [ ] ESP32 connected through UART.
- [ ] BMP280 connected through SPI.
- [ ] BMP280 chip ID is 0x58.
- [ ] All 12 physical HIL tests pass.
- [ ] Temperature reading is plausible.
- [ ] Pressure reading is plausible.
- [ ] Invalid-command handling verified.
- [ ] Reinitialization verified.
- [ ] JUnit XML report generated.

## Traceability
- [ ] Requirements reviewed.
- [ ] Every requirement has a corresponding test.
- [ ] No unresolved critical defects.

## Release evidence
- Firmware Git commit:
- ESP-IDF version:
- ESP32 board:
- Sensor module:
- Test date:
- Simulation result:
- Physical HIL result:
- GitHub Actions run URL:
- Known limitations:

## Approval
- [ ] Release criteria satisfied.
