# UART Lab 01 — ESP32 UART Command/Response and Hardware-in-the-Loop Testing

**Repository:** `esp32-hardware-interface-labs`  
**Lab:** `uart/lab01_uart_loopback`  
**Platform:** ESP32, ESP-IDF v5.5.5, Windows 11, Python  
**Status:** Modular firmware and host-side validation implemented; rerun hardware tests after firmware changes.

## 1. Purpose and learning objectives

This lab introduces UART communication between an ESP32 and a Windows PC. The ESP32 receives newline-terminated commands, processes them, and returns deterministic responses. A Python client can communicate with the board, and pytest can automate checks against a physical device.

By the end, you should understand:

- UART signals, 115200 8N1 configuration, and USB-to-serial communication.
- The difference between byte transport, line framing, and command interpretation.
- ESP-IDF UART initialization, buffering, timeouts, and resource lifecycle.
- Separation of application, peripheral abstraction, and protocol logic.
- Host-driven testing and the distinction between successful builds and physical HIL validation.

Despite the directory name `loopback`, this lab implements a **command/response protocol**, not a direct electrical TX-to-RX loopback test.

## 2. Project structure

```text
uart/lab01_uart_loopback/
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt
│   ├── main.c              Application orchestration
│   ├── uart_bus.c          UART0 initialization, read/write, cleanup
│   ├── uart_bus.h          UART public API
│   ├── protocol.c          Command processing
│   └── protocol.h          Protocol public API
├── host/
│   └── uart_client.py      Manual PC-side serial client
├── tests/
│   └── test_uart_protocol.py  Host-side protocol tests
└── TUTORIAL.md
```

The firmware is intentionally small and divided by responsibility. `main.c` coordinates the application; `uart_bus.c` deals with ESP-IDF and serial I/O; `protocol.c` knows the commands. The Python tools are outside the firmware and do not need to be compiled into the ESP32 image.

## 3. End-to-end architecture

```text
Windows PC                          ESP32
──────────                          ─────
Python / pytest
     │
     ▼
PySerial (COM3, 115200)
     │
     ▼
USB-to-serial bridge ──────────► UART0 (ESP-IDF driver)
                                      │
                                      ▼
                              uart_bus_read_line()
                                      │ command string
                                      ▼
                                 app_main()
                                      │
                                      ▼
                            protocol_process_command()
                                      │ response string
                                      ▼
                              uart_bus_write_line()
                                      │
                                      ▼
PC receives response ◄────────── UART0 / USB bridge
```

This is a layered design, not three independent running programs inside the ESP32. `app_main()` calls the other two firmware modules synchronously.

## 4. UART fundamentals

UART stands for **Universal Asynchronous Receiver/Transmitter**. It sends serial bits without a separate clock wire. Both endpoints agree on a baud rate and frame format.

A basic UART link requires:

```text
Device A TX  ─────► Device B RX
Device A RX  ◄───── Device B TX
Device A GND ───── Device B GND
```

The lab uses **115200 8N1**:

| Parameter | Value | Meaning |
|---|---|---|
| Baud rate | 115200 | Nominal symbols per second |
| Data bits | 8 | Eight payload bits per character |
| Parity | None | No parity bit |
| Stop bits | 1 | One stop bit |
| Flow control | None | No hardware RTS/CTS flow control |

The development board's USB-to-serial bridge normally connects to **UART0**, which also carries boot and console logs. The host therefore may see ESP-IDF logging mixed with application responses. The lab's debug line `CMD: ...` is intentionally preserved because existing host-side tests use it for synchronization.

## 5. Firmware API boundaries

### `uart_bus.h` — public UART interface

```c
esp_err_t uart_bus_init(void);
esp_err_t uart_bus_deinit(void);
esp_err_t uart_bus_read_line(char *buffer, size_t capacity);
esp_err_t uart_bus_write_line(const char *line);
```

The caller does not need to know the UART port, buffer sizes, or low-level driver calls. Errors are reported using ESP-IDF's `esp_err_t` type.

### `protocol.h` — public command interface

```c
void protocol_process_command(
    const char *command,
    char *response,
    size_t response_size);
```

