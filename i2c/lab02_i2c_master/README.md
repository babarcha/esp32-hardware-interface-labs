# ESP32 I2C Master Lab

This lab demonstrates an I2C master interface on ESP32 using the
modern ESP-IDF I2C master API.

## Objectives

- Configure an ESP32 as an I2C master
- Understand SDA and SCL
- Scan the 7-bit I2C address space
- Detect devices using ACK/NACK
- Separate application code from the I2C bus abstraction
- Register I2C device handles
- Implement generic transmit and receive operations
- Support write-read register transactions
- Remove devices and release bus resources

## Hardware

- ESP32
- SDA: GPIO21
- SCL: GPIO22
- UART/USB console: COM3

No external I2C peripheral is required for the basic scanner test.

## Software

- ESP-IDF 5.5.5
- Modern I2C master API:
  `driver/i2c_master.h`

## Architecture

Application:

    main.c

        |
        v

I2C abstraction:

    i2c_bus.h
    i2c_bus.c

        |
        v

ESP-IDF I2C master driver

        |
        v

ESP32 I2C controller

        |
        +---- SDA GPIO21
        |
        +---- SCL GPIO22

## Implemented API

    i2c_bus_init()
    i2c_bus_deinit()

    i2c_bus_probe()
    i2c_bus_scan()

    i2c_bus_add_device()
    i2c_bus_remove_device()

    i2c_bus_write()
    i2c_bus_read()
    i2c_bus_write_read()

## I2C Address Scanning

The scanner checks normal usable 7-bit addresses:

    0x08 - 0x77

The ranges:

    0x00 - 0x07
    0x78 - 0x7F

are excluded because they contain reserved addresses.

The scanner therefore checks:

    112 addresses

## Expected Result Without a Peripheral

    I2C controller initialized successfully
    SDA = GPIO21
    SCL = GPIO22

    Scanning I2C bus...

    Scan complete
    Addresses checked : 112
    Devices found     : 0

Finding zero devices is expected when no external I2C target is
connected.

## I2C Device Model

Multiple devices can share the same physical SDA/SCL bus:

              ESP32
                |
            I2C Master
                |
          SDA -------+
          SCL -------+
                |
        +-------+-------+
        |       |       |
      0x3C    0x68    0x76
      Device  Device  Device

Each target is identified by its I2C address.

## Register Transactions

Many I2C peripherals use register-oriented communication.

A typical register read is:

    START
      |
      +--> Device Address + WRITE
      |
      +--> Register Address
      |
      +--> Repeated START
      |
      +--> Device Address + READ
      |
      +<-- Register Data
      |
     STOP

The abstraction provides:

    i2c_bus_write_read()

for this type of transaction.

## Build

From the lab directory:

    idf.py build

## Flash

    idf.py -p COM3 flash

## Monitor

    idf.py -p COM3 monitor

Exit the monitor using:

    Ctrl + ]

## Current Limitation

No external I2C peripheral was available during development.

Therefore:

- bus initialization was hardware tested
- address scanning was hardware tested
- bus lifecycle was hardware tested
- zero-device behavior was verified
- device-specific read/write operations were implemented but not
  validated against a physical I2C target

A future extension should connect an I2C sensor, display, EEPROM, or
other peripheral and validate actual register transactions.