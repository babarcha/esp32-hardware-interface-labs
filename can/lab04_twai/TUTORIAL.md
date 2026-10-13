# CAN/TWAI Lab 04 — ESP32 CAN Controller, API Boundary Validation, and Bus Abstraction

**Repository:** `esp32-hardware-interface-labs`

**Lab:** `can/lab04_twai`

**Platform:** ESP32, ESP-IDF v5.5.5, Windows 11

**Firmware milestone:** Pass 7 — Boundary Validation

**Validation scope:** Controller lifecycle and API boundary checks. A physical CAN network has not been validated.

---

## 1. Purpose of the lab

This lab introduces the ESP32's **TWAI (Two-Wire Automotive Interface)** controller, which implements the classical CAN protocol. The firmware provides a small reusable controller abstraction and a test application that deliberately makes invalid API calls to verify that they are rejected with the expected error codes.

The principal learning goals are:

1. Understand CAN frames, identifiers, arbitration, and physical signaling.
2. Distinguish the on-chip CAN controller from an external CAN transceiver.
3. Configure the ESP32 TWAI driver using ESP-IDF.
4. Manage the controller lifecycle: install, start, stop, and uninstall.
5. Validate message pointers, data lengths, identifiers, and call order.
6. Understand what a passing API test proves—and what it does not prove.
7. Prepare for later physical CAN and hardware-in-the-loop (HIL) validation.

**Important:** The existing test program does not transmit a valid frame to a real CAN network. The ten checks are embedded-firmware API checks, not physical bus communication tests.

## 2. What is CAN?

**Controller Area Network (CAN)** is a multi-node, message-oriented serial communication protocol used in vehicles, industrial controllers, machines, and embedded systems.

Unlike a UART point-to-point command channel, a CAN bus can be shared by multiple nodes. A frame carries an **identifier** that describes the message; it is not normally a destination address.

For example, a hypothetical system might use one identifier for motor speed and another for temperature. Interested nodes receive and interpret the corresponding messages.

### 2.1 CAN arbitration

CAN supports nondestructive bitwise arbitration. When multiple nodes start transmitting together, the node transmitting the higher-priority identifier wins arbitration. For otherwise comparable frames, a lower numerical identifier has higher priority.

CAN also includes error detection, acknowledgment, and retransmission mechanisms. These features depend on real bus activity and are **not exercised by the current lab**.

### 2.2 Classical CAN versus CAN FD

The present firmware targets **classical CAN**:

- Standard identifier: 11 bits.
- Extended identifier: 29 bits.
- Data payload: up to 8 bytes in a classical data frame.
- Configured nominal bitrate: 500 kbit/s.

CAN FD permits larger payloads and different data-phase rates, but it is outside this lab's scope.

## 3. TWAI controller versus CAN transceiver

The ESP32 contains a TWAI controller, but the controller pins do not directly provide CANH and CANL differential signaling.

```text
ESP32 application
      |
      v
twai_bus.c / twai_bus.h
      |
      v
ESP-IDF TWAI driver
      |
      v
ESP32 TWAI controller
      |
      +-- TX GPIO21 --> CAN transceiver TXD
      |
      +-- RX GPIO22 <-- CAN transceiver RXD
                             |
                             +-- CANH
                             +-- CANL
                                  |
                              CAN network
                                  |
                           Another CAN node
```

The **controller** manages CAN frames, timing, arbitration, and protocol-related errors. The **transceiver** converts the controller's logic-level TX/RX signals into differential CAN bus signals.

Never connect ESP32 GPIO21 or GPIO22 directly to CANH or CANL.

## 4. Hardware and electrical configuration

| Parameter | Current firmware |
|---|---|
| Controller | ESP32 TWAI |
| TX signal | GPIO21 |
| RX signal | GPIO22 |
| Bitrate | 500 kbit/s |
| Mode | `TWAI_MODE_NORMAL` |
| Filter | Accept all identifiers |
| Frame type | Classical CAN |
| Physical CAN transceiver | Not used in this validation pass |