The protocol module receives a complete command string and writes a response into the caller-provided buffer. It does not access UART hardware.

## 6. `uart_bus.c` — UART configuration and initialization

The module defines:

```c
#define LAB_UART UART_NUM_0
#define LAB_BAUD_RATE 115200
#define LAB_RX_BUFFER_SIZE 1024
#define LAB_TX_BUFFER_SIZE 1024

static bool initialized = false;
```

`initialized` is private to this source file. It tracks whether this abstraction has successfully installed the driver.

`uart_bus_init()` first rejects a second initialization attempt:

```c
if (initialized) return ESP_ERR_INVALID_STATE;
```

It then configures the UART:

```c
uart_config_t config = {
    .baud_rate = LAB_BAUD_RATE,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
};
```

The function calls `uart_param_config()` and then `uart_driver_install()` with 1024-byte RX and TX buffers. The flag changes to `true` **only if installation succeeds**. It leaves the board's existing UART0 pin routing unchanged rather than assigning new GPIOs.

**Design lesson:** Separate configuration and driver lifecycle from application behavior, and never mark a peripheral ready before initialization succeeds.

## 7. `uart_bus.c` — reading a complete command

UART transfers bytes, not messages. The lab defines a message as a sequence of characters ending with a newline (`\n`).

The API is:

```c
esp_err_t uart_bus_read_line(char *buffer, size_t capacity);
```

It first checks that the UART is initialized, the destination buffer is not `NULL`, and the capacity is at least two bytes (one character and a terminating `\0`). It then repeatedly reads **one byte at a time**:

```c
int received = uart_read_bytes(
    LAB_UART, &byte, 1, pdMS_TO_TICKS(100));
```

- A negative return becomes `ESP_FAIL`.
- Zero bytes means the 100 ms read timed out; the function continues waiting.
- Carriage return (`\r`) is ignored.
- Newline (`\n`) finishes the command; the function adds a null terminator.

### Oversized commands

The caller in `main.c` allocates a 128-byte command buffer. A maximum of **127 non-newline characters** can be stored, leaving one byte for `\0`.

If a line exceeds the capacity, `uart_bus_read_line()` sets an overflow flag and **continues consuming input until the newline**. It then returns `ESP_ERR_INVALID_SIZE`.

This matters because simply returning on the first excess byte would leave the rest of that command in the UART stream, where it could be mistaken for a new command.

**Design lesson:** Framing and overflow recovery belong in the transport abstraction; command meaning belongs in the protocol module.

## 8. `uart_bus.c` — writing a response

```c
esp_err_t uart_bus_write_line(const char *line);
```

The function checks the driver state and pointer, then calls `uart_write_bytes()` for the response text. It separately appends `\r\n` and waits for TX completion using:

```c
uart_wait_tx_done(LAB_UART, pdMS_TO_TICKS(1000));
```

The return value indicates whether the driver accepted/transmitted the bytes according to its API. It does not prove that a host application correctly parsed the response; that is checked separately at the host.

### Cleanup

`uart_bus_deinit()` rejects deinitialization when the driver is not initialized. Otherwise it calls `uart_driver_delete()` and clears the private flag only on success. The present `app_main()` runs continuously and does not normally call deinitialization.

## 9. `protocol.c` — interpreting commands

The protocol implementation uses `strcmp()` for exact matches and `snprintf()` to format bounded responses.

| Command | Response | Purpose |
|---|---|---|
| `PING` | `PONG` | Basic communication check |
| `GET_INFO` | `MODEL=ESP32;FW=1.0.0` | Device information |
| `GET_COUNTER` | `COUNTER=n` | Stateful behavior; increments each request |
| Unknown command | `ERROR=UNKNOWN_COMMAND` | Defined negative response |

For example:

```c
if (strcmp(command, "PING") == 0) {
    snprintf(response, response_size, "PONG");
} else if (strcmp(command, "GET_INFO") == 0) {
    snprintf(response, response_size, "MODEL=ESP32;FW=1.0.0");
} else if (strcmp(command, "GET_COUNTER") == 0) {
    snprintf(response, response_size, "COUNTER=%u", ++counter);
} else {
    snprintf(response, response_size, "ERROR=UNKNOWN_COMMAND");
}
```

