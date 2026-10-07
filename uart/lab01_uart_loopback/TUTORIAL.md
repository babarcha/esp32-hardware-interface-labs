# UART Lab 01 — ESP32 UART Command/Response and Hardware-in-the-Loop Testing

## 1. Purpose of the Lab

This lab introduces UART communication on the ESP32 and demonstrates how an embedded device can be controlled and validated from a host PC.

The project implements a simple command/response protocol between:

- an ESP32 running ESP-IDF firmware, and
- a Python application running on a PC.

The same communication interface is then used by `pytest` to perform automated hardware-in-the-loop (HIL) testing.

The complete path is:

```text
Host PC
   │
   │ USB
   ▼
USB-to-Serial Interface
   │
   │ UART0
   ▼
ESP32 Firmware
   │
   ▼
Command Parser
   │
   ▼
Command Handler
   │
   ▼
UART Response
```

For automated validation:

```text
pytest
  │
  ▼
PySerial
  │
  ▼
COM3
  │
  ▼
USB-to-Serial
  │
  ▼
ESP32 UART0
  │
  ▼
Firmware
  │
  ▼
Response
  │
  ▼
pytest assertion
```

This makes the lab more than a UART demonstration. It is a small example of a real embedded validation architecture.

---

# 2. Learning Objectives

After completing this lab, you should understand:

- what UART communication is;
- how UART differs from higher-level protocols;
- how ESP32 UART peripherals are configured with ESP-IDF;
- how bytes arrive from a serial connection;
- how a simple text protocol can be designed;
- how command framing works;
- how received bytes can be converted into commands;
- how firmware generates deterministic responses;
- how Python communicates with physical embedded hardware;
- how `pytest` can validate a real device;
- what hardware-in-the-loop testing means;
- why synchronization and timeouts matter in automated hardware testing.

---

# 3. Project Structure

The lab is organized into three main parts:

```text
lab01_uart_loopback/
│
├── CMakeLists.txt
│
├── main/
│   ├── CMakeLists.txt
│   └── main.c
│
├── host/
│   └── uart_client.py
│
├── tests/
│   └── test_uart_protocol.py
│
└── TUTORIAL.md
```

Each directory has a different responsibility.

### `main/`

Contains the ESP32 firmware.

The firmware:

1. configures UART0;
2. receives bytes from the PC;
3. assembles bytes into commands;
4. processes those commands;
5. sends responses back.

### `host/`

Contains a simple Python client.

It allows us to manually communicate with the ESP32 and inspect the protocol.

### `tests/`

Contains automated `pytest` tests.

These tests communicate with the physical ESP32 and verify its behavior.

This separation is important:

```text
Firmware              Host tool              Validation
   │                      │                       │
 main.c             uart_client.py        test_uart_protocol.py
```

The embedded firmware should not depend on the test implementation.

---

# 4. What Is UART?

UART stands for:

**Universal Asynchronous Receiver/Transmitter**

It is a hardware peripheral used for serial communication.

"Serial" means bits are transmitted sequentially rather than over many parallel wires.

A basic UART connection normally uses:

```text
Device A TX  ─────────► Device B RX
Device A RX  ◄───────── Device B TX
GND          ────────── GND
```

`TX` means transmit.

`RX` means receive.

Unlike I²C or SPI, UART does not use a shared clock line.

That is why UART is called **asynchronous**.

Both devices must therefore agree on communication parameters beforehand.

---

# 5. UART Configuration

This lab uses:

```text
Baud rate : 115200
Data bits : 8
Parity    : None
Stop bits : 1
Flow ctrl : None
```

This configuration is commonly abbreviated as:

```text
115200 8N1
```

where:

```text
8 = eight data bits
N = no parity
1 = one stop bit
```

Both the PC and ESP32 must use compatible settings.

The firmware defines:

```c
#define UART_PORT UART_NUM_0
#define UART_BAUD_RATE 115200
#define RX_BUFFER_SIZE 128
```

The Python host similarly uses:

```python
PORT = "COM3"
BAUD_RATE = 115200
```

The baud rates must match.

---

# 6. Why UART0 Is Used

The lab uses:

```c
UART_NUM_0
```

rather than UART2.

On the development board used for this lab, UART0 is connected to the board's USB-to-serial interface.