For future physical testing, obtain a CAN transceiver with **3.3 V-compatible controller-side logic**, a second active CAN node or compatible CAN adapter, and suitable bus wiring and termination.

A conventional linear CAN bus uses 120-ohm termination at each physical end. Termination is a property of the physical bus, not an ESP-IDF software setting. Check the transceiver module's actual voltage requirements and pin labels before connecting it.

## 5. Repository structure

```text
can/lab04_twai/
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt
│   ├── main.c
│   ├── twai_bus.c
│   └── twai_bus.h
├── README.md
└── TUTORIAL.md
```

### File responsibilities

| File | Role |
|---|---|
| `CMakeLists.txt` | Defines the independent ESP-IDF project |
| `main/CMakeLists.txt` | Registers the firmware source files |
| `main.c` | Creates a test frame, runs ten checks, prints results |
| `twai_bus.h` | Declares the reusable TWAI API |
| `twai_bus.c` | Configures ESP-IDF TWAI and validates API usage |
| `README.md` | Concise project overview |
| `TUTORIAL.md` | Detailed learning and implementation guide |

This lab is independent of the UART, I²C, and SPI projects. Build it from `can/lab04_twai`, not from the repository root.

## 6. Architecture and design choice

```text
                 APPLICATION
                    main.c
          test fixture + expectations
                       |
                       | twai_bus_*()
                       v
               PUBLIC INTERFACE
                  twai_bus.h
                       |
                       v
              HARDWARE ABSTRACTION
                  twai_bus.c
       configuration + lifecycle + checks
                       |
                       v
                  ESP-IDF DRIVER
                 driver/twai.h
                       |
                       v
               ESP32 TWAI CONTROLLER
```

The application asks **what behavior should occur**. The bus module knows **how to configure and operate the controller**.

The module is an abstraction of ESP-IDF hardware access, not a platform-independent driver: it still exposes ESP-IDF types such as `esp_err_t` and `twai_message_t`.

## 7. Build configuration: `main/CMakeLists.txt`

The component registration is:

```cmake
idf_component_register(
    SRCS
        "main.c"
        "twai_bus.c"
    INCLUDE_DIRS "."
    REQUIRES driver
)
```

`SRCS` names the compiled C files. `INCLUDE_DIRS "."` lets the sources include `twai_bus.h` from the same folder. `REQUIRES driver` declares the ESP-IDF driver component dependency.

There is no need to compile header files separately; the compiler includes them when processing C source files.

## 8. The public interface: `twai_bus.h`

The header declares six functions:

```c
esp_err_t twai_bus_init(void);
esp_err_t twai_bus_start(void);
esp_err_t twai_bus_stop(void);
esp_err_t twai_bus_deinit(void);

esp_err_t twai_bus_transmit(
    const twai_message_t *message,
    uint32_t timeout_ms);

esp_err_t twai_bus_receive(
    twai_message_t *message,
    uint32_t timeout_ms);
```

### Why use `esp_err_t`?

ESP-IDF APIs return named status codes:

- `ESP_OK`: requested operation completed successfully.
- `ESP_ERR_INVALID_STATE`: operation conflicts with the current controller state.
- `ESP_ERR_INVALID_ARG`: supplied argument is invalid.
- Other codes: may originate from the underlying ESP-IDF driver.

These codes allow a test to compare the **actual result** with an **expected result** instead of merely printing an informal success message.

### Why use `const` for transmit?

```c
const twai_message_t *message
```

The transmit function reads the caller's message; it does not need to modify the message. `const` documents that contract.

The receive function instead accepts a writable `twai_message_t *`, because received frame data must be stored in the caller's object.

## 9. CAN frame representation: `twai_message_t`

The application creates its baseline test fixture:

```c
twai_message_t message = {0};

message.identifier = 0x123;
message.extd = 0;
message.rtr = 0;
message.data_length_code = 3;

message.data[0] = 0xAA;
message.data[1] = 0xBB;
message.data[2] = 0xCC;
```

