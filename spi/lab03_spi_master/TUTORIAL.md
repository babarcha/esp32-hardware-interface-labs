# SPI Lab 03 — ESP32 SPI Master, Bus Abstraction, and Full-Duplex Transfers

**Repository:** `esp32-hardware-interface-labs`

**Lab directory:** `spi/lab03_spi_master`

**Environment:** ESP32, ESP-IDF v5.5.5, Windows 11, PowerShell

**Milestone:** Reusable SPI3 master bus and device interface; generic driver transaction without an external SPI peripheral

## 1. Purpose of this lab

This lab teaches how to configure an ESP32 as an SPI master, register a logical SPI device, transfer bytes, interpret driver errors, and release resources. The application is intentionally simple: it transmits `0x9A 0xBC 0xDE` and prints the bytes sampled on MISO.

The lab does **not** implement an MCP3008 or other peripheral protocol yet. A successful SPI driver call means the requested transaction completed at the driver level; it does not establish that a physical device received or interpreted the data.

## 2. Learning objectives

By the end, you should be able to:

1. Explain MOSI, MISO, SCLK, and chip select.
2. Distinguish SPI from UART and I²C.
3. Describe clock polarity (CPOL), clock phase (CPHA), and SPI modes 0–3.
4. Explain SPI's simultaneous transmit and receive behavior.
5. Differentiate a bus handle from a device handle.
6. Follow the `app_main()` → `spi_bus` → ESP-IDF driver call chain.
7. Understand why ESP-IDF transaction lengths are specified in bits.
8. Build, flash, monitor, and troubleshoot the independent ESP-IDF project.
9. Explain the difference between a successful build, driver execution, and physical hardware validation.

## 3. What is SPI?

**Serial Peripheral Interface (SPI)** is a synchronous serial communication method. A controller (master) generates a clock and exchanges bits with a selected peripheral. Unlike I²C, conventional SPI does not use target addresses or an ACK/NACK bit; the controller typically selects a peripheral using its chip-select signal.

The four common signals are:

| Signal | Direction relative to master | Role |
| --- | --- | --- |
| MOSI | Output | Master sends bits to peripheral |
| MISO | Input | Master samples bits from peripheral |
| SCLK | Output | Master supplies the clock |
| CS / SS | Output | Selects the target, usually active LOW |

SPI can be **full duplex**: while one bit is sent on MOSI, another can be received on MISO during the same clock period. Whether incoming bits have useful meaning depends on the selected device and its protocol.

### SPI versus UART versus I²C

| Property | UART | I²C | SPI |
| --- | --- | --- | --- |
| Clock | Asynchronous; no shared clock wire | Shared SCL | Shared SCLK |
| Typical signal lines | TX, RX | SDA, SCL | MOSI, MISO, SCLK, CS |
| Target selection | Application protocol | 7-bit/10-bit address | Chip select |
| ACK/NACK | Not inherent to UART framing | Built into bus transactions | Not inherent to standard SPI |
| Full duplex | TX and RX can operate simultaneously | Typically half-duplex at the transaction level | Commonly full duplex |

## 4. Hardware configuration in this project

The current source uses the following pin assignments:

| SPI signal | ESP32 pin | Source definition |
| --- | --- | --- |
| MOSI | GPIO23 | `SPI_MOSI_GPIO` |
| MISO | GPIO19 | `SPI_MISO_GPIO` |
| SCLK | GPIO18 | `SPI_SCLK_GPIO` |
| CS | GPIO5 | `SPI_CS_GPIO` in `main.c` |

The bus is `SPI3_HOST`; the demonstration device uses **1 MHz** and **Mode 0**.

```text
                  ESP32 SPI3_HOST

 GPIO23 MOSI ------------------------> Peripheral DIN
 GPIO19 MISO <------------------------ Peripheral DOUT
 GPIO18 SCLK ------------------------> Peripheral CLK
 GPIO5  CS   ------------------------> Peripheral CS
 GND        ------------------------- Peripheral GND
```

This drawing is a **future wiring example**, not evidence that a peripheral is connected. When connecting real hardware, also provide compatible power and verify the peripheral's voltage levels. ESP32 GPIO must not be exposed directly to incompatible higher-voltage signals.

### Why use separate chip-select pins?

