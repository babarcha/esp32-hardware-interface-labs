# I²C Lab 02 — ESP32 Master, Address Scanning, and Bus Abstraction

**Repository:** `esp32-hardware-interface-labs`

**Lab path:** `i2c/lab02_i2c_master`

**Platform:** ESP32, ESP-IDF v5.5.5, Windows 11

**Current implementation:** I²C master initialization, address scanning, device registration and blocking transaction APIs. The application demonstrates initialization, scanning and cleanup; real-target read/write validation remains pending.

---

## 1. Purpose and learning objectives

This lab introduces **Inter-Integrated Circuit (I²C)** communication on the ESP32. It teaches both the electrical/protocol fundamentals and a practical embedded software design: separating application behavior from the peripheral driver.

After completing it, you should understand:

1. Why I²C uses two shared signal lines, **SDA** and **SCL**.
2. How 7-bit addressing, START/STOP, ACK/NACK and repeated START work.
3. How to configure the modern ESP-IDF I²C master driver.
4. How a bus scanner discovers responding target addresses.
5. The difference between an **I²C bus handle** and a **device handle**.
6. How to expose reusable peripheral operations through a C module.
7. How to implement blocking transmit, receive and combined write/read operations.
8. How to distinguish successful builds, runtime scans and actual peripheral validation.

This is **Lab 02** in the hardware-interface series. Unlike UART Lab 01, it does not currently implement a PC-to-ESP32 command protocol or a Python HIL test suite.

## 2. The hardware and signal path

The baseline project uses the ESP32's I²C controller 0:

| Signal | ESP32 GPIO | Role |
| --- | --- | --- |
| SDA | GPIO21 | Bidirectional serial data |
| SCL | GPIO22 | Serial clock |
| GND | GND | Common electrical reference for external devices |

The development board connects to the PC over USB for flashing and serial monitor output. **COM3** is the port used in the existing development setup; verify the port on your own machine.

```text
                   PC / ESP-IDF monitor
                            |
                         USB/UART
                            |
                        ESP32 board
                            |
                  I²C master controller
                       /          \
                GPIO21 SDA     GPIO22 SCL
                       \          /
                        I²C bus
                            |
                Optional external target
               (sensor, EEPROM, display)
```

**No external target is required to compile the project or demonstrate the scanning control flow.** With no target connected, the expected discovery count is zero, assuming the bus is operating normally. A scan finding zero devices is not proof that the electrical bus is healthy.

### 2.1 Why I²C needs pull-up resistors

I²C lines are normally **open-drain**: devices pull a signal LOW or release it. Pull-up resistors restore the HIGH level. The code enables ESP32 internal pull-ups, but they can be too weak for a reliable physical bus, especially with longer wiring or higher clock rates.

For a real sensor connection, use suitable pull-ups or a breakout board with correctly configured pull-ups, and ensure the signal voltage is compatible with the ESP32's **3.3 V GPIO**. Never connect a 5 V pull-up directly to an unprotected ESP32 GPIO.

## 3. I²C protocol fundamentals

### 3.1 Master and target

The **master** initiates transactions and generates SCL. A **target** (often called a slave in older documentation) responds when addressed. Several targets can share SDA and SCL if their addresses do not conflict.

```text
               ESP32 I²C master
                    SDA/SCL
                       |
          +------------+------------+
          |            |            |
      Target A     Target B     Target C
       0x3C         0x68         0x76
```

These addresses are **illustrative**, not discovered devices in this project.

### 3.2 START, STOP and ACK/NACK

A transaction generally starts when the master generates a **START** condition, sends a target address plus a read/write bit, exchanges data bytes, and ends with **STOP**. The receiver indicates acknowledgement (**ACK**) or non-acknowledgement (**NACK**) during the acknowledgement clock cycle.

A probe asks, in effect: *Does a target acknowledge this address?* An ACK indicates an address-level response, not necessarily that the target is the sensor you expected or that its measurements are valid.

### 3.3 Seven-bit addressing

This lab probes the ordinary usable 7-bit address range:

```text
0x08 through 0x77, inclusive
```

The total is `0x77 - 0x08 + 1 = 112` addresses. The ranges `0x00–0x07` and `0x78–0x7F` are excluded because they include reserved or special-purpose addresses.

### 3.4 Repeated START

Many sensors use internal registers. To read a register, the master often writes the register number, issues a **repeated START**, and reads data without an intervening STOP:

```text
START → target + WRITE → register number
      → REPEATED START → target + READ → data → STOP
```

The module implements this pattern using `i2c_master_transmit_receive()`.