Therefore:

```text
Python
   │
   │ COM3
   ▼
USB
   │
   ▼
USB-to-Serial Interface
   │
   ▼
ESP32 UART0
```

No external USB-to-UART adapter is required.

This is also why ESP-IDF console output such as `printf()` can appear on the same serial connection.

---

# 7. Firmware Architecture

The firmware can be understood as four stages:

```text
UART initialization
        │
        ▼
Receive bytes
        │
        ▼
Build command
        │
        ▼
Process command
        │
        ▼
Send response
```

The main functions involved are:

```text
app_main()
    │
    ├── UART configuration
    │
    ├── uart_read_bytes()
    │
    ▼
process_command()
    │
    ▼
send_response()
```

---

# 8. UART Initialization

The UART configuration is stored in:

```c
const uart_config_t uart_config = {
    .baud_rate = UART_BAUD_RATE,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
};
```

This structure describes how the UART peripheral should operate.

The driver is installed with:

```c
uart_driver_install(
    UART_PORT,
    RX_BUFFER_SIZE * 2,
    0,
    0,
    NULL,
    0);
```

An RX buffer is allocated because incoming serial data may arrive independently of when the application reads it.

Conceptually:

```text
PC sends bytes
      │
      ▼
UART hardware
      │
      ▼
ESP-IDF RX ring buffer
      │
      ▼
uart_read_bytes()
      │
      ▼
application
```

The application therefore does not have to process every incoming electrical bit immediately.

---

# 9. Receiving UART Data

The firmware reads one byte at a time:

```c
uint8_t byte;

int length = uart_read_bytes(
    UART_PORT,
    &byte,
    1,
    pdMS_TO_TICKS(100));
```

The important arguments are:

```text
UART_PORT             which UART peripheral
&byte                 where received data is stored
1                     maximum bytes requested
100 ms                read timeout
```

Reading one byte at a time is not necessarily the highest-performance design, but it makes the framing logic very easy to understand in this introductory lab.

---

# 10. Why a Timeout Is Used

The call does not wait forever.

It uses:

```c
pdMS_TO_TICKS(100)
```

which converts 100 milliseconds into FreeRTOS scheduler ticks.

If no data arrives, the code receives no useful byte and executes:

```c
if (length <= 0)
{
    continue;
}
```

This returns execution to the beginning of the loop.

A timeout is particularly important in embedded systems because blocking indefinitely can prevent other work from being performed.

---

# 11. From Bytes to Commands

UART itself only transfers bytes.

It does not understand commands such as:

```text
PING
GET_INFO
GET_COUNTER
```

The application must define that protocol.

Suppose Python sends:

```text
PING\n
```

The ESP32 receives approximately:

```text
'P'
'I'
'N'
'G'
'\n'
```

The firmware stores each normal character:

```c
command[command_length++] = (char)byte;
```

After four characters:

```text
command
┌───┬───┬───┬───┐
│ P │ I │ N │ G │
└───┴───┴───┴───┘

command_length = 4
```

The newline tells the firmware that the command is complete.

---

# 12. Command Framing

The protocol uses newline characters as message delimiters.

The firmware checks:

```c
if (byte == '\n' || byte == '\r')
```

This means either:

```text
LF  = \n
CR  = \r
```

can terminate a command.

This is an example of **message framing**.

Without framing, a stream such as:

```text
PINGGET_INFOGET_COUNTER
```

would not tell the receiver where one command ends and another begins.

With newline framing:

```text
PING\n
GET_INFO\n
GET_COUNTER\n
```

the boundaries are explicit.

---

# 13. C String Termination

The received UART characters are initially just bytes in an array.

Before passing the command to C string functions, the firmware adds:

```c
command[command_length] = '\0';
```

For `PING`, memory then conceptually contains:

```text
P  I  N  G  \0
```

The null byte tells C string functions where the string ends.

Without this terminator, functions such as:

```c
strcmp()
```

could continue reading beyond the valid command.

---

# 14. Buffer Overflow Protection

The command buffer is:

```c
char command[RX_BUFFER_SIZE];
```

with:

```c
#define RX_BUFFER_SIZE 128
```

The firmware only stores another character when:

```c
command_length < RX_BUFFER_SIZE - 1
```

The final byte is reserved for:

```c
'\0'
```

Conceptually:

```text
0                                      126 127
┌──────────────────────────────────────────┬───┐
│              command data                │\0 │
└──────────────────────────────────────────┴───┘
```

This is a basic but important defensive programming technique in C.

---

# 15. Command Processing

Once a complete command is available, the firmware calls:

```c
process_command(command);
```

The handler compares the received string with known commands.

For example:

```c
if (strcmp(command, "PING") == 0)
{
    send_response("PONG");
}
```

`strcmp()` returns zero when two strings are equal.

The lab supports:

```text
Command        Response
-------------  ----------------------
PING           PONG
GET_INFO       MODEL=ESP32;FW=1.0.0
GET_COUNTER    COUNTER=n
anything else  ERROR=UNKNOWN_COMMAND
```

This forms a very small application-layer protocol.

UART transports the bytes.

The application defines what those bytes mean.

---

# 16. PING — Connectivity Test

The simplest command is:

```text
PING
```

and the expected response is:

```text
PONG
```

This is useful because it validates the complete communication path:

```text
PC
 │
 ▼
serial port
 │
 ▼
USB interface
 │
 ▼
ESP32 UART
 │
 ▼
command parser
 │
 ▼
response
 │
 ▼
PC
```

A simple connectivity command is common in embedded test interfaces.

---

# 17. GET_INFO — Device Identification

The command:

```text
GET_INFO
```

returns:

```text
MODEL=ESP32;FW=1.0.0
```

This demonstrates how a validation system can query information about the DUT.

DUT means:

**Device Under Test**

In a production system, similar information might include:

```text
hardware revision
firmware version
serial number
bootloader version
manufacturing ID
```

This information can be important when deciding which tests should run against a particular product revision.

---

# 18. GET_COUNTER — Stateful Behavior

The firmware contains:

```c
static int counter = 0;
```

The command:

```text
GET_COUNTER
```

returns the current value and increments it:

```c
"COUNTER=%d", counter++
```

For example:

```text
COUNTER=0
COUNTER=1
COUNTER=2
COUNTER=3
```

This command demonstrates **stateful behavior**.

Unlike `PING`, the response depends on previous interactions with the device.

This becomes important when designing tests.

A good test should not assume that the first value must always be zero.

Instead, it verifies the actual requirement:

```text
second value = first value + 1
```

---

# 19. Unknown Command Handling

If no known command matches, the firmware sends:

```text
ERROR=UNKNOWN_COMMAND
```

This is preferable to silently ignoring invalid input.

A deterministic error response helps:

- debugging;
- automated testing;
- protocol diagnosis;
- fault handling.

A well-designed embedded protocol should define both successful and unsuccessful behavior.

---

# 20. Sending Responses

Responses are transmitted by:

```c
static void send_response(const char *response)
```

The function first sends the response text:

```c
uart_write_bytes(
    UART_PORT,
    response,
    strlen(response));
```

and then:

```c
uart_write_bytes(
    UART_PORT,
    "\r\n",
    2);
```

Therefore every response becomes a complete line.

For example:

```text
PONG\r\n
```

The host can then use line-oriented reads.

---

# 21. Diagnostic Output

Before processing a command, the firmware prints:

```c
printf("CMD: %s\n", command);
```

Therefore a transaction looks like:

```text
CMD: PING
PONG
```

The first line is diagnostic output.

The second line is the actual protocol response.

This distinction is important for the host-side test implementation.

---

# 22. Python Host Client

The manual host application is:

```text
host/uart_client.py
```

It uses PySerial:

```python
import serial
```

and opens:

```python
serial.Serial(PORT, BAUD_RATE, timeout=1)
```

For this setup:

```text
PORT      = COM3
BAUD_RATE = 115200
```

The host and firmware must agree on the serial settings.

---

# 23. Why the Host Waits Two Seconds

After opening COM3:

```python
time.sleep(2)
```

is executed.

Opening the serial connection may reset the ESP32 depending on the development board and USB-to-serial circuitry.

The firmware then needs time to boot.

The sequence can therefore be:

```text
Python opens COM3
       │
       ▼
ESP32 resets
       │
       ▼
bootloader runs
       │
       ▼
ESP-IDF starts
       │
       ▼
app_main()
```

