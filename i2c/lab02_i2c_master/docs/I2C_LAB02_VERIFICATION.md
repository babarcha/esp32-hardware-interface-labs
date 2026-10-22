# IÂ²C Lab 02 â€” Verification and Validation Report

**Project:** ESP32 Hardware Interface Labs
**Repository:** https://github.com/babarcha/esp32-hardware-interface-labs
**Component:** `i2c/lab02_i2c_master`
**Report date:** 2026-10-22
**Status:** Native tests, local physical HIL, fault injection and GitHub Actions CI verified; limitations below.

## 1. Purpose and scope

This lab demonstrates modular ESP32 IÂ²C master firmware interfacing with a BMP280 temperature/pressure sensor, together with native C unit tests, Python hardware-in-the-loop (HIL) tests, fault injection, coverage analysis and automated CI.

The report distinguishes **observed test evidence** from **untested properties**. Native test coverage is scoped to the three production C modules instrumented by the native build; it is not a measurement of the entire ESP-IDF firmware.

## 2. Hardware configuration

| ESP32 connection | BMP280 module connection | Purpose |
|---|---|---|
| 3V3 | VCC | 3.3 V power |
| GND | GND | Common ground |
| GPIO21 | SDA | IÂ²C data |
| GPIO22 | SCL | IÂ²C clock |
| 3V3 | CSB | Select IÂ²C mode (module-dependent pin behavior) |
| GND | SDO | Select 7-bit address `0x76` |

**Observed sensor:** BMP280, chip ID `0x58`, IÂ²C address `0x76`. **Configured bus:** IÂ²C controller 0, 100 kHz. The firmware enables ESP32 internal pull-ups; suitable external pull-ups depend on the breakout board and bus wiring. Use 3.3 V-compatible connections. Disconnect USB power before changing jumper wires.

## 3. Firmware architecture

```text
UART0 host / Python pytest
           |
           v
     HIL protocol
           |
           v
    Sensor manager
       |       \
       v        v
   BMP280 driver   Sensor readiness/reinitialization
       |
       v
     IÂ²C bus abstraction
       |
       v
  ESP-IDF IÂ²C master driver
       |
       v
     ESP32 GPIO21/22 -> BMP280
```

`main.c` initializes the sensor manager, creates the IÂ²C master bus, attempts sensor initialization, initializes the UART HIL protocol and enters the protocol loop. `i2c_bus.c` wraps ESP-IDF IÂ²C master operations: initialization, probing, scanning, register reads/writes and teardown. `sensor_manager.c` tracks sensor readiness and attempts the supported low/high IÂ²C addresses during reinitialization. `bmp280_compensation.c` contains numeric compensation logic tested on the host.

The serial command protocol includes `PING`, `GET_STATUS`, `GET_SENSOR_INFO`, `READ_SENSOR`, `REINIT_SENSOR` and an unknown-command error response.

## 4. Verification approach

| Level | Test environment | Primary evidence |
|---|---|---|
| Native C unit tests | Windows, GCC 16.2.0, CMake/Ninja/CTest | 3/3 CTest executables passed |
| IÂ²C bus native mocks | Host executable | 12/12 test groups passed |
| Sensor manager native mocks | Host executable | 10/10 test groups passed |
| Physical HIL regression | Windows Python 3.11, pytest, PySerial, ESP32 COM3, BMP280 | 16/16 cases passed in 4.40 s |
| Physical fault injection | Disconnect SDA, keep other connections | Unavailable sensor and graceful error responses |
| Recovery after reconnect/restart | Restore SDA, restart ESP32 | Ready state, identity, measurements, successful reinitialization |
| GitHub Actions CI | Ubuntu hosted runner | Workflow run #1 successful, coverage artifact uploaded |

### 4.1 Native unit tests

The three CTest targets are `bmp280_compensation`, `i2c_bus` and `sensor_manager`. The native IÂ²C bus tests exercise initialization, invalid arguments, address probing and scanning, register transfers, error paths and cleanup. The sensor manager tests exercise absent sensors, address fallback, initialization failures, marking unavailable and mocked recovery.

**Local commands (normal native build):**

```powershell
cmake -S tests -B build-native -G Ninja
cmake --build build-native
ctest --test-dir build-native --output-on-failure
```

### 4.2 Physical Python HIL tests

Run with the ESP32 connected on COM3 and serial monitor closed:

```powershell
$env:ESP32_PORT = "COM3"
python -m pytest .\hil_tests\test_bmp280_hil.py -v --tb=short
```

The 16 test cases cover UART ping, ready status, BMP280 identity, valid measurement formatting and broad sanity limits, 10 repeated measurements, sensor reinitialization and unknown-command handling. These limits are **sanity checks, not calibration tolerances**.

### 4.3 Physical fault-injection experiment

1. Disconnect USB power, then disconnect only GPIO21â†’SDA; retain power, ground, SCL, CSB and SDO connections.
2. Reconnect USB and run the diagnostic with the sensor data line disconnected.
3. Observed responses:

```text
PING                 -> HIL:PONG
GET_STATUS           -> HIL:STATUS=NOT_READY
GET_SENSOR_INFO      -> HIL:ERROR=SENSOR_NOT_READY
READ_SENSOR          -> HIL:ERROR=SENSOR_NOT_READY
REINIT_SENSOR        -> HIL:ERROR=SENSOR_REINIT_FAILED
GET_STATUS           -> HIL:STATUS=NOT_READY
READ_SENSOR          -> HIL:ERROR=SENSOR_NOT_READY
```