## 4. Project structure and architecture

```text
i2c/lab02_i2c_master/
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt
│   ├── main.c
│   ├── i2c_bus.c
│   └── i2c_bus.h
├── README.md
└── TUTORIAL.md
```

The relevant responsibilities are:

| File | Responsibility |
| --- | --- |
| Root `CMakeLists.txt` | Declares the ESP-IDF project |
| `main/CMakeLists.txt` | Registers application and bus module source files |
| `main.c` | Runs the demonstration and reports errors |
| `i2c_bus.h` | Declares the reusable I²C API |
| `i2c_bus.c` | Implements controller configuration, scanning and transfers |
| `README.md` | Quick lab overview and usage |
| `TUTORIAL.md` | Detailed learning guide |

The software layers are:

```text
APPLICATION                  main.c
   |                        decides what to do
   | i2c_bus_init()
   | i2c_bus_scan()
   | i2c_bus_deinit()
   v
BUS ABSTRACTION             i2c_bus.h / i2c_bus.c
   |                        knows how to use ESP-IDF
   | i2c_new_master_bus()
   | i2c_master_probe()
   | i2c_del_master_bus()
   v
ESP-IDF DRIVER              driver/i2c_master.h
   |
   v
ESP32 HARDWARE              I²C0, GPIO21, GPIO22
```

The **application** owns the sequence of operations; the **bus abstraction** owns the private bus handle and calls ESP-IDF. This separation makes it easier to add sensors without duplicating bus setup in each application.

## 5. `main/CMakeLists.txt`: building both source files

The component registers both C files:

```cmake
idf_component_register(
    SRCS
        "main.c"
        "i2c_bus.c"
    INCLUDE_DIRS "."
    REQUIRES driver
)
```

`SRCS` tells the build system which `.c` files to compile. `INCLUDE_DIRS "."` makes the local headers available. `REQUIRES driver` declares the driver component dependency.

**Beginner lesson:** Adding a new `.c` file to a project directory is not sufficient by itself; it must also be part of the build configuration.

## 6. `main.c`: the complete application sequence

The current `app_main()` is intentionally short. Its control flow is:

```text
app_main()
   |
   v
Print lab heading
   |
   v
i2c_bus_init()
   |
   +-- failure --> print error and return
   |
   v
i2c_bus_scan()
   |
   v
Print number of responding targets
   |
   v
i2c_bus_deinit()
   |
   +-- failure --> print cleanup error
   |
   v
Return
```

Initialization:

```c
esp_err_t result = i2c_bus_init();
if (result != ESP_OK) {
    printf("I2C initialization failed: %s\n", esp_err_to_name(result));
    return;
}
```

The `esp_err_t` type is ESP-IDF's error-code type. `ESP_OK` means success; `esp_err_to_name()` converts an error code to a readable name. Returning immediately after failed initialization prevents the application from scanning an unavailable bus.

Scanning:

```c
int devices_found = i2c_bus_scan();
printf("\nApplication: %d I2C device(s) detected\n", devices_found);
```

Cleanup:

```c
result = i2c_bus_deinit();
if (result != ESP_OK) {
    printf("I2C cleanup failed: %s\n", esp_err_to_name(result));
}
```

**Important:** The current implementation **does deinitialize the bus** after scanning. This is part of the normal demonstration flow, not a proposed future change.

## 7. `i2c_bus.h`: the public API

The header declares the functions other source files can call:

| API | Meaning | Return type |
| --- | --- | --- |
| `i2c_bus_init()` | Create/configure master bus | `esp_err_t` |
| `i2c_bus_deinit()` | Release master bus | `esp_err_t` |
| `i2c_bus_probe(address)` | Check for address acknowledgement | `esp_err_t` |
| `i2c_bus_scan()` | Probe 112 addresses and count ACKs | `int` |
| `i2c_bus_add_device(address, speed, out)` | Register a target and obtain its handle | `esp_err_t` |
| `i2c_bus_remove_device(handle)` | Unregister a target | `esp_err_t` |
| `i2c_bus_write(handle, data, len)` | Blocking transmit | `esp_err_t` |
| `i2c_bus_read(handle, data, len)` | Blocking receive | `esp_err_t` |
| `i2c_bus_write_read(handle, tx, tx_len, rx, rx_len)` | Combined transmit/receive | `esp_err_t` |

The header includes `driver/i2c_master.h` because the public API exposes the ESP-IDF type `i2c_master_dev_handle_t`. Therefore this is a **thin hardware abstraction**, not a driver-independent interface. That is an appropriate trade-off for an introductory lab.

