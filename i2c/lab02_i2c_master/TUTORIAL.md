# I²C Lab 02 — ESP32 Master, Address Scanning, and Bus Abstraction

**Repository:** `esp32-hardware-interface-labs`  
**Lab path:** `i2c/lab02_i2c_master`  
**Platform:** ESP32, ESP-IDF v5.5.5  
**Current milestone:** I²C bus initialization and address scanning; no external target required.

## 1. Learning objectives

By completing this lab, you should be able to explain the two-wire I²C protocol, initialize an ESP32 I²C master controller, distinguish a *bus handle* from a *device handle*, discover target addresses, and interpret acknowledgements and driver errors. You will also understand why hardware access is wrapped in a reusable module rather than embedded directly in `app_main()`.

## 2. Project architecture

```text
app_main()                     main/main.c
    |
    | i2c_bus_init(), i2c_bus_scan()
    v
i2c_bus.h                     public interface
    |
    v
i2c_bus.c                     reusable implementation
    |
    | i2c_new_master_bus(), i2c_master_probe(), ...
    v
ESP-IDF I²C master driver
    |
    v
ESP32 I²C0: GPIO21 SDA, GPIO22 SCL
    |
    v
External I²C target (optional; not connected yet)
```

The application knows *what* to do: initialize and scan. The bus module knows *how* to use the ESP-IDF driver. This separation supports later sensor drivers and testable application logic.

## 3. How I²C works

I²C is a synchronous, shared-bus serial interface. **SDA** carries data; **SCL** carries the clock. The master initiates transactions and selects a target using its address. Targets normally acknowledge their address by driving SDA low during the ACK bit. The master controls START and STOP conditions. A read may involve a repeated START to change from writing a register address to reading register data.

Unlike UART, I²C normally supports multiple targets on the same SDA/SCL pair. Each target must have an appropriate address, and bus electrical characteristics must support the chosen clock speed.

### Open-drain signaling and pull-ups

I²C participants pull a line LOW or release it; they do not actively drive it HIGH. Pull-up resistors restore HIGH. The code enables ESP32 internal pull-ups, which can be sufficient for limited experiments but may be too weak for reliable physical buses. Proper external pull-ups should be selected based on voltage, capacitance, wiring, and speed. Do not use 5 V pull-ups on unprotected ESP32 GPIO.

## 4. Addressing and ACK/NACK

This lab uses **7-bit target addresses**. During an address probe, an ACK indicates that some target responded at that address. An absent target usually produces a NACK, represented by `ESP_ERR_NOT_FOUND` in the probe API.

The scanner checks addresses **0x08 through 0x77 inclusive**. This is 112 candidate addresses; other 7-bit values are reserved or special-purpose and are not part of this general-purpose scan.

An ACK is evidence of bus-level communication, **not** proof of target identity or correct sensor readings.

## 5. Directory and file responsibilities

```text
i2c/lab02_i2c_master/
├── CMakeLists.txt           ESP-IDF project definition
├── main/
│   ├── CMakeLists.txt       component source registration
│   ├── main.c               application entry point
│   ├── i2c_bus.h            public bus API
│   └── i2c_bus.c            bus API implementation
├── README.md               concise lab overview
└── TUTORIAL.md             detailed learning material
```

The project is independent of the UART, SPI, and CAN/TWAI projects. Run `idf.py` from this lab directory, not the repository root.

## 6. `main.c`: application flow

`app_main()` prints a heading, calls `i2c_bus_init()`, checks the returned `esp_err_t`, then calls `i2c_bus_scan()` and prints the count. If initialization fails, it reports the error name using `esp_err_to_name()` and returns instead of trying to use an uninitialized controller.

The current application does not register a device or perform sensor measurements. It is intentionally a bus discovery demonstration.

## 7. `i2c_bus.h`: the public contract

The header declares functions for bus lifecycle, probing/scanning, device registration, and blocking transfers:

