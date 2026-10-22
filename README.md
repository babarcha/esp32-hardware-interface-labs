# ESP32 Hardware Interface & Validation Labs

**Embedded C · ESP-IDF · UART · I²C · SPI · CAN/TWAI · Native Unit Tests · Python HIL · GitHub Actions**

A hands-on embedded software engineering portfolio comprising four ESP32 peripheral labs. The emphasis is on **modular firmware, hardware abstraction, defensive interfaces, reproducible testing, and evidence-based qualification**—not simply successful peripheral initialization.

**Qualification snapshot (22 October 2026):** All four labs passed their reported host-native tests. Physical UART and I²C verification has been demonstrated; physical SPI and CAN/TWAI validation remains pending. See the [Project Qualification Report](docs/PROJECT_QUALIFICATION_REPORT.md) for commands, evidence, and limitations.

## At a glance

| Lab | Interface | Implemented scope | Native verification | Physical verification |
|---|---|---|---|---|
| [01](uart/lab01_uart_loopback/) | UART | Serial command protocol, parsing, host communication | **11 protocol tests passed** | Prior ESP32–host exchange verified |
| [02](i2c/lab02_i2c_master/) | I²C | Bus abstraction, BMP280 sensor integration, status/reinitialization | **3/3 CTest executables passed** | **16/16 BMP280 HIL cases passed**; SDA fault injection and post-restart recovery |
| [03](spi/lab03_spi_master/) | SPI | Bus/device abstraction and BMP280 SPI driver | **2/2 CTest executables passed** | Pending physical SPI sensor test |
| [04](can/lab04_twai/) | CAN/TWAI | Driver lifecycle, frame validation, TX/RX API | **14 native test groups passed** | Pending external transceiver and second CAN node |

> Test counts have different granularities: individual UART cases, CTest executables, and CAN/TWAI test groups. They should not be interpreted as equivalent test-case counts.

## Architecture and verification strategy

```text
                    Developer / GitHub Actions
                              |
                +-------------+-------------+
                |                           |
         Host-native tests             Physical HIL tests
         (GCC, CTest, mocks)          (Python, pytest, serial)
                |                           |
                +-------------+-------------+
                              |
                     Firmware interface API
                              |
            +-----------------+------------------+
            |                 |                  |
       Protocol /        Bus / device       Lifecycle and
       application       abstractions       error handling
            |                 |                  |
            +-----------------+------------------+
                              |
                       ESP-IDF drivers
                              |
                       ESP32 peripherals
                              |
                  Physical bus and device
                  (when connected and tested)
```

The labs separate **host-native software behavior**, **firmware/peripheral execution**, and **electrical bus/device behavior**. A passing mocked unit test does not establish a successful physical transfer. Likewise, controller initialization alone does not prove CAN frame acknowledgment or SPI peripheral communication.

## Repository structure

```text
esp32-hardware-interface-labs/
├── uart/lab01_uart_loopback/
├── i2c/lab02_i2c_master/
├── spi/lab03_spi_master/
├── can/lab04_twai/
├── docs/
│   └── PROJECT_QUALIFICATION_REPORT.md
└── .github/workflows/
```

Each lab maintains its own firmware and validation assets. See the lab directories for their current implementation, tests, and documentation.

## Lab highlights

### UART — protocol and host automation

UART2 uses **TX GPIO17**, **RX GPIO16**, and **115200 baud, 8N1** for the loopback exercise. The command protocol supports `PING`, `GET_INFO`, `GET_COUNTER`, and an explicit unknown-command response. A Python host client and tests exercise command/response behavior. The host-native C protocol suite passed **11 tests**.

### I²C — BMP280 integration and fault handling

I²C controller 0 uses **SDA GPIO21** and **SCL GPIO22**. A connected BMP280 was observed at address **0x76** with chip ID **0x58**. The implementation separates I²C bus operations, sensor logic, and the UART-facing HIL protocol.

Physical Python HIL regression passed **16/16 cases**. A controlled SDA-disconnect experiment verified that the UART interface remained responsive while sensor commands reported unavailable status. Normal readings resumed **after restoring SDA and restarting the ESP32**; autonomous hot recovery without restart has **not** been established.

Prior native coverage for the three instrumented production modules (`bmp280_compensation.c`, `i2c_bus.c`, `sensor_manager.c`) was **98.7% lines**, **95.7% branches**, and **100% functions**. These numbers **do not represent whole-firmware coverage**.

Detailed evidence: [I²C Lab 02 Verification Report](i2c/lab02_i2c_master/docs/I2C_LAB02_VERIFICATION.md).

### SPI — bus abstraction and BMP280 driver

SPI3 configuration: **MOSI GPIO23**, **MISO GPIO19**, **SCLK GPIO18**, **CS GPIO5**. The lab includes native tests for the SPI bus and BMP280 SPI driver; **2/2 CTest executables passed**. A connected SPI peripheral still needs to be tested physically before claiming end-to-end SPI qualification.

### CAN/TWAI — defensive controller lifecycle

