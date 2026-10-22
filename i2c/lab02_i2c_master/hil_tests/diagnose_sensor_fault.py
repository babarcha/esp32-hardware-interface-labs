
import os
import time
import serial

port = os.getenv("ESP32_PORT", "COM3")

commands = [
    "PING",
    "GET_STATUS",
    "GET_SENSOR_INFO",
    "READ_SENSOR",
    "REINIT_SENSOR",
    "GET_STATUS",
    "READ_SENSOR",
]

with serial.Serial(port, 115200, timeout=1) as ser:
    time.sleep(2)
    ser.reset_input_buffer()

    for command in commands:
        ser.reset_input_buffer()
        ser.write((command + "\n").encode("ascii"))
        ser.flush()

        deadline = time.monotonic() + 4
        response = None

        while time.monotonic() < deadline:
            line = ser.readline().decode(
                "utf-8", errors="replace"
            ).strip()

            if line.startswith("HIL:"):
                response = line
                break

        print(f"{command:20} -> {response or 'TIMEOUT'}")