The counter is a private static unsigned integer initialized to zero when the firmware starts. It is **not persistent across resets**. A `GET_COUNTER` response is useful for checking repeated command handling, but the precise value depends on earlier requests during the current run.

**Design lesson:** A command parser should not depend on GPIO pins, UART ports, or ESP-IDF driver internals.

## 10. `main.c` — application orchestration

`app_main()` is short because it delegates specialized work:

```c
#define COMMAND_CAPACITY 128
#define RESPONSE_CAPACITY 128

void app_main(void)
{
    esp_err_t err = uart_bus_init();
    if (err != ESP_OK) {
        printf("UART initialization failed: %s\n", esp_err_to_name(err));
        return;
    }

    char command[COMMAND_CAPACITY];
    char response[RESPONSE_CAPACITY];

    while (1) {
        err = uart_bus_read_line(command, sizeof(command));
        if (err == ESP_ERR_INVALID_SIZE) {
            uart_bus_write_line("ERROR=COMMAND_TOO_LONG");
            continue;
        }
        if (err != ESP_OK) {
            printf("UART read failed: %s\n", esp_err_to_name(err));
            continue;
        }

        printf("CMD: %s\n", command);
        fflush(stdout);
        protocol_process_command(command, response, sizeof(response));
        err = uart_bus_write_line(response);
        if (err != ESP_OK)
            printf("UART write failed: %s\n", esp_err_to_name(err));
    }
}
```

The `CMD:` debug line is part of the current host-test synchronization behavior, even though it is not a protocol response. Because console and command traffic share UART0, host code should distinguish debug output from actual responses.

Notice that the oversized-command branch sends an error and resumes the loop; it does not pass the invalid command to `protocol_process_command()`.

## 11. Example command exchange

A host sends newline-terminated commands; the firmware sends CRLF-terminated responses. The following shows the **logical protocol responses**, omitting possible boot/debug lines:

```text
Host → ESP32: PING\n
ESP32 → Host: PONG\r\n

Host → ESP32: GET_INFO\n
ESP32 → Host: MODEL=ESP32;FW=1.0.0\r\n

Host → ESP32: GET_COUNTER\n
ESP32 → Host: COUNTER=1\r\n

Host → ESP32: HELLO\n
ESP32 → Host: ERROR=UNKNOWN_COMMAND\r\n
```

For an oversized command, the expected response is:

```text
ERROR=COMMAND_TOO_LONG
```

`COUNTER=1` assumes this is the first `GET_COUNTER` request since boot.

## 12. Host-side Python client

The file `host/uart_client.py` provides a PC-side interface for exchanging commands with the board. Python communicates through a serial port such as `COM3` using PySerial. The port name is machine-dependent.

Typical responsibilities are:

1. Open the selected serial port with matching UART settings.
2. Send a newline-terminated command.
3. Read and interpret incoming serial lines.
4. Display the device response or report a timeout.
5. Close the port when finished.

Because the same UART0 connection can carry ESP-IDF logs and `CMD:` debug output, a robust client should recognize the intended response rather than assuming every line is a response.

## 13. Automated hardware-in-the-loop testing

The `tests/test_uart_protocol.py` module provides host-side tests for the command protocol. In this arrangement:

```text
pytest assertion
       │
       ▼
Python serial client
       │
       ▼
USB-to-serial interface
       │
       ▼
ESP32 firmware running on real hardware
       │
       ▼
Observed serial response
```

This is **hardware-in-the-loop (HIL)** because the test controls and observes an actual ESP32, rather than only calling a software simulation.

Useful checks include valid commands, unknown commands, counter progression, response formatting, and recovery after malformed or oversized input. The exact automated assertions should be read from the test file, rather than inferred from this list of possible checks.

A successful pytest run demonstrates the specific behaviors exercised by those tests on that device and firmware revision. It does not prove every possible UART failure mode or electrical timing characteristic.

## 14. Build, flash, and monitor

Activate an ESP-IDF-enabled PowerShell terminal, then navigate to the **lab project directory**:

```powershell
cd C:\Users\babar\esp32-hardware-interface-labs\uart\lab01_uart_loopback
idf.py build
```