The ESP32 TWAI configuration uses **TX GPIO21**, **RX GPIO22**, **500 kbit/s**, and normal mode. Native tests cover installation/start/stop lifecycle, failure/retry behavior, invalid message arguments, DLC and identifier bounds, and mocked transmit/receive paths. **14 native test groups passed**.

Physical CAN requires an appropriate **external transceiver** and **another CAN node**. The native tests do not demonstrate electrical CAN traffic or acknowledgment.

## Reproduce the native qualification tests

Run the following in **PowerShell from the repository root**. Use a compatible GCC toolchain (the recorded Windows runs used MSYS2 UCRT64 GCC) and have CMake/CTest available for the I²C and SPI builds.

### UART

```powershell
New-Item -ItemType Directory -Force qualification-build

gcc -std=c11 -Wall -Wextra -Werror -pedantic `
  -I uart/lab01_uart_loopback/main `
  uart/lab01_uart_loopback/main/protocol.c `
  uart/lab01_uart_loopback/tests/native/test_protocol.c `
  -o qualification-build/uart_protocol_tests.exe

.\qualification-build\uart_protocol_tests.exe
```

### CAN/TWAI

```powershell
New-Item -ItemType Directory -Force qualification-build

gcc -std=c11 -Wall -Wextra -Werror -pedantic `
  -I can/lab04_twai/tests/native/mocks `
  -I can/lab04_twai/main `
  can/lab04_twai/main/twai_bus.c `
  can/lab04_twai/tests/native/test_twai_bus.c `
  -o qualification-build/twai_native_tests.exe

.\qualification-build\twai_native_tests.exe
```

### I²C

```powershell
cmake -S i2c/lab02_i2c_master/tests `
  -B i2c/lab02_i2c_master/build-native `
  -G Ninja `
  -DCMAKE_C_COMPILER=C:/msys64/ucrt64/bin/gcc.exe
cmake --build i2c/lab02_i2c_master/build-native
ctest --test-dir i2c/lab02_i2c_master/build-native --output-on-failure
```

### SPI

```powershell
cmake -S spi/lab03_spi_master/tests/native `
  -B spi/lab03_spi_master/build-ninja `
  -G Ninja `
  -DCMAKE_C_COMPILER=C:/msys64/ucrt64/bin/gcc.exe
cmake --build spi/lab03_spi_master/build-ninja
ctest --test-dir spi/lab03_spi_master/build-ninja --output-on-failure
```

Use a fresh build directory when switching CMake generators; do not reconfigure a `MinGW Makefiles` build directory in place with `Ninja`. The above configuration commands are reproducibility instructions; the recorded SPI pass used an existing build directory.

## Build and flash ESP32 firmware

The labs use **ESP-IDF 5.5.x** (recorded development environment: **5.5.5**), Windows 11, VS Code, CMake/Ninja, Python, and Git.

Open an **ESP-IDF-enabled PowerShell** session, then select a lab. For example:

```powershell
cd can/lab04_twai
idf.py build
idf.py -p COM3 flash
idf.py -p COM3 monitor
```

Replace `COM3` with the actual serial port. Exit the serial monitor using **Ctrl + ]**. ESP-IDF firmware builds and physical HIL runs require the appropriate environment and hardware; the native C tests do not require an ESP32.

## CI and qualification evidence

GitHub Actions workflow definitions include UART, I²C, SPI, and TWAI native tests, plus I²C coverage. An earlier [I²C coverage workflow run](https://github.com/babarcha/esp32-hardware-interface-labs/actions/runs/39789894330) completed successfully and uploaded a coverage artifact. **Green status for every workflow on the latest commit has not yet been independently verified.**

- [Project-wide qualification report](docs/PROJECT_QUALIFICATION_REPORT.md) — test commands, results, limitations, and qualification decision
- [I²C detailed verification report](i2c/lab02_i2c_master/docs/I2C_LAB02_VERIFICATION.md) — physical HIL, fault injection, recovery, and scoped coverage
- [GitHub Actions workflows](.github/workflows/) — native CI definitions

## Engineering skills demonstrated

Embedded C, ESP-IDF, UART/I²C/SPI/CAN interfaces, hardware abstraction, protocol design, lifecycle and state management, negative/boundary testing, mock-based native unit tests, Python/pytest/PySerial HIL, CMake/Ninja, GCC warning discipline, coverage reporting, GitHub Actions, and Git-based version control.

## Open validation work

1. Validate SPI communication against a physically connected BMP280 or other compatible peripheral and document observed transfers.
2. Validate CAN/TWAI frames over a physical bus with an external transceiver and a second node.
3. Repeat all four ESP-IDF firmware builds at the final qualification revision.
4. Confirm current GitHub Actions results for each workflow.
5. Extend fault, timing, and robustness testing as hardware becomes available.

**Qualification decision:** **PASS for host-native software testing across four labs; PARTIAL for physical hardware qualification.** This is an engineering learning and portfolio project, not a production safety certification.
