# ESP32 Hardware Interface & Validation Labs

Hands-on embedded systems labs for developing and validating common hardware
interfaces on the ESP32 using C/C++, ESP-IDF, Python, and hardware-oriented
test workflows.

The repository progresses through four fundamental embedded communication
interfaces:

- UART
- I²C
- SPI
- CAN/TWAI

The focus is not only on making the interfaces work, but also on developing
reusable driver abstractions, validating behavior, handling error conditions,
and separating application logic from hardware-specific implementation.

## Repository Structure

```text
esp32-hardware-interface-labs/
│
├── uart/
│   └── lab01_uart_loopback/
│
├── i2c/
│   └── lab02_i2c_master/
│
├── spi/
│   └── lab03_spi_master/
│
└── can/
    └── lab04_twai/
```

## Labs

| Lab | Interface | Main Concepts | Status |
|---|---|---|---|
| 01 | UART | Serial protocol, command parsing, Python host testing | Hardware validated |
| 02 | I²C | Master bus, scanning, device abstraction, read/write API | Software validated; peripheral testing pending |
| 03 | SPI | Master bus, device registration, full-duplex transfers, abstraction | Software validated; peripheral testing pending |
| 04 | CAN/TWAI | Controller lifecycle, CAN frames, TX/RX API, validation | Controller validated; physical CAN bus testing pending |

---

## Lab 01 — UART

Implements UART communication between the ESP32 and a host-side Python
application.

Example command protocol:

```text
PING
→ PONG

GET_INFO
→ MODEL=ESP32;FW=1.0.0

GET_COUNTER
→ COUNTER=n
```

The lab includes automated host-side testing with `pytest`.

Key concepts:

- UART configuration
- command/response protocols
- newline framing
- parser robustness
- host/device communication
- PySerial
- automated hardware testing

---

## Lab 02 — I²C Master

Implements an ESP32 I²C master using the ESP-IDF modern I²C master API.

Architecture:

```text
Application
    |
    v
i2c_bus.c / i2c_bus.h
    |
    v
ESP-IDF I²C driver
    |
    v
ESP32 I²C controller
    |
    v
I²C peripheral
```

Implemented functionality includes:

- I²C controller initialization
- bus scanning
- device registration
- write operations
- read operations
- combined write/read transactions
- device removal
- bus deinitialization

Physical peripheral validation remains a future extension.

---

## Lab 03 — SPI Master

Implements an ESP32 SPI master with a reusable abstraction layer.

Default configuration:

```text
MOSI : GPIO23
MISO : GPIO19
SCLK : GPIO18
CS   : GPIO5
```

Implemented functionality includes:

- SPI bus initialization
- device registration
- configurable clock and SPI mode
- write transactions
- read transactions
- full-duplex transfers
- lifecycle management
- argument and state validation

A real SPI peripheral such as an MCP3008 ADC can be used for complete
hardware validation.

---

## Lab 04 — CAN/TWAI

Uses the ESP32's integrated TWAI controller for Classical CAN communication.

Configuration:

```text
TX      : GPIO21
RX      : GPIO22
Bitrate : 500 kbit/s
Mode    : Normal
```

Architecture:

```text
Application
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
    v
CAN transceiver
    |
    v
CANH / CANL
```

Implemented functionality includes:

- TWAI driver initialization
- controller start/stop
- standard and extended CAN frame validation
- transmit API
- receive API
- configurable timeouts
- lifecycle validation
- DLC validation
- CAN identifier validation

Example frame:

```text
Identifier : 0x123
Format     : Standard 11-bit
DLC        : 3
Data       : AA BB CC
```

The ESP32 TWAI controller has been exercised in firmware. Complete physical
CAN validation requires an external CAN transceiver and another CAN node.

---

## Validation Philosophy

The labs distinguish between three different levels of validation:

```text
Software/API validation
        |
        v
MCU peripheral validation
        |
        v
Physical bus/device validation
```

A successful driver call does not necessarily prove that the physical
interface is operating correctly.

For example:

```text
SPI transaction succeeds
        !=
external SPI peripheral responded correctly
```

and:

```text
TWAI controller starts
        !=
CAN frame was transmitted and acknowledged
```

This distinction is important when developing production-oriented embedded
validation systems.

## Development Environment

The labs were developed using:

- ESP32
- ESP-IDF 5.5.x
- C/C++
- Python
- pytest
- PySerial
- CMake
- Ninja
- Git
- GitHub
- Windows 11
- Visual Studio Code

## Building a Lab

Activate the ESP-IDF environment first.

Example:

```powershell
C:\Espressif\frameworks\esp-idf-v5.5.5\export.ps1
```

Navigate to a lab:

```powershell
cd can\lab04_twai
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

Exit the monitor with:

```text
Ctrl + ]
```

The serial port may differ between systems.

## Hardware Validation Roadmap

The next stage is to add real peripherals to the software-complete interfaces:

```text
I²C
 └── real I²C sensor/peripheral

SPI
 └── MCP3008 ADC
       ├── CH0 → GND      ≈ 0
       └── CH0 → VREF     ≈ full scale

CAN/TWAI
 └── CAN transceiver
       └── CANH/CANL
             └── second CAN node
```

These extensions will allow end-to-end validation from application software
through the ESP32 peripheral and electrical interface to the external device.

## Skills Demonstrated

This repository demonstrates practical experience with:

- embedded C/C++
- ESP32 and ESP-IDF
- UART, I²C, SPI, and CAN/TWAI
- hardware abstraction layers
- embedded communication protocols
- defensive API design
- state and lifecycle management
- error handling
- hardware/software integration
- Python-based hardware testing
- pytest
- serial communication
- CMake/Ninja build systems
- Git-based development

## Future Work

Planned extensions include:

- real I²C peripheral validation
- MCP3008 SPI ADC integration
- physical CAN/TWAI communication
- second-node CAN testing
- automated host-side validation
- additional negative and boundary tests
- CI for software-only tests
- separation of hardware-dependent and hardware-independent tests