The `{0}` initializer clears the structure's fields before the application assigns the fields it needs.

| Field | Current value | Meaning |
|---|---|---|
| `identifier` | `0x123` | CAN arbitration identifier |
| `extd` | `0` | Standard 11-bit identifier |
| `rtr` | `0` | Data frame, not remote request |
| `data_length_code` | `3` | Three payload bytes |
| `data[0..2]` | `AA BB CC` | Example payload |

This is a **valid frame fixture** used as the starting point for invalid-input tests. The current application does not transmit this valid frame while the controller is running.

## 10. CAN identifier boundaries

### Standard format

An 11-bit identifier can represent:

```text
Minimum: 0x000
Maximum: 0x7FF
```

Therefore, `0x800` is invalid for a standard frame.

### Extended format

A 29-bit identifier can represent:

```text
Minimum: 0x00000000
Maximum: 0x1FFFFFFF
```

Therefore, `0x20000000` is invalid for an extended frame.

The firmware explicitly checks these boundaries before calling the ESP-IDF transmit function.

## 11. Controller configuration: `twai_bus.c`

The implementation defines:

```c
#define TWAI_TX_GPIO 21
#define TWAI_RX_GPIO 22
```

Inside `twai_bus_init()`, it creates three configuration objects:

```c
twai_general_config_t general_config =
    TWAI_GENERAL_CONFIG_DEFAULT(
        TWAI_TX_GPIO,
        TWAI_RX_GPIO,
        TWAI_MODE_NORMAL);

twai_timing_config_t timing_config =
    TWAI_TIMING_CONFIG_500KBITS();

twai_filter_config_t filter_config =
    TWAI_FILTER_CONFIG_ACCEPT_ALL();
```

### General configuration

Selects controller-side pins and operating mode. **Normal mode** is intended for participation in a physical CAN network. It does not create a simulated CAN peer.

### Timing configuration

Selects a nominal rate of 500 kbit/s. Communicating nodes on the same bus must use compatible timing.

### Acceptance filter

`TWAI_FILTER_CONFIG_ACCEPT_ALL()` allows frames with any identifier through the hardware acceptance filter. This does not mean the application has received any frames.

The driver is installed using:

```c
esp_err_t result = twai_driver_install(
    &general_config,
    &timing_config,
    &filter_config);
```

If installation succeeds, the module records that state and prints configuration information.

## 12. Software state tracking

The module owns two file-local variables:

```c
static bool driver_installed = false;
static bool controller_started = false;
```

`static` at file scope limits direct access to `twai_bus.c`. The application cannot accidentally overwrite these flags.

They represent the abstraction's known lifecycle state:

| `driver_installed` | `controller_started` | Meaning |
|---|---|---|
| `false` | `false` | Driver absent |
| `true` | `false` | Driver installed, controller stopped |
| `true` | `true` | Controller running |
| `false` | `true` | Not a valid intended state |

The flags are updated only after successful underlying driver calls.

They are **not** mutexes and do not make the API safe for simultaneous unsynchronized calls from multiple tasks.

## 13. Lifecycle state machine

```text
       [UNINSTALLED]
             |
             | twai_bus_init()
             v
      [INSTALLED / STOPPED]
             |
             | twai_bus_start()
             v
          [RUNNING]
             |
             | twai_bus_stop()
             v
      [INSTALLED / STOPPED]
             |
             | twai_bus_deinit()
             v
       [UNINSTALLED]
```

The wrapper rejects operations that violate this sequence.

Examples:

- Calling `twai_bus_start()` twice is invalid.
- Calling `twai_bus_stop()` twice is invalid.
- Calling `twai_bus_deinit()` while running is invalid.
- Calling `twai_bus_transmit()` before initialization is invalid.

Explicit lifecycle rules are important in embedded systems because peripheral drivers own finite hardware and memory resources.

## 14. Driver installation: `twai_bus_init()`

The first check is:

