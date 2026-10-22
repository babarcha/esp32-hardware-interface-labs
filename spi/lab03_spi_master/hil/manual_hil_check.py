
import time
import serial

PORT = "COM3"
BAUDRATE = 115200

COMMANDS = [
    "PING",
    "GET_STATUS",
    "GET_SENSOR_INFO",
    "READ_SENSOR",
    "REINIT_SENSOR",
    "READ_SENSOR",
]

with serial.Serial(
    port=PORT,
    baudrate=BAUDRATE,
    timeout=2,
) as device:

    # Allow ESP32 to restart after port opening.
    time.sleep(2)

    # Discard startup messages.
    device.reset_input_buffer()

    for command in COMMANDS:
        print(f"\nTX: {command}")

        device.write(
            (command + "\n").encode("ascii")
        )
        device.flush()

        deadline = time.monotonic() + 3.0
        response = None

        while time.monotonic() < deadline:
            line = device.readline().decode(
                "utf-8", errors="replace"
            ).strip()

            if line.startswith("RSP:"):
                response = line
                break

        if response is None:
            raise RuntimeError(
                f"No protocol response for {command}"
            )

        print(f"RX: {response}")
