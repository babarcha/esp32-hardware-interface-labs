
"""
SPI Lab 03 — Pass 10.4
Manual UART HIL smoke test.
"""

import time
import serial

PORT = "COM3"
BAUD = 115200

COMMANDS = [
    "PING",
    "GET_STATUS",
    "GET_SENSOR_INFO",
    "READ_SENSOR",
    "REINIT_SENSOR",
    "UNKNOWN",
]

with serial.Serial(
    PORT,
    BAUD,
    timeout=2,
    write_timeout=2,
) as device:

    # Opening COM3 may reset the ESP32.
    time.sleep(2)
    device.reset_input_buffer()

    for command in COMMANDS:
        device.write((command + "\n").encode("ascii"))
        device.flush()

        response = None
        deadline = time.monotonic() + 3

        while time.monotonic() < deadline:
            line = device.readline().decode(
                "utf-8", errors="replace"
            ).strip()

            if line.startswith("RSP:"):
                response = line[4:]
                break

        print(f"TX: {command}")
        print(f"RX: {response or 'TIMEOUT'}")
        print()
