import time
import serial

COMMANDS = [
    "PING",
    "GET_STATUS",
    "GET_SENSOR_INFO",
    "READ_SENSOR",
    "UNKNOWN",
]

with serial.Serial("COM3", 115200, timeout=2) as device:
    # Allow the ESP32 to reboot after the port opens.
    time.sleep(2)
    device.reset_input_buffer()

    for command in COMMANDS:
        print(f"HOST -> {command}")

        device.write((command + "\n").encode("ascii"))
        device.flush()

        deadline = time.monotonic() + 3
        response = None

        while time.monotonic() < deadline:
            line = device.readline().decode(
                "utf-8", errors="replace"
            ).strip()

            if line.startswith("HIL:"):
                response = line
                break

        print(f"ESP32 <- {response or 'TIMEOUT'}")