### 7.1 What is a function declaration?

For example:

```c
esp_err_t i2c_bus_probe(uint8_t address);
```

This tells the compiler that a function exists, accepts an 8-bit address, and returns an ESP-IDF status code. The implementation is in `i2c_bus.c`.

## 8. `i2c_bus.c`: private state and configuration

The module defines its wiring and timeouts:

```c
#define I2C_SDA_GPIO 21
#define I2C_SCL_GPIO 22
#define I2C_GLITCH_FILTER 7
#define I2C_PROBE_TIMEOUT_MS 50
#define I2C_TRANSFER_TIMEOUT_MS 100
```

It also owns one private bus handle:

```c
static i2c_master_bus_handle_t bus_handle = NULL;
```

`static` at file scope limits the variable's visibility to `i2c_bus.c`. `NULL` means that this module does not currently hold an initialized bus handle.

### 8.1 Creating the bus

`i2c_bus_init()` first checks whether a bus is already active:

```c
if (bus_handle != NULL) {
    return ESP_ERR_INVALID_STATE;
}
```

It then creates a configuration structure:

```c
i2c_master_bus_config_t bus_config = {
    .i2c_port = I2C_NUM_0,
    .sda_io_num = I2C_SDA_GPIO,
    .scl_io_num = I2C_SCL_GPIO,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = I2C_GLITCH_FILTER,
    .flags.enable_internal_pullup = true,
};
```

A C **designated initializer** assigns values to named structure members. The modern ESP-IDF API then allocates the master bus:

```c
esp_err_t result = i2c_new_master_bus(&bus_config, &bus_handle);
```

The first argument supplies the configuration; the second receives the newly created handle. On success, the module prints the initialization message and configured GPIOs.

### 8.2 Deinitializing the bus

`i2c_bus_deinit()` rejects calls when `bus_handle == NULL`, then calls:

```c
esp_err_t result = i2c_del_master_bus(bus_handle);
if (result == ESP_OK) {
    bus_handle = NULL;
}
```

The module clears the handle only when the driver reports successful deletion. Registered device handles should be removed before deleting the master bus.

## 9. `i2c_bus_probe()`: checking one address

The function first verifies that the bus exists and the address lies within the ordinary scanning range:

```c
if (bus_handle == NULL) {
    return ESP_ERR_INVALID_STATE;
}
if (address < 0x08 || address > 0x77) {
    return ESP_ERR_INVALID_ARG;
}
```

It then calls the ESP-IDF driver:

```c
return i2c_master_probe(bus_handle, address, I2C_PROBE_TIMEOUT_MS);
```

Possible outcomes include:

- `ESP_OK`: a target acknowledged the address.
- `ESP_ERR_NOT_FOUND`: no target acknowledged the address.
- Other error codes: the driver encountered another condition, such as a timeout or invalid state.

A probe timeout of **50 ms** is configured. The actual elapsed scan time depends on the bus and driver behavior.

## 10. `i2c_bus_scan()`: the address-scanning algorithm

The scanner initializes two counters:

```c
int devices_found = 0;
int addresses_checked = 0;
```

It iterates over the 112 candidate addresses:

```c
for (uint8_t address = 0x08; address <= 0x77; address++) {
    addresses_checked++;
    esp_err_t result = i2c_bus_probe(address);
    // Interpret the result.
}
```

For each probe:

| Result | Scanner behavior |
| --- | --- |
| `ESP_OK` | Print `ACK`, increment `devices_found` |
| `ESP_ERR_NOT_FOUND` | Continue without printing an error |
| Any other result | Print the address and error name; continue |

A representative acknowledgement line is:

```text
ACK  : device at 0x76
```

That is an **example**, not a claim that a device was found in the current setup.

The scanner prints both the total number of addresses checked and the number of responding targets. One limitation is that `i2c_bus_scan()` returns only the count: a clean scan with zero devices and a scan with some driver errors can both return `0`. The detailed console errors help distinguish them. A production-oriented interface could return an error status separately from the count.

## 11. Bus handles versus device handles

The ESP-IDF master API uses two levels of handles:

```text
bus_handle — shared master controller and SDA/SCL bus
    |
    +-- device_handle_A — target address, configured clock
    +-- device_handle_B — target address, configured clock
```

The **bus handle** is private to `i2c_bus.c`. A **device handle** is created when an application registers a particular target and is then passed into transfer functions.

`i2c_bus_add_device()` checks:

- The bus has been initialized.
- The address is between `0x08` and `0x77`.
- The requested clock speed is nonzero.
- The output handle pointer is not `NULL`.