| API | Responsibility |
| --- | --- |
| `i2c_bus_init()` | Allocate/configure I²C master bus |
| `i2c_bus_deinit()` | Release bus resources |
| `i2c_bus_probe(address)` | Check whether an address acknowledges |
| `i2c_bus_scan()` | Count ACK responses across 0x08–0x77 |
| `i2c_bus_add_device(address, speed, out)` | Create a target-specific handle |
| `i2c_bus_remove_device(handle)` | Remove a registered target |
| `i2c_bus_write(handle, data, len)` | Blocking write |
| `i2c_bus_read(handle, data, len)` | Blocking read |
| `i2c_bus_write_read(handle, tx, tx_len, rx, rx_len)` | Combined write/read |

The wrapper returns ESP-IDF error codes for most operations. **Exception:** `i2c_bus_scan()` returns a count, so it cannot directly propagate a distinct failure status to the caller; it prints errors encountered during individual probes.

## 8. `i2c_bus.c`: controller configuration

The implementation configures:

```c
#define I2C_SDA_GPIO 21
#define I2C_SCL_GPIO 22
#define I2C_GLITCH_FILTER 7
#define I2C_PROBE_TIMEOUT_MS 50
#define I2C_TRANSFER_TIMEOUT_MS 100
```

`i2c_master_bus_config_t` selects `I2C_NUM_0`, GPIO21/22, the default clock source, a glitch filter setting, and internal pull-ups. `i2c_new_master_bus()` creates the master bus and returns a handle.

The `static i2c_master_bus_handle_t bus_handle` variable is private to `i2c_bus.c`. `NULL` represents the uninitialized state. A second call to `i2c_bus_init()` returns `ESP_ERR_INVALID_STATE` rather than creating a duplicate bus.

## 9. Bus handle versus device handle

A **bus handle** represents the initialized ESP32 controller and shared SDA/SCL wires. A **device handle** represents one registered target's address and clock configuration on that bus.

```text
I²C master bus handle
  ├── device handle A (address 0x76, chosen speed)
  ├── device handle B (address 0x40, chosen speed)
  └── device handle C (address 0x68, chosen speed)
```

The addresses above are illustrative only. No such devices have been detected in this lab yet. Device registration is not the same as successful communication: adding a handle configures software state; an actual transfer or probe is needed to exercise hardware.

## 10. Address scanner implementation

`i2c_bus_scan()` loops from `0x08` to `0x77`, calls `i2c_bus_probe()` and categorizes the result:

- `ESP_OK`: print the address and increment `devices_found`.
- `ESP_ERR_NOT_FOUND`: target did not acknowledge; continue silently.
- Other error: print an error and continue.

`addresses_checked` increments for each attempted address. A bus error may occur at multiple addresses; therefore a result of zero devices does not, by itself, prove that wiring or electrical conditions are correct. Read the error messages too.

The probe timeout is **50 ms per address**. The maximum elapsed scan time can be substantially longer than a single probe timeout if multiple probes stall.

## 11. Device registration

`i2c_bus_add_device()` validates that the bus is initialized and the output pointer is non-NULL. It builds an `i2c_device_config_t` with 7-bit addressing, the requested address, and `scl_speed_hz`, then calls `i2c_master_bus_add_device()`.

When finished with a registered target, `i2c_bus_remove_device()` passes its handle to `i2c_master_bus_rm_device()`. In a future application, remove registered devices before deleting the bus.

The current wrapper does not explicitly validate the numeric address or clock speed; the underlying driver is responsible for additional validation. A future hardening pass could add wrapper-level checks.

## 12. Blocking writes and reads

`i2c_bus_write()` calls `i2c_master_transmit()`. `i2c_bus_read()` calls `i2c_master_receive()`. Both validate a non-NULL device handle, a non-NULL data buffer, and a nonzero length. They use a **100 ms transaction timeout**.

Blocking means the calling task waits until the driver finishes or returns an error/timeout. This simplifies the first lab but means the calling task cannot perform unrelated work during the transfer.

## 13. Combined write/read transaction

Many register-based devices require the master to first write a register address and then read the corresponding value. The wrapper provides `i2c_bus_write_read()`, implemented with `i2c_master_transmit_receive()`.

