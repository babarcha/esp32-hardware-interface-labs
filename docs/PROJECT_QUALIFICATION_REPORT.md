# ESP32 Hardware Interface Labs — Project Qualification Report

**Repository:** https://github.com/babarcha/esp32-hardware-interface-labs  
**Assessment date:** 2026-10-22  
**Scope:** UART Lab 01, I2C Lab 02, SPI Lab 03, CAN/TWAI Lab 04  
**Assessment type:** Host-native software qualification with separately reported physical HIL evidence.

## 1. Executive summary

All four labs passed the native test executions reported in the qualification session. This supports **native software qualification**, not full physical hardware qualification. UART and I2C have prior physical verification evidence. SPI and CAN/TWAI physical bus validation remain pending.

The figures below count different granularities (individual tests, CTest executables, and test groups), so the aggregate of 30 is a count of **reported units**, not 30 identically scoped test cases.

## 2. Qualification matrix

| Lab | Native qualification result | Physical validation status | Qualification conclusion |
|---|---|---|---|
| UART 01 | 11 native protocol tests passed; exit code 0 | Prior UART command exchange verified | Native qualified; prior physical evidence |
| I2C 02 | 3/3 CTest executables passed | BMP280 physical pytest 16/16; SDA disconnect and post-restart recovery verified | Native qualified; scoped physical HIL verified |
| SPI 03 | 2/2 CTest executables passed | Physical SPI BMP280 test pending | Native qualified; hardware qualification pending |
| CAN/TWAI 04 | 14 native test groups passed; exit code 0 | Transceiver and second CAN node required | Native qualified; hardware qualification pending |

## 3. Evidence and exact reproduction commands

Run these commands from the repository root in PowerShell, using MSYS2 UCRT64 GCC available on PATH. The `qualification-build` folder must exist before compiling executables into it.

### 3.1 UART

```powershell
New-Item -ItemType Directory -Force qualification-build

gcc -std=c11 -Wall -Wextra -Werror -pedantic `
  -I uart/lab01_uart_loopback/main `
  uart/lab01_uart_loopback/main/protocol.c `
  uart/lab01_uart_loopback/tests/native/test_protocol.c `
  -o qualification-build/uart_protocol_tests.exe

.\qualification-build\uart_protocol_tests.exe
$LASTEXITCODE
```

**Observed:** `PASS: 11 native UART protocol tests`; exit code `0`.

### 3.2 I2C

```powershell
cmake --build i2c/lab02_i2c_master/build-coverage
ctest --test-dir i2c/lab02_i2c_master/build-coverage --output-on-failure
```

**Observed:** `ninja: no work to do.` and all three CTest executables passed: `bmp280_compensation`, `i2c_bus`, `sensor_manager`.

Prior local coverage, limited to the three instrumented production modules: **98.7% line (147/149), 95.7% branch (67/70), 100% function (15/15)**. These are previously generated coverage measurements, not a new GCOVR measurement from the project-wide run.

For detailed HIL evidence, see [I2C Lab 02 Verification Report](../i2c/lab02_i2c_master/docs/I2C_LAB02_VERIFICATION.md).

### 3.3 SPI

```powershell
cmake --build spi/lab03_spi_master/build-native
ctest --test-dir spi/lab03_spi_master/build-native --output-on-failure
```

**Observed:** `spi_bus_native` and `bmp280_spi_native` passed (2/2). The existing build directory used **MinGW Makefiles**; a prior attempt to reconfigure that same directory with **Ninja** produced a generator mismatch. This is a build-directory configuration issue, not a test failure. Use a fresh directory to change generators.

### 3.4 CAN/TWAI

```powershell
New-Item -ItemType Directory -Force qualification-build

gcc -std=c11 -Wall -Wextra -Werror -pedantic `
  -I can/lab04_twai/tests/native/mocks `
  -I can/lab04_twai/main `
  can/lab04_twai/main/twai_bus.c `
  can/lab04_twai/tests/native/test_twai_bus.c `
  -o qualification-build/twai_native_tests.exe

.\qualification-build\twai_native_tests.exe
$LASTEXITCODE
```

**Observed:** 14 native test groups passed; exit code `0`. Groups include lifecycle, failure/retry, null message, DLC, standard/extended identifier bounds, and TX/RX forwarding/error paths. Driver behavior is mocked; no physical CAN bus communication is established by these tests.

## 4. Continuous integration

The repository contains these relevant workflow definitions:

- `.github/workflows/uart-native-tests.yml`
- `.github/workflows/i2c-native-tests.yml`
- `.github/workflows/i2c-native-coverage.yml`
- `.github/workflows/spi-native-tests.yml`
- `.github/workflows/spi-lab03-ci.yml`
- `.github/workflows/twai-native-tests.yml`

**Independently observed earlier:** I2C native coverage workflow run https://github.com/babarcha/esp32-hardware-interface-labs/actions/runs/39789894330 succeeded and uploaded a coverage artifact. **Not yet established in this assessment:** that every workflow passed on the latest commit. Hosted workflows are not a substitute for physical HIL tests.

## 5. Open qualification gaps

1. SPI: perform and record physical SPI transfers with a connected compatible device.
2. CAN/TWAI: acquire suitable CAN transceiver(s) and a second node; validate actual bus traffic and errors.
3. Firmware: repeat all four ESP-IDF firmware builds at the final qualification revision.
4. CI: verify the latest runs of all relevant GitHub Actions workflows.
5. I2C: improve diagnostic command/response correlation before treating the ad hoc fault script as an automated acceptance test.
6. I2C: no independent BMP280 measurement calibration and no demonstrated live recovery without restart.
7. Repository hygiene: review `.gitignore`, preserve useful HIL scripts, and exclude generated `qualification-build` output.

## 6. Qualification decision

**Decision: PASS — host-native software qualification for four labs.**

**Physical hardware qualification: PARTIAL.** UART and I2C have scoped physical evidence; SPI and CAN/TWAI are pending physical validation. This is an educational/portfolio qualification record, not a production safety certification.

## 7. Interview-ready summary

> I developed and tested four ESP32 peripheral labs covering UART, I2C, SPI and CAN/TWAI. All four passed host-native unit testing using GCC and mocked hardware dependencies where appropriate. I also implemented physical UART and I2C verification, including a 16-case BMP280 HIL regression and an SDA fault-injection experiment. I integrated GitHub Actions workflows and scoped code coverage reporting. I explicitly distinguish software-level test results from physical bus qualification, which remains pending for SPI and CAN.