It configures 7-bit addressing:

```c
i2c_device_config_t device_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = address,
    .scl_speed_hz = clock_speed_hz,
};
```

Then it calls `i2c_master_bus_add_device()`. This **registers software configuration**; it does not by itself prove that the target is physically connected or responding.

`i2c_bus_remove_device()` calls `i2c_master_bus_rm_device()` to release a previously registered device handle.

**Current application scope:** `main.c` does not call either registration function. They are available for a future sensor integration pass.

## 12. Blocking data transfers

The bus abstraction implements three transaction functions. Each rejects a `NULL` device handle, a `NULL` buffer, or a zero-length transfer as appropriate.

### 12.1 Writing bytes

```c
return i2c_master_transmit(
    device_handle,
    data,
    data_length,
    I2C_TRANSFER_TIMEOUT_MS);
```

The master sends bytes to the selected target. The driver manages the bus-level signaling.

### 12.2 Reading bytes

```c
return i2c_master_receive(
    device_handle,
    data,
    data_length,
    I2C_TRANSFER_TIMEOUT_MS);
```

The driver fills the caller's buffer with bytes received from the target.

### 12.3 Combined write/read

```c
return i2c_master_transmit_receive(
    device_handle,
    write_data,
    write_length,
    read_data,
    read_length,
    I2C_TRANSFER_TIMEOUT_MS);
```

This is useful for register reads, such as sending a one-byte register address and receiving a one-byte identification value. It uses a repeated START between the write and read phases.

All three calls use a **100 ms timeout**. They are blocking: the calling task waits for the operation to complete or fail.

**Validation boundary:** These functions are implemented in the repository but have not been confirmed against a connected physical target in the current lab milestone.

## 13. Build, flash and monitor on Windows

Open an ESP-IDF-enabled terminal (for example, through the VS Code ESP-IDF extension). From the I²C project directory:

```powershell
cd C:\Users\babar\esp32-hardware-interface-labs\i2c\lab02_i2c_master
idf.py --version
idf.py build
```

The previously reported `idf.py build` completed successfully with ESP-IDF v5.5.5. This confirms the project compiled and linked in that environment; it does **not** prove physical bus transactions.

With the ESP32 connected and the correct serial port identified:

```powershell
idf.py -p COM3 flash
idf.py -p COM3 monitor
```

Exit the monitor using **Ctrl+]**. You can also use:

```powershell
idf.py -p COM3 flash monitor
```

Only one program should own the serial port at a time. If COM3 is unavailable, inspect Windows Device Manager or the ports reported by your development tools.

## 14. Expected console output and interpretation

With no external I²C target connected, an illustrative baseline is:

```text
I2C Lab 02 - Pass 5: Bus Abstraction
------------------------------------
I2C controller initialized successfully
SDA = GPIO21
SCL = GPIO22

Scanning I2C bus...

Scan complete
Addresses checked : 112
Devices found     : 0

Application: 0 I2C device(s) detected
```

The actual output may also include ESP-IDF boot logs or driver warnings. Do not ignore any `ERROR:` lines: zero discovered devices can reflect an empty bus **or** an electrical/driver problem.

### What has been demonstrated?

| Activity | Status |
| --- | --- |
| ESP-IDF compilation and linking | Confirmed in the development workflow |
| Initialization/scanning/cleanup code | Implemented |
| Address scanning without an external target | Described in previous lab documentation; recheck on the current flashed revision as needed |
| Successful read/write to a physical sensor | Not yet validated |
| Correct sensor identification and measurements | Not yet validated |

Avoid calling the lab “sensor validated” until actual target transactions and expected register values have been checked.

## 15. Future hardware exercise: BMP280 or BME280

A suitable next step is a BMP280/BME280-family breakout board that supports 3.3 V I²C signaling. **Check the exact breakout board's pin labels and electrical specifications before wiring.** Typical connections are:

| ESP32 | Sensor breakout |
| --- | --- |
| 3V3 | Supply input compatible with 3.3 V |
| GND | GND |
| GPIO21 | SDA |
| GPIO22 | SCL |

Some breakouts also expose address-select or SPI-mode pins; their required states depend on the module. Do not assume that every breakout has identical pin labels or built-in pull-ups.

A sensible validation sequence is:

1. Power off and wire the target.
2. Verify supply voltage, common ground, SDA/SCL and pull-ups.
3. Flash the current scanner and confirm an ACK at the target's configured address.
4. Register a device handle with an appropriate bus speed.
5. Read the sensor identification register; for BMP280/BME280-family devices, `0xD0` is commonly used.
6. Compare the returned ID with the specific device datasheet.
7. Only then add measurement configuration and compensated sensor readings.
8. Remove the device handle and deinitialize the bus during cleanup.

