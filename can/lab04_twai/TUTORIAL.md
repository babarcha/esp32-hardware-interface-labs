# CAN/TWAI Lab 04 — Beginner Tutorial

**Repository:** `esp32-hardware-interface-labs/can/lab04_twai`  
**Platform:** ESP32, ESP-IDF v5.5.5  
**Current pass:** Pass 7 — Boundary Validation  
**Scope:** Software/API validation. Physical CAN networking has **not** been established.

## 1. Learning objectives

By the end of this lab, you should be able to explain what a CAN controller does, why a separate transceiver is required, how a CAN frame is represented, how ESP-IDF installs and starts the TWAI driver, and how an application checks invalid state transitions and malformed frame fields. You should also distinguish a successful API boundary test from an actual CAN communication test.

## 2. What is CAN?

**Controller Area Network (CAN)** is a multi-node, message-oriented communication bus widely used in automotive and industrial embedded systems. Devices broadcast frames identified by message IDs rather than by destination device addresses. Nodes decide which identifiers are relevant to them.

CAN uses arbitration: when nodes attempt transmission simultaneously, the identifier's bit pattern determines which frame continues without corrupting the winning frame. The lower numerical identifier generally has higher arbitration priority for otherwise comparable frames.

This lab uses **classical CAN**, not CAN FD.

## 3. TWAI versus CAN

ESP-IDF calls the ESP32 CAN controller peripheral **TWAI** (Two-Wire Automotive Interface). The on-chip controller handles CAN framing, timing, arbitration, and error mechanisms. It does **not** directly generate the physical differential CANH/CANL bus signaling.

Physical path:

```text
Application main.c
        |
        v
TWAI abstraction twai_bus.c/.h
        |
        v
ESP-IDF driver/twai.h
        |
        v
ESP32 TWAI controller (TX GPIO21, RX GPIO22)
        |
        v
External CAN transceiver (not yet connected)
        |
        v
CANH / CANL wiring and termination
        |
        v
Another CAN node
```

The transceiver converts logic-level controller TX/RX signals to differential bus signals. It must be compatible with the ESP32's 3.3 V logic and the CAN bus voltage requirements. **Do not connect ESP32 GPIO pins directly to CANH/CANL.**

## 4. Hardware and configuration

| Setting | Existing source value |
|---|---|
| TWAI TX | GPIO21 |
| TWAI RX | GPIO22 |
| Bitrate | 500 kbit/s |
| Driver mode | `TWAI_MODE_NORMAL` |
| Acceptance filter | `TWAI_FILTER_CONFIG_ACCEPT_ALL()` |
| Frame format tested | Classical CAN, standard and extended IDs |

GPIO21/GPIO22 are **controller-side** signals. They are not the CANH/CANL differential pair. A real two-ended bus normally needs proper termination (typically 120 ohms at each physical end), a compatible transceiver at each node, common reference as appropriate, and another active node to acknowledge traffic. These components are not part of the present software-only validation pass.

## 5. Repository layout

```text
can/lab04_twai/
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt
│   ├── main.c           Application and 10 boundary checks
│   ├── twai_bus.c       ESP-IDF TWAI abstraction
│   └── twai_bus.h       Public abstraction API
├── README.md
└── TUTORIAL.md          This tutorial
```

The top-level lab folder is an independent ESP-IDF project. Run `idf.py` inside `can/lab04_twai`, not from the repository root.

## 6. Architecture and separation of responsibilities

- `main.c` constructs a test frame, invokes the public functions, and checks return codes.
- `twai_bus.h` declares the operations available to application code.
- `twai_bus.c` knows which GPIO pins, bitrate, mode, and ESP-IDF calls are used.
- The ESP-IDF driver controls the on-chip TWAI peripheral.

This separation allows the application to focus on behavior and validation rather than hardware setup details. The abstraction is still ESP-IDF-specific because its API uses `esp_err_t` and `twai_message_t`.

## 7. CAN frame anatomy

The application initializes:

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

| Field | Meaning |
|---|---|
| `identifier` | Message arbitration identifier (`0x123`) |
| `extd = 0` | Standard 11-bit identifier format |
| `rtr = 0` | Data frame, not remote request |
| `data_length_code = 3` | Three payload bytes |
| `data[]` | Payload `AA BB CC` |

A standard identifier fits in 11 bits (`0x000` through `0x7FF`). An extended identifier fits in 29 bits (`0x00000000` through `0x1FFFFFFF`). Classical CAN carries up to eight data bytes in this lab's API.

## 8. `twai_bus.h` — public interface

```c
esp_err_t twai_bus_init(void);
esp_err_t twai_bus_start(void);
esp_err_t twai_bus_stop(void);
esp_err_t twai_bus_deinit(void);
esp_err_t twai_bus_transmit(const twai_message_t *message, uint32_t timeout_ms);
esp_err_t twai_bus_receive(twai_message_t *message, uint32_t timeout_ms);
```

