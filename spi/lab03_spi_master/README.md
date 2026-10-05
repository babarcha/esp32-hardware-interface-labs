# ESP32 SPI Master Interface Lab

A reusable SPI master interface implemented on ESP32 using ESP-IDF.

This lab demonstrates SPI bus initialization, device registration, full-duplex
transactions, transmit/receive APIs, resource lifecycle management, and basic
error handling.

## Objectives

- Configure ESP32 as an SPI master
- Understand MOSI, MISO, SCLK, and CS
- Understand SPI modes 0-3
- Register SPI devices using ESP-IDF device handles
- Perform synchronous SPI transactions
- Implement write, read, and full-duplex transfer APIs
- Separate application logic from hardware-interface logic
- Manage SPI device and bus lifecycle safely

## Hardware Configuration

| Signal | ESP32 GPIO | Direction |
|---|---:|---|
| MOSI | GPIO23 | Master → Peripheral |
| MISO | GPIO19 | Peripheral → Master |
| SCLK | GPIO18 | Master → Peripheral |
| CS | GPIO5 | Master → Peripheral |

SPI host:

    SPI3_HOST

Example device configuration:

    Clock: 1 MHz
    Mode: 0
    CS: GPIO5

## Architecture

    Application
       main.c
          |
          v
       spi_bus.h
          |
          v
       spi_bus.c
          |
          v
    ESP-IDF SPI Master Driver
          |
          v
       SPI3_HOST
          |
       MOSI / MISO / SCLK / CS

`main.c` contains application-level behavior.

`spi_bus.c` contains ESP32 and ESP-IDF-specific SPI implementation details.

`spi_bus.h` exposes the reusable interface.

## SPI Device Selection

Unlike I2C, SPI does not normally use bus addresses.

I2C:

    Controller
        |
        +---- address 0x68
        +---- address 0x76

SPI:

    Master
        |
        +---- CS1 ---- Device 1
        |
        +---- CS2 ---- Device 2

MOSI, MISO, and SCLK can be shared between devices, while each device normally
has its own chip-select signal.

## SPI Modes

SPI communication is controlled by clock polarity (CPOL) and clock phase
(CPHA).

| Mode | CPOL | CPHA | Clock Idle |
|---|---:|---:|---|
| 0 | 0 | 0 | Low |
| 1 | 0 | 1 | Low |
| 2 | 1 | 0 | High |
| 3 | 1 | 1 | High |

The correct mode depends on the target peripheral.

This lab currently uses Mode 0.

## Public API

### Bus lifecycle

    spi_bus_init()
    spi_bus_deinit()

### Device lifecycle

    spi_device_register()
    spi_device_unregister()

### Transactions

    spi_bus_write()
    spi_bus_read()
    spi_bus_transfer()

`spi_bus_transfer()` performs a full-duplex SPI transaction.

## Example Transaction

The demonstration application transmits:

    0x9A 0xBC 0xDE

using:

    spi_bus_transfer()

ESP-IDF represents transaction lengths in bits, so a three-byte transaction is:

    3 bytes × 8 = 24 bits

## Resource Lifecycle

The normal lifecycle is:

    spi_bus_init()
          |
          v
    spi_device_register()
          |
          v
    SPI transaction(s)
          |
          v
    spi_device_unregister()
          |
          v
    spi_bus_deinit()

The abstraction performs argument and state validation to reduce invalid API
usage.

## Build

Activate ESP-IDF and build from this directory:

    idf.py build

Flash:

    idf.py -p COM3 flash

Monitor:

    idf.py -p COM3 monitor

Exit the monitor with:

    Ctrl + ]

## Validation

The following ESP32-side functionality has been exercised:

- SPI bus initialization
- SPI device registration
- Synchronous SPI transaction execution
- Device removal
- SPI bus release
- Abstraction-layer lifecycle

Example runtime output:

    SPI Lab 03 - Pass 5: Bus Abstraction
    ------------------------------------
    SPI bus initialized successfully
    SPI device registered successfully
    SPI transaction completed successfully
    TX: 0x9A 0xBC 0xDE
    RX: 0x9A 0xBC 0xDE
    NOTE: RX data is not meaningful without a connected SPI peripheral.
    SPI device removed successfully
    SPI bus released successfully

## Hardware Validation Limitation

No external SPI peripheral is currently connected.

Therefore, successful completion of `spi_device_transmit()` demonstrates that
the ESP32 SPI driver executed the configured transaction, but it does not prove
that a real peripheral received or understood the data.

In particular, the observed RX bytes must not be interpreted as valid
peripheral data.

A future hardware-validation pass can use:

- A real SPI sensor or memory device
- An oscilloscope
- A logic analyzer

to verify CS, SCLK, MOSI, MISO, SPI mode, timing, and device-specific protocol
behavior.

## ESP-IDF

Developed using ESP-IDF 5.5.5.