Several SPI peripherals can share MOSI, MISO, and SCLK if their electrical and protocol requirements permit it. Each typically has a separate CS signal so only the intended peripheral responds during a transaction.

## 5. SPI modes: CPOL and CPHA

SPI modes describe the idle level of the clock and which clock edge is used for sampling data.

- **CPOL** determines the clock's idle level: LOW for 0, HIGH for 1.
- **CPHA** determines the sampling/shift phase relative to the clock edges.

| SPI mode | CPOL | CPHA | Idle clock |
| --- | ---: | ---: | --- |
| 0 | 0 | 0 | LOW |
| 1 | 0 | 1 | LOW |
| 2 | 1 | 0 | HIGH |
| 3 | 1 | 1 | HIGH |

The application registers its demonstration device with `SPI_MODE` set to `0`. That is a software setting; the correct mode for a real peripheral must come from its datasheet.

## 6. Project directory structure

```text
esp32-hardware-interface-labs/
└── spi/
    └── lab03_spi_master/
        ├── CMakeLists.txt
        ├── README.md
        ├── TUTORIAL.md
        └── main/
            ├── CMakeLists.txt
            ├── main.c
            ├── spi_bus.c
            └── spi_bus.h
```

The SPI lab is an **independent ESP-IDF project**. Run `idf.py` from `spi/lab03_spi_master`, not from the repository root.

### File responsibilities

| File | Responsibility |
| --- | --- |
| `main.c` | Application sequence, demonstration payload, console output, cleanup |
| `spi_bus.h` | Public SPI abstraction declarations |
| `spi_bus.c` | ESP-IDF bus setup, device registration, transaction construction, validation |
| `main/CMakeLists.txt` | Registers `main.c` and `spi_bus.c` with ESP-IDF |
| `README.md` | Short project overview and current limitations |
| `TUTORIAL.md` | Detailed concepts and code walkthrough |

## 7. Firmware architecture

```text
                   APPLICATION
                     main.c
                       |
                       | calls public SPI functions
                       v
                 PUBLIC INTERFACE
                   spi_bus.h
                       |
                       v
                BUS IMPLEMENTATION
                   spi_bus.c
                       |
                       | spi_bus_initialize()
                       | spi_bus_add_device()
                       | spi_device_transmit()
                       | spi_bus_remove_device()
                       | spi_bus_free()
                       v
                ESP-IDF SPI DRIVER
                       |
                       v
              ESP32 SPI3_HOST HARDWARE
                       |
              MOSI / MISO / SCLK / CS
```

**Design choice:** `main.c` expresses *what* the demonstration should do. `spi_bus.c` encapsulates *how* ESP-IDF configures and operates the SPI controller. The application does not construct `spi_transaction_t` itself.

This is a useful embedded-software boundary: application behavior can change without duplicating low-level driver setup across files.

## 8. `main.c` — the application walkthrough

The program begins in ESP-IDF's `app_main()` function. Its sequence is:

```text
app_main()
   |
   +-- spi_bus_init()
   |
   +-- spi_device_register(CS=5, 1 MHz, Mode 0)
   |
   +-- prepare TX and RX buffers
   |
   +-- spi_bus_transfer(...)
   |
   +-- print status, TX bytes, and RX bytes
   |
   +-- spi_device_unregister(...)
   |
   +-- spi_bus_deinit()
```

### 8.1 Application configuration

```c
#define SPI_CS_GPIO 5
#define SPI_CLOCK_HZ 1000000
#define SPI_MODE 0
```

These are **device-specific settings**, not the shared bus pin configuration. MOSI/MISO/SCLK are configured in `spi_bus.c`; CS, speed, and mode are supplied when registering the logical device.

### 8.2 Initialize the bus

```c
esp_err_t result = spi_bus_init();

if (result != ESP_OK)
{
    printf("SPI initialization failed: %s\n",
           esp_err_to_name(result));
    return;
}
```

`esp_err_t` is ESP-IDF's error-code type. `ESP_OK` indicates success. The application returns early if initialization fails because later operations depend on the bus.

### 8.3 Register a logical device

```c
spi_device_handle_t device_handle = NULL;

result = spi_device_register(
    SPI_CS_GPIO,
    SPI_CLOCK_HZ,
    SPI_MODE,
    &device_handle);
```

The output argument is `&device_handle`, the address of the handle variable. The wrapper requires this variable to be initialized to `NULL`. ESP-IDF populates it on successful registration.