```c
if (driver_installed)
{
    return ESP_ERR_INVALID_STATE;
}
```

This prevents the wrapper from installing its driver twice.

The function then calls `twai_driver_install()`. Only after `ESP_OK` does it set:

```c
driver_installed = true;
```

It also prints the TX pin, RX pin, bitrate, and mode.

**Installation does not start the controller.** A separate `twai_bus_start()` call is required.

## 15. Starting the controller: `twai_bus_start()`

The wrapper first checks:

```c
if (!driver_installed)
{
    return ESP_ERR_INVALID_STATE;
}

if (controller_started)
{
    return ESP_ERR_INVALID_STATE;
}
```

If the call is legal, it invokes:

```c
esp_err_t result = twai_start();
```

On success:

```c
controller_started = true;
```

The controller is now logically in the running state, but this alone does not demonstrate that CANH/CANL wiring or another node exists.

## 16. Stopping the controller: `twai_bus_stop()`

The stop operation rejects requests unless the driver is installed and running:

```c
if (!driver_installed || !controller_started)
{
    return ESP_ERR_INVALID_STATE;
}
```

It then calls `twai_stop()`, and clears `controller_started` only on success.

Stopping is a required part of the normal shutdown sequence before driver uninstall.

## 17. Driver removal: `twai_bus_deinit()`

The deinitialization function rejects:

1. An attempt to uninstall a driver that was never installed.
2. An attempt to uninstall while the controller is still running.

It calls:

```c
esp_err_t result = twai_driver_uninstall();
```

Only after successful uninstall does it set:

```c
driver_installed = false;
```

This preserves the module's state when the driver reports an error.

## 18. Transmit function: `twai_bus_transmit()`

The transmit wrapper performs input checks **before** delegating to the ESP-IDF driver.

```c
if (!driver_installed || !controller_started)
{
    return ESP_ERR_INVALID_STATE;
}

if (message == NULL)
{
    return ESP_ERR_INVALID_ARG;
}

if (message->data_length_code > 8)
{
    return ESP_ERR_INVALID_ARG;
}
```

Then it validates the identifier according to `extd`:

```c
if (!message->extd &&
    message->identifier > 0x7FF)
{
    return ESP_ERR_INVALID_ARG;
}

if (message->extd &&
    message->identifier > 0x1FFFFFFF)
{
    return ESP_ERR_INVALID_ARG;
}
```

Only a valid call reaches:

```c
return twai_transmit(
    message,
    pdMS_TO_TICKS(timeout_ms));
```

### Why convert milliseconds?

The wrapper accepts timeout values in milliseconds, a convenient unit for callers. The FreeRTOS driver API expects ticks. `pdMS_TO_TICKS()` performs the conversion using the configured RTOS tick rate.

### What does a successful transmit API call mean?

A successful enqueue operation does not necessarily mean another CAN node has received the frame. Actual physical transmission, acknowledgment, retries, and bus faults require additional observation.

**The current test application never reaches `twai_transmit()` with a valid frame while running.**

## 19. Receive function: `twai_bus_receive()`

The receive wrapper checks controller state and the output pointer:

```c
if (!driver_installed || !controller_started)
{
    return ESP_ERR_INVALID_STATE;
}

if (message == NULL)
{
    return ESP_ERR_INVALID_ARG;
}
```

It then delegates:

```c
return twai_receive(
    message,
    pdMS_TO_TICKS(timeout_ms));
```

The current application tests **receive after stop**, which must return `ESP_ERR_INVALID_STATE`. It does not test successful receipt of a real frame.

## 20. Application test harness: `main.c`

The application declares:

```c
static int tests_passed = 0;
static int tests_failed = 0;
```

Its helper is:

```c
static void check_result(
    const char *name,
    esp_err_t actual,
    esp_err_t expected)
```

For each check, the helper compares `actual` with `expected`. Matching codes increment `tests_passed`; mismatches increment `tests_failed` and print the expected and actual error names.

A test is successful because **the expected behavior occurred**, not because the API returned `ESP_OK`.