Conceptually:

```text
START → TARGET+WRITE → REGISTER ADDRESS → REPEATED START
      → TARGET+READ → DATA BYTE(S) → STOP
```

A combined transaction avoids a separate STOP between the register-selection phase and the read phase. The function is **implemented but not yet exercised against a real sensor**.

## 14. Error handling and resource ownership

The wrapper checks basic invalid states and arguments. Initialization failure is propagated to `app_main()`; probe failures are interpreted by the scanner. On successful deinitialization, `bus_handle` is set back to `NULL`.

There are two useful design caveats:

1. The current `app_main()` does not call `i2c_bus_deinit()` after scanning. For a firmware demonstration that finishes scanning and leaves the controller allocated, this is not an immediate functional issue, but an application with repeated setup/teardown should manage its lifecycle deliberately.
2. The scanner's integer return type cannot distinguish a clean zero-device scan from one that encountered errors. A production interface could return `esp_err_t` and expose the count via an output parameter.

These are possible future improvements, **not changes made in this documentation pass**.

## 15. Build, flash, and monitor

Use an ESP-IDF v5.5.5 environment and an attached ESP32 board. In PowerShell:

```powershell
cd C:\Users\babar\esp32-hardware-interface-labs\i2c\lab02_i2c_master
idf.py --version
idf.py build
idf.py -p COM3 flash
idf.py -p COM3 monitor
```

Exit the monitor with **Ctrl+]**. If the COM port changes, update the `-p` argument. The firmware build and flash steps should be run in the ESP-IDF-enabled terminal.

## 16. Baseline monitor output

The previously observed baseline without an external I²C device was:

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

This shows that initialization and the scanning application executed. It does **not** demonstrate successful communication with a target device.

## 17. Future BMP/BME280 integration (not performed)

A future lab pass will connect a suitable sensor breakout, probe its I²C address, register a device handle, and read the identification register before interpreting measurements. For BMP280/BME280-family devices, register `0xD0` is commonly used for identification; its exact response will need to be verified from hardware. Sensor wiring and measurement validation are deliberately postponed.

## 18. Troubleshooting

| Symptom | First checks |
| --- | --- |
| Build fails | Confirm ESP-IDF terminal, active project directory, and component files |
| Flash fails | Confirm COM port, USB cable, and that another monitor is not using the port |
| Zero devices | Expected with no target; with a target, inspect power, common ground, SDA/SCL and pull-ups |
| Probe errors | Read exact `esp_err_to_name()` output; distinguish NACK from timeout/driver errors |
| Unstable detection | Check wire length, pull-up strength, target voltage and bus clock |
| Stale firmware output | Reflash the intended lab and ensure the monitor uses its build artifacts |

## 19. Interview preparation

**Why wrap the ESP-IDF driver?** To keep application logic independent of hardware setup and consolidate configuration, error handling and lifecycle management.

**What does an ACK prove?** That a target acknowledged a particular bus address during a probe; it does not establish the device's identity or functional correctness.

**Why are pull-ups necessary?** Because I²C uses open-drain/open-collector-style signaling; lines need a mechanism to return HIGH when released.

**Why use a combined write/read operation?** Many register-oriented devices require writing a register index followed by reading data without ending the logical transaction.

**How is this different from UART Lab 01?** UART uses asynchronous point-to-point framing and a command protocol; I²C uses a shared synchronous clock, target addressing, ACK/NACK, and controller-managed transactions.

**What is still unvalidated?** Device registration and real-target transfers, sensor identification, and measurements.

## 20. Key takeaways

- The application uses a reusable bus abstraction rather than direct ESP-IDF calls.
- The ESP32 master uses GPIO21 for SDA and GPIO22 for SCL.
- The scanner checks 112 usable 7-bit addresses and interprets acknowledgements.
- Bus handles and device handles represent different resource scopes.
- Read/write APIs are available for later hardware integration but have not yet been validated against a sensor.
- Documentation-only changes should preserve firmware behavior and be committed separately from future hardware changes.
