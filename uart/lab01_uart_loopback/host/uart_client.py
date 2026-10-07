"""
UART Lab 01 host client.

Sends text commands to the ESP32 over the board's USB-to-serial
connection and displays the firmware's diagnostic and protocol responses.
"""

import serial
import time


PORT = "COM3"
BAUD_RATE = 115200

# Exercise normal commands, stateful behavior, and error handling.
commands = [
    "PING",
    "GET_INFO",
    "GET_COUNTER",
    "GET_COUNTER",
    "HELLO",
]


print(f"Opening {PORT} at {BAUD_RATE} baud...")

with serial.Serial(PORT, BAUD_RATE, timeout=1) as ser:

    # Opening the serial port may reset the ESP32; allow it to boot.
    time.sleep(2)

    # Remove ESP-IDF boot output before starting protocol communication.
    ser.reset_input_buffer()

    for command in commands:

        print(f"\nTX -> {command}")

        # Commands are newline-delimited to match the firmware parser.
        ser.write((command + "\n").encode("utf-8"))
        ser.flush()

        # Firmware returns a diagnostic command echo followed by
        # the actual protocol response, e.g.:
        #
        #   CMD: PING
        #   PONG
        line1 = ser.readline().decode(
            "utf-8",
            errors="replace"
        ).strip()

        line2 = ser.readline().decode(
            "utf-8",
            errors="replace"
        ).strip()

        print(f"RX <- {line1}")
        print(f"RX <- {line2}")