For example, rejecting an invalid DLC with `ESP_ERR_INVALID_ARG` is a successful negative test.

## 21. The ten boundary-validation tests

| Test | Operation | Expected |
|---:|---|---|
| 1 | Transmit before initialization | `ESP_ERR_INVALID_STATE` |
| 2 | Start controller twice | `ESP_ERR_INVALID_STATE` |
| 3 | Transmit `NULL` message | `ESP_ERR_INVALID_ARG` |
| 4 | Transmit frame with DLC = 9 | `ESP_ERR_INVALID_ARG` |
| 5 | Standard ID = `0x800` | `ESP_ERR_INVALID_ARG` |
| 6 | Extended ID = `0x20000000` | `ESP_ERR_INVALID_ARG` |
| 7 | Deinitialize while running | `ESP_ERR_INVALID_STATE` |
| 8 | Stop controller twice | `ESP_ERR_INVALID_STATE` |
| 9 | Receive after stop | `ESP_ERR_INVALID_STATE` |
| 10 | Normal deinitialization | `ESP_OK` |

The tests exercise the wrapper's explicit state and argument checks and the normal driver lifecycle.

### Test 1: transmit before initialization

The application calls `twai_bus_transmit()` before `twai_bus_init()`. The wrapper rejects the call because the driver is not installed.

### Test 2: start twice

After successful initialization and start, a second start call is rejected.

### Test 3: NULL message

A running controller receives a transmit request with a `NULL` pointer. The wrapper returns `ESP_ERR_INVALID_ARG` without dereferencing the pointer.

### Test 4: DLC = 9

The test temporarily sets `data_length_code = 9`. Since the wrapper supports classical CAN with at most eight payload bytes, the request is rejected. The original DLC of three is restored afterward.

### Test 5: invalid standard identifier

The test sets `extd = 0` and `identifier = 0x800`. That exceeds the maximum 11-bit identifier.

### Test 6: invalid extended identifier

The test sets `extd = 1` and `identifier = 0x20000000`. That exceeds the maximum 29-bit identifier. The fixture is then restored to standard ID `0x123`.

### Test 7: deinitialize while running

The application attempts to uninstall the driver without first stopping the controller. The wrapper rejects the operation.

### Test 8: stop twice

After a successful normal stop, a second stop request is rejected.

### Test 9: receive after stop

The application attempts to receive a frame while the controller is stopped. The wrapper returns `ESP_ERR_INVALID_STATE` before accessing the receive queue.

### Test 10: normal deinitialization

The driver is still installed but stopped, so `twai_bus_deinit()` is expected to return `ESP_OK`.

## 22. Fatal errors and early exits

The ten checks run only when initialization and the normal lifecycle steps succeed.

### Initialization failure

If `twai_bus_init()` fails, `main.c` prints a fatal error and returns. The test summary is not printed.

### Start failure

If `twai_bus_start()` fails, the application prints the failure and attempts `twai_bus_deinit()` to release the installed driver. It also reports a cleanup error if deinitialization fails.

This cleanup behavior is present in the current firmware and should not be confused with an additional boundary test.

### Stop failure

If the normal `twai_bus_stop()` call fails, the application reports a fatal error and returns. The current code does not attempt further recovery in this path.

### Why not count fatal setup errors as ordinary test failures?

The test harness cannot reliably exercise later state-dependent checks if the controller never reaches the required state. An early exit avoids misleading results.

## 23. Understanding expected console output

If initialization, start, stop, and all ten comparisons succeed, the final summary is expected to look like:

```text
Validation summary
------------------
Passed : 10
Failed : 0
RESULT : ALL TESTS PASSED
```

This is **illustrative expected output**, not evidence that the current version has been flashed and executed on hardware.

An individual check prints `PASS` when the actual return code matches the expected return code.

For a mismatch, the helper prints the expected and actual symbolic ESP-IDF error names.

### What does “ALL TESTS PASSED” establish?

It establishes that the ten tested expectations matched during that run.