**Important:** registering a device creates a driver-side configuration. It does not probe for physical presence; standard SPI has no general-purpose discovery ACK.

If registration fails, the application attempts to deinitialize the bus and returns. This is the early-failure cleanup path.

### 8.4 Prepare the transmit and receive buffers

```c
uint8_t tx_data[] = {0x9A, 0xBC, 0xDE};
uint8_t rx_data[sizeof(tx_data)] = {0};
```

`uint8_t` represents an 8-bit unsigned value. `sizeof(tx_data)` evaluates to three bytes here. Initializing `rx_data` to zero makes its initial state predictable, but its contents after a completed transfer depend on the MISO signal.

The bytes `0x9A 0xBC 0xDE` are an arbitrary demonstration payload. They are **not an MCP3008 command**.

### 8.5 Execute a full-duplex transfer

```c
result = spi_bus_transfer(
    device_handle,
    tx_data,
    rx_data,
    sizeof(tx_data));
```

This sends and receives three bytes during one clocked transaction. The driver controls CS and SCLK according to the registered device configuration.

The application prints the transmitted and received buffers only when the driver returns `ESP_OK`. It also prints a warning that RX bytes are not meaningful without a connected peripheral.

### 8.6 Release resources

```c
result = spi_device_unregister(device_handle);
```

The application unregisters the logical device before calling `spi_bus_deinit()` to free the shared bus.

The current code returns immediately if device removal fails, leaving the bus allocated rather than attempting to free it while the device may still be registered. The bus-deinitialization result is reported separately.

## 9. `spi_bus.h` — the public interface

The header defines the functions available to application code:

| Function | Purpose |
| --- | --- |
| `spi_bus_init()` | Initialize shared SPI3 host |
| `spi_bus_deinit()` | Free shared SPI3 host |
| `spi_device_register(...)` | Register device configuration and return a handle |
| `spi_device_unregister(...)` | Remove a registered device |
| `spi_bus_write(...)` | Transmit bytes |
| `spi_bus_read(...)` | Receive bytes while generating clock |
| `spi_bus_transfer(...)` | Transmit and receive simultaneously |

The wrapper accepts transfer lengths in **bytes** and returns `esp_err_t`. Device registration uses ESP-IDF's `spi_device_handle_t` type, so the wrapper is a focused abstraction rather than a completely hardware-independent interface.

### Why use a header file?

`main.c` needs function declarations and types but should not need to know how each SPI transaction is built. Including `spi_bus.h` provides that contract. The implementation remains in `spi_bus.c`.

## 10. `spi_bus.c` — shared bus initialization

The module defines:

```c
#define SPI_MOSI_GPIO 23
#define SPI_MISO_GPIO 19
#define SPI_SCLK_GPIO 18
#define SPI_HOST_USED SPI3_HOST

static bool bus_initialized = false;
```

`static` at file scope limits visibility of `bus_initialized` to `spi_bus.c`. The flag tracks whether **this module** has successfully initialized its SPI bus; it is not a mutex or a general multi-threaded synchronization mechanism.

### Bus configuration

```c
spi_bus_config_t bus_config = {
    .mosi_io_num = SPI_MOSI_GPIO,
    .miso_io_num = SPI_MISO_GPIO,
    .sclk_io_num = SPI_SCLK_GPIO,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 0,
};
```

The unused quad-data pins are set to `-1`. This lab uses ordinary single-data-line MOSI and MISO signaling rather than quad SPI.

Initialization then calls:

```c
esp_err_t result = spi_bus_initialize(
    SPI_HOST_USED,
    &bus_config,
    SPI_DMA_CH_AUTO);
```

`SPI_DMA_CH_AUTO` requests automatic DMA channel selection by ESP-IDF. The code sets `bus_initialized = true` only when the driver reports success.

### Duplicate initialization

Calling `spi_bus_init()` while `bus_initialized` is true returns `ESP_ERR_INVALID_STATE`. This prevents accidental duplicate setup through the wrapper.

## 11. Bus handle versus device handle

A **bus** represents shared SPI controller resources, including the host and shared signal pins. A **device handle** represents a peripheral's registration on that bus, including CS, speed, and mode.