Sending commands immediately could cause them to arrive before the firmware is ready.

---

# 24. Clearing Boot Messages

ESP-IDF generates boot output during startup.

Before starting protocol communication, the host executes:

```python
ser.reset_input_buffer()
```

This discards old serial data.

Without it, the first call to:

```python
ser.readline()
```

might return a boot message rather than the response to `PING`.

This illustrates an important validation principle:

**Synchronize the test environment before making assertions.**

---

# 25. Sending Commands from Python

The host sends:

```python
ser.write((command + "\n").encode("utf-8"))
```

There are three steps here.

First:

```python
command + "\n"
```

adds the protocol delimiter.

For `PING`:

```text
PING
```

becomes:

```text
PING\n
```

Second:

```python
.encode("utf-8")
```

converts the Python string into bytes.

Serial interfaces transmit bytes, not Python string objects.

Finally:

```python
ser.write(...)
```

passes those bytes to the serial port.

---

# 26. Reading Responses

The firmware currently produces:

```text
CMD: PING
PONG
```

The manual host client therefore reads two lines:

```python
line1 = ser.readline()
line2 = ser.readline()
```

The bytes are decoded:

```python
.decode("utf-8")
```

and whitespace/newline characters are removed with:

```python
.strip()
```

The resulting strings can then be displayed normally.

---

# 27. From Manual Testing to Automated Testing

The host client proves that communication works manually.

But repeatedly checking output by eye is not scalable.

Instead of asking:

```text
Did the ESP32 print PONG?
```

we want software to check:

```python
assert response == "PONG"
```

This is the transition from manual testing to automated validation.

---

# 28. Hardware-in-the-Loop Testing

The tests in:

```text
tests/test_uart_protocol.py
```

are HIL tests because they communicate with a real physical ESP32.

The test path is:

```text
pytest
  │
  ▼
PySerial
  │
  ▼
Windows COM3
  │
  ▼
USB-to-Serial
  │
  ▼
Physical ESP32
  │
  ▼
Firmware
```

This is different from a unit test.

A unit test might test a function entirely inside the PC:

```text
pytest → Python function
```

Our HIL test includes actual hardware:

```text
pytest → serial interface → physical DUT
```

Therefore it can detect problems that pure software tests cannot, including communication and device-integration failures.

---

# 29. pytest Fixture

The serial connection is created with:

```python
@pytest.fixture(scope="module")
def uart():
```

The fixture:

1. opens COM3;
2. waits for the ESP32;
3. clears old input;
4. gives the serial connection to the tests;
5. closes it when testing is finished.

Conceptually:

```text
pytest starts
     │
     ▼
open COM3
     │
     ▼
run test_ping
     │
     ▼
run test_get_info
     │
     ▼
run test_counter
     │
     ▼
run test_unknown_command
     │
     ▼
close COM3
```

The fixture has:

```python
scope="module"
```

so the serial port is opened once for the complete test module rather than once for every test.

This avoids repeatedly resetting the ESP32.

---

# 30. The `yield` Fixture Pattern

The fixture contains:

```python
yield ser
```

Everything before `yield` is setup.

Everything after `yield` is cleanup.

Conceptually:

```text
SETUP
  open serial port
  wait
  clear buffer

        │
        ▼

      yield ser

        │
        ▼

TESTS EXECUTE

        │
        ▼

CLEANUP
  close serial port
```

This pattern is widely used for hardware test resources.

---

# 31. `send_command()` as a Test Abstraction

Rather than duplicating serial communication logic in every test, the project defines:

```python
send_command(ser, command)
```

Individual tests can therefore focus on behavior:

```python
debug, response = send_command(uart, "PING")

assert debug == "CMD: PING"
assert response == "PONG"
```

instead of repeatedly implementing:

```text
clear buffer
encode command
write UART
read lines
decode bytes
handle timeout
```

This is an important validation-software design principle:

**separate transport mechanics from test intent.**

---

# 32. Synchronizing with the DUT

Serial communication may contain unrelated output.

The test therefore does not blindly assume that the first line received is the response.

Instead, it waits for:

```text
CMD: <command>
```

For:

```text
PING
```

it expects:

```text
CMD: PING
```

Only after seeing that line does it accept the next meaningful line as the response.

Conceptually:

```text
serial stream
     │
     ├── possible unrelated output
     ├── possible unrelated output
     │
     ├── CMD: PING        ← synchronization point
     │
     └── PONG             ← response
```

This makes the HIL test more robust.

---

# 33. Why Tests Need a Deadline

The test helper defines:

```python
deadline = time.time() + 2.0
```

and loops only until that deadline.

Without a deadline, a hardware failure could cause a test to wait forever.

For example:

```text
ESP32 disconnected
        │
        ▼
no response
        │
        ▼
test waits forever
```

With a deadline:

```text
ESP32 disconnected
        │
        ▼
no response
        │
        ▼
2-second deadline reached
        │
        ▼
test returns
        │
        ▼
assertion fails
```

A test failure is much better than a test system that hangs indefinitely.

---

# 34. Test 1 — Connectivity

The first test is:

```python
def test_ping(uart):
```

It sends:

```text
PING
```

and verifies:

```text
CMD: PING
PONG
```

This validates basic host-to-DUT communication and command handling.

---

# 35. Test 2 — Device Information

The second test sends:

```text
GET_INFO
```

and expects:

```text
MODEL=ESP32;FW=1.0.0
```

This verifies that the firmware exposes the expected identity/version information.

---

# 36. Test 3 — Stateful Counter

The test sends `GET_COUNTER` twice.

It does **not** assert:

```text
first == 0
second == 1
```

because previous manual commands may already have changed the device state.

Instead it verifies:

```python
assert second_value == first_value + 1
```

This is a stronger test because it checks the actual behavioral requirement rather than assuming a particular initial state.

---

# 37. Test 4 — Invalid Input

The test sends:

```text
HELLO
```

which is deliberately unsupported.

The expected response is:

```text
ERROR=UNKNOWN_COMMAND
```

This is a negative test.

Testing invalid input is important because validation is not only about proving that valid operations work.

It also checks that invalid operations fail predictably.

---

# 38. Running the Firmware

Open an ESP-IDF terminal and enter the lab directory:

```powershell
cd C:\Users\babar\esp32-hardware-interface-labs\uart\lab01_uart_loopback
```

Build:

```powershell
idf.py build
```

Flash:

```powershell
idf.py -p COM3 flash
```

Monitor:

```powershell
idf.py -p COM3 monitor
```

The firmware should report:

```text
UART Lab 01 ready
```

Exit the monitor with:

```text
Ctrl + ]
```

The monitor must be closed before the Python programs open COM3.

---

# 39. Python Environment

The host tools use the repository's Python virtual environment.

From the repository root:

```powershell
.\.venv\Scripts\Activate.ps1
```

The prompt should show:

```text
(.venv)
```

The required packages include:

```text
pytest
pyserial
```

They can be verified with:

```powershell
python -m pytest --version
python -c "import serial; print(serial.__version__)"
```

---

# 40. Manual Validation

Run:

```powershell
python host\uart_client.py
```

A successful session looks similar to:

```text
Opening COM3 at 115200 baud...

TX -> PING
RX <- CMD: PING
RX <- PONG

TX -> GET_INFO
RX <- CMD: GET_INFO
RX <- MODEL=ESP32;FW=1.0.0

TX -> GET_COUNTER
RX <- CMD: GET_COUNTER
RX <- COUNTER=2

TX -> GET_COUNTER
RX <- CMD: GET_COUNTER
RX <- COUNTER=3

TX -> HELLO
RX <- CMD: HELLO
RX <- ERROR=UNKNOWN_COMMAND
```

The actual counter starting value may differ.

The important property is that consecutive values increment by one.

---

# 41. Automated HIL Validation

Run:

```powershell
python -m pytest tests\test_uart_protocol.py -v
```

The validated result for this lab is:

```text
test_ping              PASSED
test_get_info          PASSED
test_counter           PASSED
test_unknown_command   PASSED

4 passed
```

This demonstrates that the complete host-to-hardware validation path works.

---

# 42. Important Debugging Lessons

Several practical embedded-development lessons emerge from this lab.

## Correct UART Peripheral

The host communicates through the development board's USB serial connection.

Therefore this implementation uses:

```c
UART_NUM_0
```

Using a different UART peripheral such as UART2 would require wiring its TX/RX pins to an appropriate external interface.

## Serial Port Ownership

