# SPI Lab 03 â€” Release Qualification

**Release candidate:** Pass 10.13
**Target:** ESP32 + BMP280 SPI
**Firmware framework:** ESP-IDF v5.5.5

## 1. Verification scope

The release qualification includes:

- Native C unit testing
- Python protocol simulation
- Production source coverage
- Physical ESP32/BMP280 HIL testing
- Firmware build and flash verification
- Documentation and traceability

## 2. Automated quality gates

| Quality gate | Acceptance criterion | Status |
|---|---|---|
| Native C tests | All tests pass | Pending CI |
| Python simulation | All tests pass | Pending CI |
| Line coverage | At least 90% | Passed locally in Pass 10.10 |
| Branch coverage | At least 80% | Passed locally in Pass 10.10 |
| Function coverage | Report actual result | 100% locally in Pass 10.10 |
| GitHub Actions | All required jobs pass | Pending |
| Physical HIL | ESP32 + BMP280 verified | Pending |

## 3. Physical HIL qualification

Physical validation requires evidence of:

- Successful ESP-IDF firmware build and flash
- BMP280 chip identification (`0x58`)
- Successful sensor measurement
- UART protocol communication
- Python serial HIL tests
- Recovery from supported faults

## 4. Release evidence

Record the following:

- Git commit SHA:
- GitHub Actions run URL:
- Native test result:
- Simulation test result:
- Line coverage:
- Branch coverage:
- Coverage report artifact:
- Physical HIL result:
- Firmware build result:

## 5. Release decision

**Current decision: NOT YET QUALIFIED**

Automated CI qualification and physical HIL verification must be completed before declaring the release fully qualified.