Do not run `idf.py build` from the repository root or `uart/` parent directory; the ESP-IDF project `CMakeLists.txt` is inside the lab directory.

When the ESP32 is connected and its port is available:

```powershell
idf.py -p COM3 flash
idf.py -p COM3 monitor
```

Exit the ESP-IDF monitor with **Ctrl+]**. Close other serial monitors before running the Python host client or pytest; two programs normally cannot open the same Windows COM port simultaneously.

For a combined flash-and-monitor command:

```powershell
idf.py -p COM3 flash monitor
```

These are commands to run, **not claims that the current revision has been flashed or tested**.

## 15. What each validation level proves

| Level | Evidence | What it proves | What it does not prove |
|---|---|---|---|
| Build | `idf.py build` succeeds | Firmware compiles and links | The board executes it correctly |
| Flash/boot | Serial monitor shows application startup | Firmware runs on the target | All commands behave correctly |
| Manual protocol test | Python receives expected responses | Selected end-to-end commands work | All boundary cases work |
| Automated HIL test | pytest passes against ESP32 | Tested requirements behave as asserted | Complete coverage or signal integrity |
| Electrical inspection | Logic analyzer or oscilloscope | Observable timing/voltage properties | Higher-level correctness by itself |

Earlier UART hardware tests may have passed on a prior revision. After refactoring, rerun the host tests to confirm the modular firmware still produces the same observable behavior.

## 16. Troubleshooting

| Symptom | Likely cause | Check |
|---|---|---|
| `CMakeLists.txt not found` | Wrong working directory | Enter `uart/lab01_uart_loopback` |
| COM3 cannot be opened | Wrong port, disconnected board, or port in use | Device Manager; close serial monitor |
| No response | Wrong baud rate, missing newline, or firmware not running | Use 115200 8N1; append `\n`; inspect boot logs |
| Unexpected `CMD:` or boot text | UART0 shares console and command traffic | Filter debug/log lines in the host client |
| Command appears split | Line framing or timeout behavior | Verify newline termination |
| `ERROR=COMMAND_TOO_LONG` | More than 127 command characters | Shorten input or deliberately test overflow handling |
| Counter differs from expectation | Earlier requests or board reset | Track requests within the current boot session |
| pytest serial-port error | Monitor and pytest compete for COM3 | Close monitor before testing |

## 17. Design choices and trade-offs

**Why three firmware modules?** The application stays readable; UART configuration is isolated; command behavior can be understood independently of hardware setup.

**Why read one byte at a time?** It simplifies teaching line framing and overflow recovery. It is not necessarily the most efficient approach for high-throughput serial links.

**Why fixed-size buffers?** They make memory usage predictable and avoid dynamic allocation for short command messages. Their limits must be checked explicitly.

**Why `esp_err_t`?** It distinguishes successful operations from invalid state, invalid arguments, oversized input, and driver failures.

**Why keep UART0?** It uses the development board's existing USB-to-serial path, but console output can mix with protocol traffic. A separate UART would be an alternative for a more isolated production protocol.

**Why preserve the `CMD:` line?** Existing host-side tests synchronize on it. Removing or changing it without updating the host tests could break the validation workflow even if the command responses remain correct.

## 18. Suggested exercises

1. Trace `PING` from Python through UART0, `uart_bus_read_line()`, `protocol_process_command()`, and `uart_bus_write_line()`.
2. Explain why a newline is required and why `\r` is ignored.
3. Send an unknown command and identify which module generates the error.
4. Send a command longer than 127 characters and confirm the next valid command is still parsed correctly.
5. Request `GET_COUNTER` several times, then reset the board and observe the counter behavior.
6. Run the host tests after flashing the refactored firmware and record the actual results.
7. Explain the difference between a successful ESP-IDF build and a passing hardware-in-the-loop test.

## 19. Key takeaways

The UART lab demonstrates a complete, small embedded communication path: **physical serial transport → line framing → command processing → response transmission → host-side verification**. The modular design keeps hardware-specific code in `uart_bus.c`, command semantics in `protocol.c`, and application flow in `main.c`. It also shows why test evidence must be associated with the exact firmware revision and hardware setup being validated.