The return type `esp_err_t` makes failure handling explicit. A function can return `ESP_OK`, `ESP_ERR_INVALID_STATE`, `ESP_ERR_INVALID_ARG`, or an error from the underlying driver. The transmit/receive timeouts are passed in **milliseconds** by the abstraction and converted to FreeRTOS ticks internally.

## 9. `twai_bus.c` — lifecycle state

```c
static bool driver_installed = false;
static bool controller_started = false;
```

These are file-local software flags. They track whether the abstraction successfully installed the driver and started the controller. They prevent invalid operations such as starting twice, stopping before starting, or uninstalling a running controller. They are not a substitute for inspecting all hardware error states in a production system.

Normal sequence:

```text
Uninstalled
   | twai_bus_init()
   v
Installed, stopped
   | twai_bus_start()
   v
Running
   | twai_bus_stop()
   v
Installed, stopped
   | twai_bus_deinit()
   v
Uninstalled
```

## 10. Installing the driver — `twai_bus_init()`

The implementation builds three configuration structures:

```c
TWAI_GENERAL_CONFIG_DEFAULT(TWAI_TX_GPIO, TWAI_RX_GPIO, TWAI_MODE_NORMAL)
TWAI_TIMING_CONFIG_500KBITS()
TWAI_FILTER_CONFIG_ACCEPT_ALL()
```

The general configuration chooses GPIOs and operating mode. The timing configuration sets the nominal CAN bitrate. The acceptance filter allows incoming frames with any identifier. `twai_driver_install()` installs the driver; the `driver_installed` flag changes to `true` only after `ESP_OK`.

**Installed is not the same as started.** The driver must be started separately before transmit/receive calls are permitted by the abstraction.

## 11. Starting and stopping

`twai_bus_start()` checks that the driver exists and the controller is not already running, then calls `twai_start()`. `twai_bus_stop()` checks the opposite conditions and calls `twai_stop()`. The software state changes only when the driver operation succeeds.

`twai_bus_deinit()` refuses to uninstall while `controller_started` is true. After stopping, `twai_driver_uninstall()` releases the driver and clears the installed flag on success.

## 12. Transmit validation — `twai_bus_transmit()`

The abstraction checks the following, in order:

1. Driver installed **and** controller started.
2. Message pointer is not `NULL`.
3. Data length code is no greater than eight.
4. Standard identifier does not exceed `0x7FF` when `extd == 0`.
5. Extended identifier does not exceed `0x1FFFFFFF` when `extd != 0`.

Only after passing these checks does it call:

```c
return twai_transmit(message, pdMS_TO_TICKS(timeout_ms));
```

The current test application intentionally avoids transmitting a valid frame while running in normal mode without a physical CAN network. It tests rejected calls instead. A successful return from `twai_transmit()` would not, by itself, prove successful end-to-end application-level reception.

## 13. Receive validation — `twai_bus_receive()`

The receive function requires a running controller and a non-NULL output pointer, then delegates to:

```c
return twai_receive(message, pdMS_TO_TICKS(timeout_ms));
```

A stopped controller is rejected before the driver call. The existing application tests this invalid-state case, not reception of a real CAN frame.

## 14. `main.c` — test harness

`check_result(name, actual, expected)` prints PASS when the returned error matches the expected error and increments `tests_passed`; otherwise it prints both error names and increments `tests_failed`.

The test counters summarize API expectations, not successful network messages. If initialization or start fails, the application prints a fatal error and exits early instead of continuing through all checks.

## 15. The 10 boundary tests

| # | Scenario | Expected result |
|---:|---|---|
| 1 | Transmit before initialization | `ESP_ERR_INVALID_STATE` |
| 2 | Start controller twice | `ESP_ERR_INVALID_STATE` |
| 3 | Transmit a NULL message | `ESP_ERR_INVALID_ARG` |
| 4 | Classical CAN DLC = 9 | `ESP_ERR_INVALID_ARG` |
| 5 | Standard identifier = `0x800` | `ESP_ERR_INVALID_ARG` |
| 6 | Extended identifier = `0x20000000` | `ESP_ERR_INVALID_ARG` |
| 7 | Deinitialize while running | `ESP_ERR_INVALID_STATE` |
| 8 | Stop controller twice | `ESP_ERR_INVALID_STATE` |
| 9 | Receive after stop | `ESP_ERR_INVALID_STATE` |
| 10 | Deinitialize stopped controller | `ESP_OK` |

These are deterministic negative and lifecycle tests when driver initialization/start/stop behave as expected. The application does **not** exercise arbitration, physical signaling, ACK, real transmit success, receive payloads, or bus-off recovery.