```text
                    SPI3_HOST
                       |
                Shared SPI bus
              MOSI / MISO / SCLK
                       |
          +------------+------------+
          |                         |
   Device handle A             Device handle B
   CS GPIO5                    Separate CS pin
   Mode 0, 1 MHz               Own mode/speed
```

The second device is illustrative. The current application registers only one logical device. Device registration does not prove a physical device is attached.

## 12. Device registration and validation

`spi_device_register()` checks:

1. The bus must already be initialized.
2. The output pointer must not be `NULL`.
3. The caller's existing handle must be `NULL`.
4. CS GPIO must be nonnegative.
5. Clock speed must be positive.
6. SPI mode must be in the range 0–3.

It then constructs:

```c
spi_device_interface_config_t device_config = {
    .clock_speed_hz = clock_speed_hz,
    .mode = mode,
    .spics_io_num = cs_gpio,
    .queue_size = 1,
};
```

Finally, it calls `spi_bus_add_device()` and returns the driver's status. These are **basic wrapper checks**; ESP-IDF may impose additional pin, frequency, and resource constraints.

`queue_size = 1` configures transaction queue capacity. The current wrapper uses synchronous `spi_device_transmit()`, not the asynchronous queue APIs.

## 13. SPI transactions: the critical data structure

ESP-IDF uses `spi_transaction_t` to describe transfers. In this lab, each transfer function creates a local transaction and zero-initializes it:

```c
spi_transaction_t transaction;
memset(&transaction, 0, sizeof(transaction));
```

This matters because the structure has optional fields. Clearing it prevents uninitialized fields from accidentally affecting the driver call.

### Bits versus bytes

The wrapper accepts a byte count, but ESP-IDF's `transaction.length` uses **bits**:

```c
transaction.length = data_length * 8;
```

For the demonstration:

```text
3 bytes × 8 bits/byte = 24 bits
```

This conversion is easy to overlook in embedded-driver development and is a useful interview example.

## 14. Write-only transfer — `spi_bus_write()`

The wrapper validates the initialized state, device handle, transmit pointer, and nonzero byte count. It then assigns:

```c
transaction.length = data_length * 8;
transaction.tx_buffer = tx_data;
```

and calls:

```c
spi_device_transmit(device_handle, &transaction);
```

Clock pulses are generated and bits are sent on MOSI. Although electrical SPI signaling can still shift bits on MISO, this function does not provide an RX buffer to the caller.

## 15. Read-only transfer — `spi_bus_read()`

The wrapper sets:

```c
transaction.length = data_length * 8;
transaction.rxlength = data_length * 8;
transaction.rx_buffer = rx_data;
```

Even when the application only wants to **read**, the SPI master must generate clock pulses. Unlike I²C, a generic SPI read does not inherently specify a device register or command. Real peripherals often require an initial command/address phase, which may be implemented using full-duplex or multi-phase transactions.

This function is implemented in the abstraction but is **not called by the current `main.c` demonstration**.

## 16. Full-duplex transfer — `spi_bus_transfer()`

The wrapper sets:

```c
transaction.length = data_length * 8;
transaction.tx_buffer = tx_data;
transaction.rx_buffer = rx_data;
```

It then executes a synchronous transaction with `spi_device_transmit()`.

For the current three-byte payload, the controller clocks out 24 bits on MOSI and samples 24 bits on MISO during the same transaction. The received bytes may be affected by a floating MISO pin if no peripheral drives it.

**Do not assume that RX equals TX.** Equal values would require an actual loopback connection or a peripheral that deliberately echoes the data. The sample output in the repository README is illustrative; it is not a guaranteed result for an unconnected MISO line.

## 17. Synchronous and asynchronous SPI

The current wrapper uses:

```c
spi_device_transmit(device_handle, &transaction);
```

This is a **blocking** call: the calling task waits for completion or an error. It is appropriate for a small educational example because the control flow is straightforward.

ESP-IDF also provides queued/asynchronous transaction APIs. Those can be useful for overlapping work with transfers or managing multiple pending operations, but they introduce additional state and buffer-lifetime requirements. They are intentionally outside the scope of this lab.

## 18. Error handling and resource lifecycle

The public functions return ESP-IDF error codes. The wrapper distinguishes several invalid-use cases:

| Situation | Typical wrapper result |
| --- | --- |
| Initialize twice | `ESP_ERR_INVALID_STATE` |
| Register before bus initialization | `ESP_ERR_INVALID_STATE` |
| Register with non-NULL existing output handle | `ESP_ERR_INVALID_STATE` |
| Register with invalid mode or zero speed | `ESP_ERR_INVALID_ARG` |
| Transfer before bus initialization | `ESP_ERR_INVALID_STATE` |
| Transfer with NULL data pointer or zero length | `ESP_ERR_INVALID_ARG` |
| Free uninitialized bus | `ESP_ERR_INVALID_STATE` |

Other error codes come from ESP-IDF itself.

### Correct ownership order

```text
spi_bus_init()
     |
spi_device_register()
     |
spi_bus_transfer()
     |
spi_device_unregister()
     |
spi_bus_deinit()
```

A registered device must be removed before freeing its bus. `spi_bus_deinit()` clears `bus_initialized` only if `spi_bus_free()` succeeds.

### Cleanup on failures

- If bus initialization fails, `main.c` returns without registering a device.
- If device registration fails, `main.c` attempts `spi_bus_deinit()` and returns. The current code does not separately print an error if this early cleanup attempt fails.
- If the transfer fails, the application reports the error and still attempts device removal and bus deinitialization.
- If device removal fails, the application reports the failure and returns without freeing the bus.
- If final bus deinitialization fails, the application reports that error.

This describes the **current source behavior**, not an idealized future design.

## 19. Build, flash, and monitor

Open an ESP-IDF-enabled terminal in VS Code or activate your ESP-IDF v5.5.5 environment first.

```powershell
cd C:\Users\babar\esp32-hardware-interface-labs\spi\lab03_spi_master
idf.py --version
idf.py build
```

To flash the connected board:

```powershell
idf.py -p COM3 flash
```

To view the serial output:

```powershell
idf.py -p COM3 monitor
```

Exit the monitor using **Ctrl+]**. If the board uses another COM port, replace `COM3` accordingly.

### What each command proves

| Step | Evidence |
| --- | --- |
| `idf.py build` succeeds | Source compiled and linked into firmware |
| `idf.py flash` succeeds | Firmware was written to the selected board |
| Monitor shows startup and driver messages | Application executed and reached those operations |
| Logic analyzer captures CS/SCLK/MOSI | Actual electrical signaling occurred |
| Peripheral returns a verified response | Device-level communication has been demonstrated |

A successful build alone does not prove that SPI hardware transactions ran on the ESP32.

## 20. Expected demonstration output

The application prints a heading, reports bus and device setup, attempts a full-duplex transfer, displays TX and RX bytes if successful, and reports cleanup.

Example **illustrative** output:

```text
SPI Lab 03 - Pass 5: Bus Abstraction
------------------------------------
SPI bus initialized successfully
SPI device registered successfully
SPI transaction completed successfully
TX: 0x9A 0xBC 0xDE
RX: 0x?? 0x?? 0x??
NOTE: RX data is not meaningful without a connected SPI peripheral.
SPI device removed successfully
SPI bus released successfully
```

`0x??` is a placeholder, **not literal output from the program**. Actual bytes depend on what the MISO line samples. Do not use a particular RX pattern as a pass criterion until a real peripheral or defined loopback is connected.

## 21. Validation status and limitations

The existing repository documentation describes ESP32-side SPI initialization, registration, transaction execution, and cleanup as previously exercised. The source also has a confirmed successful build from the prior development pass. Those statements should not be confused with physical-device validation.

**Currently not demonstrated with an external SPI peripheral:**

- Device identification or register reads.
- Verified data integrity on MISO.
- Peripheral response to the transmitted bytes.
- Pin-level timing and signal quality measured with a logic analyzer.
- MCP3008 analog-to-digital conversion results.

The generic SPI abstraction is therefore ready for the next hardware-oriented exercise, but device-specific correctness remains to be established.

## 22. Next extension: MCP3008 ADC

The MCP3008 is a 10-bit, eight-channel SPI analog-to-digital converter. It is a practical next step because it requires meaningful command bytes and returns a value that can be checked against a known analog input.

A future MCP3008 pass should:

1. Confirm the specific module's pin labels and voltage requirements against its datasheet or board documentation.
2. Connect shared ground, compatible supply, MOSI, MISO, SCLK, and CS.
3. Configure a suitable SPI clock and mode for the device.
4. Send the device's documented channel-selection command.
5. Decode the returned ADC bits into a 10-bit result.
6. Compare results against known input voltages.
7. Optionally capture CS/SCLK/MOSI/MISO with a logic analyzer.