4. Disconnect USB power, restore SDA, reconnect USB and rerun the diagnostic.
5. Observed recovery:

```text
PING                 -> HIL:PONG
GET_STATUS           -> HIL:STATUS=READY
GET_SENSOR_INFO      -> HIL:SENSOR=BMP280;ID=0x58;ADDR=0x76
READ_SENSOR          -> HIL:TEMP_C=22.72;PRESSURE_HPA=1023.78
REINIT_SENSOR        -> HIL:SENSOR_REINIT=OK
GET_STATUS           -> HIL:STATUS=READY
READ_SENSOR          -> HIL:TEMP_C=22.73;PRESSURE_HPA=1023.82
```

A subsequent physical pytest regression passed **16/16**. This establishes recovery **after reconnecting the wire and restarting the ESP32**, not autonomous live recovery without restarting the MCU.

**Diagnostic caveat:** One intermediate diagnostic run returned `HIL:PONG` for all commands. Later correct responses and pytest success did not reproduce it. The ad hoc diagnostic accepts the first `HIL:` line without validating its command association; improve response synchronization before using it as a strict automated acceptance test.

## 5. Native code coverage

**Local environment:** MSYS2 UCRT64 GCC/gcov 16.2.0; GCOVR 8.7. Coverage-enabled build:

```powershell
cmake -S tests -B build-coverage -G Ninja `
  -DCMAKE_C_COMPILER=C:/msys64/ucrt64/bin/gcc.exe `
  -DENABLE_COVERAGE=ON
cmake --build build-coverage
ctest --test-dir build-coverage --output-on-failure
python -m gcovr --root . --object-directory build-coverage `
  --gcov-executable C:/msys64/ucrt64/bin/gcov.exe `
  --filter 'main/.*\.c$' --exclude 'main/main\.c$' --txt
```

| Module | Line coverage | Branch coverage | Function coverage |
|---|---:|---:|---:|
| `bmp280_compensation.c` | 48/49 (98.0%) | 13/14 (92.9%) | 3/3 (100%) |
| `i2c_bus.c` | 62/63 (98.4%) | 36/38 (94.7%) | 7/7 (100%) |
| `sensor_manager.c` | 37/37 (100%) | 18/18 (100%) | 5/5 (100%) |
| **Total** | **147/149 (98.7%)** | **67/70 (95.7%)** | **15/15 (100%)** |

Unexecuted source lines reported locally: `bmp280_compensation.c:160` and `i2c_bus.c:73`. The numbers apply **only to these three instrumented production files**. They exclude ESP-IDF drivers, the firmware entry point, UART protocol implementation and other code not linked into the native test executables.

## 6. Continuous integration

**Workflow:** `.github/workflows/i2c-native-coverage.yml`
**Verified run:** https://github.com/babarcha/esp32-hardware-interface-labs/actions/runs/39789894330
**Outcome:** Successful first run on `main`, approximately 25 seconds total; uploaded `i2c-native-coverage` artifact (18.7 KB).

The workflow checks out the repository, installs host build/coverage tools, configures the coverage-enabled native build, compiles, executes CTest, generates GCOVR HTML output and uploads it as an artifact. Hosted CI **does not run physical COM3/ESP32 HIL tests**.

A non-fatal GitHub Actions annotation reported Node.js 20 deprecation for an action dependency. Update the affected action to a supported release after checking the action's current compatibility and then verify the workflow again.

## 7. Reproduction checklist

- [ ] Confirm BMP280 3.3 V wiring and common ground.
- [ ] Confirm ESP32 enumerates on the intended COM port.
- [ ] Run the three native CTest executables.
- [ ] Generate local GCOVR HTML coverage report.
- [ ] Run physical pytest HIL tests with the serial monitor closed.
- [ ] If testing faults, power off before disconnecting/reconnecting SDA.
- [ ] Inspect the GitHub Actions workflow run and download its coverage artifact.
- [ ] Record exact tool versions, results, dates and any deviations.

## 8. Known limitations and future work

1. **No live hot-recovery claim:** Recovery without MCU reset has not been physically verified.
2. **No independent sensor calibration:** Plausible temperature/pressure values do not establish metrological accuracy.
3. **Limited coverage scope:** Native coverage does not include the entire firmware or ESP-IDF.
4. **UART diagnostic robustness:** The ad hoc fault diagnostic needs command/response correlation to avoid accepting stale lines.
5. **CI versus physical HIL:** GitHub-hosted CI runs host-native tests only; physical HIL remains a local test stage.
6. **Electrical fault coverage:** Disconnecting SDA is one fault scenario; SCL faults, brownouts, noisy bus, timing stress and long-duration soak tests remain unverified.

## 9. Interview summary

> I built a modular ESP32 IÂ²C interface for a BMP280 sensor, separated hardware access from sensor lifecycle management, and validated the implementation with mocked native C tests and physical Python HIL tests. The three instrumented production modules achieved 98.7% line, 95.7% branch and 100% function coverage locally. I also injected a physical SDA disconnection, verified explicit sensor-unavailable responses while UART remained responsive, and verified operation after restoring the connection and restarting the board. Finally, I automated the native tests and coverage artifact generation using GitHub Actions. I distinguish host unit-test coverage from full firmware coverage and post-restart recovery from autonomous live recovery.
