
import os
import time

import pytest
import serial


@pytest.fixture(scope="module")
def device():
    port = os.getenv("ESP32_PORT", "COM3")

    with serial.Serial(
        port=port,
        baudrate=115200,
        timeout=1,
        write_timeout=1,
    ) as ser:
        # Opening the port may reset the ESP32.
        time.sleep(2)
        ser.reset_input_buffer()

        def command(text):
            ser.reset_input_buffer()
            ser.write((text + "\n").encode("ascii"))
            ser.flush()

            deadline = time.monotonic() + 3

            while time.monotonic() < deadline:
                line = ser.readline().decode(
                    "utf-8", errors="replace"
                ).strip()

                if line.startswith("HIL:"):
                    return line

            raise TimeoutError(
                f"No HIL response for command: {text}"
            )

        yield command