The current arbitrary payload `0x9A 0xBC 0xDE` is **not** an MCP3008 measurement command. Keep this generic lab separate from the future sensor-specific driver until the hardware is ready.

## 23. Troubleshooting guide

| Symptom | Likely area to inspect |
| --- | --- |
| `idf.py` not found | ESP-IDF environment not activated |
| Build fails | Correct lab directory, `main/CMakeLists.txt`, ESP-IDF version |
| Flash fails | USB cable, COM port, other monitor occupying port |
| Unexpected old output | Wrong project flashed or stale monitor session |
| Bus initialization fails | Host/resource conflict, configuration, driver error |
| Device registration fails | CS pin, clock, mode, output handle state |
| Transaction returns error | Device handle, data buffers, length, driver error |
| RX data looks random | No device driving MISO, wrong wiring, timing or protocol |
| Bus deinitialization fails | Registered devices not removed or driver error |

Always record the exact `esp_err_to_name()` result before deciding whether a failure is caused by software configuration or hardware wiring.

## 24. Design choices and tradeoffs

**Why a wrapper?** It centralizes ESP-IDF-specific configuration, validation, and transaction setup, keeping `main.c` focused on application behavior.

**Why synchronous calls?** They are easier to understand and debug in an introductory lab. Queued transactions can be introduced after the basic lifecycle is understood.

**Why separate bus and device lifecycle?** Several device configurations can share one SPI host; each must be removed before the shared host is freed.

**Why no peripheral driver yet?** The goal is first to understand the generic transport. MCP3008 command formatting, response decoding, and analog validation belong in the next hardware pass.

**Why keep this separate from `embedded-validation-platform`?** This repository teaches low-level peripheral interfaces. A separate validation platform can later add reusable automated test orchestration without complicating these introductory firmware modules.

## 25. Interview questions and concise answers

**Q: What is SPI?**

A synchronous serial interface where a master clocks data and typically selects a peripheral with a chip-select signal.

**Q: Why is SPI full duplex?**

MOSI and MISO are separate lines, allowing transmission and reception during the same clock cycles.

**Q: What determines the SPI mode?**

Clock polarity and phase, selected to match the peripheral's timing requirements.

**Q: Why does `transaction.length` use `data_length * 8`?**

The wrapper receives a byte count, but ESP-IDF requires the transaction length in bits.

**Q: Does successful `spi_device_register()` mean a chip is present?**

No. It creates a logical driver registration and does not probe the hardware.

**Q: Does `spi_device_transmit()` returning `ESP_OK` prove valid sensor data?**

No. It confirms driver-level completion; sensor correctness requires a device-specific protocol and a validated response.

**Q: Why remove a device before freeing the bus?**

The SPI host owns shared resources, and ESP-IDF expects registered devices to be removed before the host is freed.

**Q: What is the main architectural benefit?**

`main.c` describes application behavior while `spi_bus.c/.h` encapsulates hardware-driver operations and resource management.

## 26. Suggested learning exercises

1. Trace every call made by `app_main()` into `spi_bus.c`.
2. Explain the state and argument checks for `spi_device_register()`.
3. Calculate the transaction bit length for 1, 3, 8, and 16 bytes.
4. Compare `spi_bus_write()`, `spi_bus_read()`, and `spi_bus_transfer()` line by line.
5. Explain why the RX buffer cannot be used as a sensor reading without a real peripheral.
6. When hardware is available, connect a compatible peripheral and validate the command/response behavior.
7. Capture SPI signals with a logic analyzer and identify CS assertion, clock edges, MOSI, and MISO.

## 27. Key takeaways

- The application uses a reusable `spi_bus` abstraction around ESP-IDF's SPI master driver.
- SPI3_HOST is configured with MOSI GPIO23, MISO GPIO19, and SCLK GPIO18.
- The demonstration registers one logical device using CS GPIO5, Mode 0, and 1 MHz.
- Three bytes produce a 24-bit full-duplex transaction.
- Driver completion and meaningful peripheral communication are different validation milestones.
- Device handles must be unregistered before the bus is freed.
- MCP3008 integration is the next meaningful hardware validation step, not part of the current generic demonstration.