Only one application can normally own COM3 at a time.

Therefore:

```text
ESP-IDF monitor
```

must be closed before:

```text
uart_client.py
```

or:

```text
pytest
```

tries to open the port.

## Firmware and Host Must Agree

Both sides must agree on:

```text
baud rate
message framing
command names
response format
```

A communication interface is effectively a contract between the host and DUT.

## Boot Output Can Interfere with Tests

ESP32 boot messages share the serial connection.

The host therefore waits for boot completion and clears stale input before testing.

## Hardware Tests Need Timeouts

Physical hardware can disconnect, reset, stall, or fail.

Automated HIL tests must therefore avoid infinite waits.

---

# 43. Design Choices

This lab deliberately uses a simple design.

### Text Protocol

Commands such as:

```text
PING
GET_INFO
GET_COUNTER
```

are human-readable.

This makes debugging easy with a terminal or Python script.

A production protocol might instead use binary packets for efficiency.

### Newline Framing

Newline framing is simple and easy to inspect.

More advanced protocols might use:

```text
packet length
start/end markers
checksums
CRC
sequence numbers
```

### Byte-at-a-Time Parsing

The firmware reads one byte at a time.

This emphasizes how a serial stream becomes an application message.

A higher-performance implementation might process blocks of received data.

### Diagnostic Echo

The:

```text
CMD: ...
```

line provides a convenient synchronization point for the HIL test.

In a larger production system, diagnostic logging and machine-readable protocol traffic might be separated more strictly.

---

# 44. What This Lab Demonstrates Professionally

Although the project is small, it demonstrates several concepts relevant to embedded validation engineering:

```text
Embedded firmware
      +
UART peripheral configuration
      +
serial protocol design
      +
host-side automation
      +
Python tooling
      +
pytest
      +
physical DUT
      =
small HIL validation system
```

The key point is that the test software is not merely testing Python code.

It is controlling and verifying behavior implemented on a physical embedded target.

---

# 45. Relationship to the Other Interface Labs

UART Lab 01 establishes the host/DUT validation pattern.

The later labs extend hardware-interface knowledge:

```text
Lab 01 — UART
    │
    ├── serial communication
    ├── command protocol
    └── host/HIL validation
          │
          ▼
Lab 02 — I²C
    │
    ├── master bus
    ├── device addresses
    ├── ACK/probing
    └── sensor communication
          │
          ▼
Lab 03 — SPI
    │
    ├── clocked full-duplex bus
    ├── chip select
    └── MCP3008 ADC
          │
          ▼
Lab 04 — CAN/TWAI
    │
    ├── message-oriented bus
    ├── arbitration
    └── external transceiver
```

Together, these labs build experience with common embedded hardware interfaces rather than treating each peripheral as an isolated API exercise.

---

# 46. Key Takeaways

The most important concepts from UART Lab 01 are:

1. UART transfers a serial stream of bytes.
2. UART0 provides the PC communication path used in this lab.
3. The application protocol gives those bytes meaning.
4. Newline characters provide message framing.
5. C strings require explicit null termination.
6. Buffer boundaries must be protected.
7. Host and firmware must agree on protocol behavior.
8. Serial tests need synchronization.
9. Hardware tests need bounded timeouts.
10. pytest can validate a physical embedded DUT through PySerial.
11. Stateful behavior should be tested through invariants rather than fragile assumptions about initial state.
12. Separating firmware, host tooling, and tests makes the project easier to maintain and extend.

The resulting architecture is:

```text
                    UART LAB 01

              ┌───────────────────┐
              │      pytest       │
              │  HIL assertions   │
              └─────────┬─────────┘
                        │
              ┌─────────▼─────────┐
              │     PySerial      │
              │ host communication│
              └─────────┬─────────┘
                        │
                      COM3
                        │
              ┌─────────▼─────────┐
              │   USB-to-Serial   │
              └─────────┬─────────┘
                        │
                      UART0
                        │
              ┌─────────▼─────────┐
              │       ESP32       │
              │     Firmware      │
              ├───────────────────┤
              │ UART RX           │
              │ Command framing   │
              │ Command parser    │
              │ Response handling │
              └───────────────────┘
```

This is the foundation for progressively more sophisticated embedded hardware and validation work in the remaining labs.