It does **not** establish:

- Valid CAN frame transmission to a physical bus.
- Receipt or acknowledgment by another node.
- Correct CANH/CANL electrical waveforms.
- Arbitration behavior under competing transmitters.
- Bus-off detection or recovery.
- Payload integrity over a real network.

## 24. Build, flash, and monitor

Use an ESP-IDF-enabled PowerShell or ESP-IDF terminal.

```powershell
cd C:\Users\babar\esp32-hardware-interface-labs\can\lab04_twai

idf.py --version
idf.py build
idf.py -p COM3 flash
idf.py -p COM3 monitor
```

Exit the monitor using **Ctrl+]**.

`COM3` is the port previously used for the ESP32 board. If Windows assigns another port, replace `COM3` accordingly.

### If a stale build environment causes errors

From the **lab directory**, you can use:

```powershell
idf.py fullclean
idf.py build
```

`fullclean` removes generated build artifacts; it does not delete the firmware source. It is unnecessary for routine builds when the build cache is healthy.

### What a successful build proves

A successful build confirms the project compiled and linked for the configured target. It does not prove runtime behavior or physical CAN communication.

## 25. Debugging and troubleshooting

| Symptom | Likely investigation |
|---|---|
| `idf.py` not recognized | Open an ESP-IDF-enabled terminal |
| Build from wrong folder | Navigate to `can/lab04_twai` |
| Flash cannot open COM3 | Check cable, port number, and other serial monitors |
| Driver install fails | Inspect ESP-IDF logs, pin conflicts, and existing TWAI ownership |
| Controller start fails | Inspect the returned error and configuration |
| Boundary test reports FAIL | Compare actual and expected error codes |
| No CANH/CANL traffic | Current lab has no transceiver or valid-frame traffic |
| Physical node never ACKs | Check transceiver, wiring, termination, bitrate, and peer node |
| Monitor shows another lab | Verify the correct firmware was flashed and the correct build is monitored |

## 26. Validation levels: what is tested?

It is useful to distinguish three forms of evidence:

| Level | Example | Status |
|---|---|---|
| Build verification | `idf.py build` succeeds | Previously confirmed for this lab |
| Firmware API validation | Ten boundary checks produce expected return codes | Implemented; verify with actual monitor logs |
| Physical CAN validation | Frames travel between transceiver-equipped nodes | Not performed |

This separation is important in embedded engineering interviews and validation reports. Avoid saying “CAN communication validated” when only API boundary behavior was tested.

## 27. Preparing a future physical CAN lab

A physical extension would add:

1. A suitable CAN transceiver connected to TWAI TX/RX.
2. A second CAN-capable node or USB-CAN interface.
3. Correct CANH/CANL wiring, common reference as required, and termination.
4. A known bitrate shared by all nodes.
5. A valid frame transmit test and a matching receive test.
6. Logged evidence of identifiers, DLC, payloads, acknowledgments, and errors.

### Proposed validation sequence

```text
Power off
   |
   v
Connect transceiver and CAN bus
   |
   v
Verify supply and signal voltages
   |
   v
Configure both nodes for 500 kbit/s
   |
   v
Transmit known valid frame
   |
   v
Receive and compare ID + DLC + payload
   |
   v
Exercise timeouts and recovery
```

Do not treat a successful `twai_transmit()` queue operation alone as proof that a remote node received the frame.

## 28. Suggested future software tests

The current lab deliberately remains small. Possible extensions include:

- Initialize twice.
- Deinitialize before initialization.
- Start before installation.
- Stop before start.
- Receive with a `NULL` pointer while running.
- Valid standard and extended frame transmissions with hardware.
- Queue-full and timeout handling.
- Bus-off and recovery under controlled conditions.
- Host-driven automated testing through a UART command interface.
- Concurrent access protection if multiple tasks use the abstraction.

These are **future ideas**, not features claimed to exist in the present source.

## 29. Comparison with the earlier interface labs