This sequence is a **future exercise**, not functionality already demonstrated by the present `app_main()`.

## 16. Common errors and troubleshooting

| Symptom | Likely checks |
| --- | --- |
| `idf.py` not recognized | Use the ESP-IDF-enabled terminal |
| Link/build failure | Confirm `main/CMakeLists.txt` includes `i2c_bus.c` and the driver dependency |
| Flash failure | Verify COM port, USB cable, board power and other serial-port users |
| Monitor shows old UART lab | Confirm current working directory, flash target and monitor session |
| Zero devices, no target connected | Expected baseline; inspect any driver errors |
| Zero devices, sensor connected | Check sensor power, SDA/SCL, GND, address-select pins and pull-ups |
| Unstable ACKs | Check voltage, wiring length, pull-up values and clock speed |
| `ESP_ERR_INVALID_STATE` | Confirm initialization/lifecycle sequence |
| `ESP_ERR_INVALID_ARG` | Check address range, nonzero speed, handles and buffers |
| Bus deletion fails | Ensure registered target handles are removed first |

## 17. Design decisions and trade-offs

**Why use a dedicated `i2c_bus` module?** It keeps the application focused on the demonstration and provides one place to configure GPIOs, timeouts, error checks and resource ownership.

**Why use the modern master API?** The lab targets ESP-IDF v5.5.5 and its `driver/i2c_master.h` interface, with explicit bus and device handles.

**Why scan 112 addresses?** The loop deliberately excludes reserved and special-purpose 7-bit address ranges.

**Why not identify sensors from ACK alone?** Different devices may use the same address. Identification requires a device-specific register transaction and an expected response.

**Why blocking transfers?** They are straightforward to understand and adequate for a beginner lab. A more complex application may need asynchronous scheduling, concurrency controls or different timeout policies.

**Why not build a larger framework now?** This lab's objective is to learn I²C and a minimal reusable module. Device-specific drivers, automated HIL testing and more advanced abstractions can be separate later exercises.

## 18. Interview questions and short answers

**Q: What distinguishes UART from I²C?**

UART is asynchronous serial communication with no shared clock; I²C is synchronous, address-based and can connect multiple targets on shared SDA/SCL lines.

**Q: What does a NACK mean?**

The addressed target did not acknowledge that transfer phase. In an address scan it often means no target is present at that address, but electrical faults can also prevent communication.

**Q: What is the difference between a bus handle and a device handle?**

A bus handle represents the shared I²C master controller; a device handle represents a registered target's address and communication settings.

**Q: Does `i2c_bus_add_device()` prove the sensor is connected?**

No. It registers software configuration. A probe or actual transfer is needed to observe a response.

**Q: Why is `i2c_master_transmit_receive()` useful?**

It supports a register-selection write followed by a read within one combined transaction, using a repeated START.

**Q: What is the scanner's principal API limitation?**

It returns only a device count, so the caller cannot directly distinguish a clean zero-device scan from a scan that encountered driver errors.

**Q: What has been verified in this lab?**

The firmware build was successful. The code implements initialization, scanning and cleanup, but device-specific transfers still require a connected target and runtime verification.

## 19. Practical exercises

1. Locate each public function declaration in `i2c_bus.h` and its implementation in `i2c_bus.c`.
2. Trace the execution from `app_main()` to `i2c_master_probe()`.
3. Explain why `bus_handle` is declared `static` and initialized to `NULL`.
4. Calculate why `0x08` through `0x77` contains 112 addresses.
5. Explain the difference between `ESP_ERR_NOT_FOUND` and `ESP_ERR_INVALID_ARG`.
6. Connect a compatible target in a later hardware session and verify its address.
7. Extend the application to read a target's ID register, checking the return code and expected ID.
8. As a later API design exercise, consider returning `esp_err_t` from a scanner while providing the count through an output pointer.

## 20. Summary

The I²C Lab 02 firmware follows a small, understandable design: `main.c` runs a scanner, `i2c_bus.h` exposes a reusable API, and `i2c_bus.c` owns the ESP-IDF master controller. The scanner checks 112 ordinary 7-bit addresses and counts acknowledgements. Device registration and blocking read/write operations are already implemented for future hardware work, but a successful build or empty-bus scan should not be mistaken for validated sensor communication.

**Next milestone:** connect an I²C target, confirm its address and ID register, and validate actual data transfers.
