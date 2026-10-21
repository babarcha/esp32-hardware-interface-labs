
import time

import serial

from device.base import BaseDevice


class SerialDevice(BaseDevice):
    def __init__(
        self,
        port: str = "COM3",
        baudrate: int = 115200,
        timeout: float = 3.0,
    ):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial_port = None

    def connect(self) -> None:
        self.serial_port = serial.Serial(
            port=self.port,
            baudrate=self.baudrate,
            timeout=0.2,
            write_timeout=self.timeout,
        )

        # Opening the USB serial port can reset ESP32.
        time.sleep(2)
        self.serial_port.reset_input_buffer()

    def command(self, request: str) -> str:
        if self.serial_port is None:
            raise RuntimeError("Device is not connected")

        if "\r" in request or "\n" in request:
            raise ValueError("Command must be a single line")

        self.serial_port.reset_input_buffer()

        self.serial_port.write(
            (request + "\n").encode("ascii")
        )
        self.serial_port.flush()

        deadline = time.monotonic() + self.timeout

        while time.monotonic() < deadline:
            line = self.serial_port.readline().decode(
                "utf-8",
                errors="replace",
            ).strip()

            if line.startswith("RSP:"):
                return line[4:]

        raise TimeoutError(
            f"No response to {request!r} "
            f"within {self.timeout} seconds"
        )

    def close(self) -> None:
        if self.serial_port is not None:
            self.serial_port.close()
            self.serial_port = None