| Lab | Interface | Main learning focus |
|---|---|---|
| UART 01 | UART | Byte streams, line framing, command/response |
| I²C 02 | I²C | Shared bus, addresses, ACK/NACK, device handles |
| SPI 03 | SPI | Clock modes, chip select, full-duplex transfers |
| CAN/TWAI 04 | CAN controller | Message identifiers, controller state, API boundaries |

UART, I²C, and SPI typically connect a microcontroller to local peripherals. CAN is designed for communication among networked controllers and requires additional physical-layer hardware.

## 30. Interview preparation

**Q: What is the difference between TWAI and CAN?**

A: TWAI is Espressif's name for the on-chip CAN-compatible controller peripheral. A complete physical CAN network additionally needs transceivers and CANH/CANL wiring.

**Q: Why is a CAN transceiver required?**

A: The controller produces logic-level TX/RX signals, while a CAN bus uses differential physical signaling. The transceiver bridges those electrical interfaces.

**Q: Why separate installation and start?**

A: Installation allocates and configures driver resources; starting activates the controller. Separate states make lifecycle management explicit.

**Q: Why does DLC = 9 fail?**

A: This lab supports classical CAN data frames with a maximum payload of eight bytes, so the wrapper rejects DLC values greater than eight.

**Q: What is the difference between standard and extended identifiers?**

A: Standard CAN identifiers use 11 bits; extended identifiers use 29 bits. The wrapper validates each format's maximum value.

**Q: Why test invalid input before calling the driver?**

A: Early validation makes the wrapper's contract explicit and prevents invalid arguments from reaching lower-level code.

**Q: Does a passing test prove CAN communication?**

A: No. These tests verify expected error codes and lifecycle behavior. Real bus communication requires a transceiver, a bus, another node, and separate validation evidence.

**Q: Is this a hardware-in-the-loop test?**

A: The checks execute on the ESP32 firmware and exercise the installed controller driver, but the current application is not a host-driven HIL test suite and does not include a physical CAN network.

**Q: What would you test next?**

A: Known-frame transmission and reception with a second node, then timing, filtering, error handling, and recovery under controlled conditions.

## 31. Learning exercises

### Exercise A — Trace the lifecycle

For each call below, predict the return code:

```text
twai_bus_start()       // before init
twai_bus_init()
twai_bus_init()        // duplicate
twai_bus_start()
twai_bus_stop()
twai_bus_deinit()
```

Identify which operations should be rejected by the wrapper and explain why.

### Exercise B — Identifier limits

For each pair, decide whether the wrapper should accept the identifier range:

```text
extd = 0, identifier = 0x7FF
extd = 0, identifier = 0x800
extd = 1, identifier = 0x1FFFFFFF
extd = 1, identifier = 0x20000000
```

Do not transmit frames on a physical network merely to check these numerical boundaries; the wrapper can reject out-of-range values locally.

### Exercise C — Explain test evidence

Write one sentence each describing what these results prove:

```text
idf.py build succeeded
Passed : 10 / Failed : 0
Received expected frame from a second CAN node
```

Notice that these are three different kinds of evidence.

## 32. Completion checklist

- [ ] I can distinguish the CAN controller from the CAN transceiver.
- [ ] I understand standard and extended identifier limits.
- [ ] I can explain the six public TWAI wrapper functions.
- [ ] I can trace the install/start/stop/uninstall state machine.
- [ ] I understand all ten boundary checks in `main.c`.
- [ ] I can distinguish expected return codes from actual CAN traffic.
- [ ] I can build the lab from the correct ESP-IDF project directory.
- [ ] I know what additional hardware is needed for real CAN validation.

## 33. Key takeaways

This lab provides a reusable TWAI controller abstraction and a firmware test harness that checks ten API boundaries. It teaches correct peripheral lifecycle management, error-code validation, and CAN frame limits.

The next engineering milestone is **physical CAN communication testing**, not another software-only claim of successful networking. The firmware should remain simple and modular until a real transceiver and second CAN node are available.