## 16. Why boundary testing matters

Embedded interfaces can fail from invalid call order, invalid pointers, out-of-range identifiers, and incorrect frame lengths. Rejecting invalid input before invoking a hardware driver makes failures easier to diagnose and reduces undefined application behavior. Boundary testing also produces evidence for a validation matrix: each scenario has a specified input and expected return code.

This is not yet a host-driven automated HIL test suite. The checks execute in firmware and report to the serial console.

## 17. Build, flash, monitor

Activate the ESP-IDF PowerShell environment, then:

```powershell
cd C:\Users\babar\esp32-hardware-interface-labs\can\lab04_twai
idf.py build
idf.py -p COM3 flash
idf.py -p COM3 monitor
```

Exit the serial monitor with **Ctrl + ]**. If ESP-IDF reports that the active Python environment differs from the build cache, run `idf.py fullclean` and then `idf.py build` in this lab directory. `fullclean` removes generated build files, not application source files.

## 18. How to interpret runtime output

When all expected error codes match, the program prints:

```text
Validation summary
------------------
Passed : 10
Failed : 0
RESULT : ALL TESTS PASSED
```

This is the **expected** output, not a claim that the current documentation pass has flashed the board or observed that result. Check your actual monitor output before recording validation as passed.

## 19. Common troubleshooting

**Build fails:** Verify ESP-IDF v5.5.5 environment activation, current working directory, and `main/CMakeLists.txt` registration. Use `idf.py fullclean` for stale build-cache Python paths.

**Driver installation fails:** Check ESP-IDF logs, GPIO availability, conflicting peripheral ownership, and any previous TWAI driver installation.

**Serial monitor busy:** Close another serial terminal or monitor session holding COM3.

**Physical transmit does not work:** The controller GPIOs are not a CAN bus. Add a compatible transceiver, correct CANH/CANL wiring, termination, shared bitrate, and an ACK-capable second node before expecting real traffic.

**A test reports FAIL:** Read the `expected` and `actual` error names. Distinguish a failed expectation from an earlier fatal driver initialization error.

## 20. Physical CAN validation — future pass

A future hardware pass can add a 3.3 V-logic-compatible CAN transceiver, a second CAN node (or CAN adapter), appropriate termination, and controlled test frames. Then verify: bus bitrate, TX/RX wiring, frame identifiers, payload integrity, receive filtering, arbitration under contention, error counters, timeouts, and recovery. Record evidence from serial logs and ideally a CAN analyzer.

Do not connect the ESP32 directly to a vehicle CAN network without suitable isolation/protection and a well-defined test setup.

## 21. Suggested future improvements (not implemented here)

- Add positive transmit/receive tests with a physical second node.
- Add an explicit test for receive with a NULL pointer while running.
- Test repeated init and deinit before init.
- Test timeout and queue-full behavior.
- Test bus-off and recovery behavior with safe hardware.
- Make hardware pin assignments configurable.
- Consider synchronizing lifecycle state if multiple tasks may call the API concurrently.
- Separate pure validation logic for host-side unit testing.

These are possible enhancements, **not** functionality claimed for the current repository.

## 22. Interview questions and answers

**Q: What is the difference between a CAN controller and transceiver?**  
A: The controller manages frame formatting, arbitration, and protocol logic; the transceiver interfaces the controller to differential CANH/CANL physical signaling.

**Q: Why are there separate install and start functions?**  
A: Installation allocates/configures the driver; starting transitions the controller into the operational state. This enables explicit lifecycle management.

**Q: Why test a DLC of nine?**  
A: The current API supports classical CAN data payloads of up to eight bytes; nine is an out-of-range boundary value.

**Q: What does `ESP_ERR_INVALID_STATE` mean in this lab?**  
A: The requested operation conflicts with the abstraction's current lifecycle, such as transmitting before start or uninstalling while running.

**Q: Does `RESULT : ALL TESTS PASSED` prove CAN networking?**  
A: No. It proves the checked API expectations matched; no external transceiver, bus, or second node was exercised.

**Q: Why use an abstraction?**  
A: To isolate ESP-IDF configuration and state management from the application and expose a small reusable API.

## 23. Lab completion criteria

- [ ] The four documentation files are copied into their correct paths.
- [ ] `git diff --check` and staged whitespace checks pass.
- [ ] The source diff is verified as comment-only.
- [ ] `idf.py build` succeeds after replacing the source files.
- [ ] Documentation is committed and pushed.
- [ ] Hardware runtime verification is recorded separately, if performed.

**Bottom line:** This lab establishes a structured ESP32 TWAI abstraction and tests invalid operations and frame boundaries. A real CAN network validation pass remains future work.
