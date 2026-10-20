
import time
from dataclasses import dataclass

import serial


class DeviceError(Exception):
    """Base exception for HIL communication errors."""


class DeviceTimeoutError(DeviceError):
    """Raised when the device does not respond in time."""


class DeviceProtocolError(DeviceError):
    """Raised when the device returns an invalid response."""


@dataclass(frozen=True)
class SensorMeasurement:
    temperature_c: float
    pressure_hpa: float


class SerialDevice:
    def __init__(
        self,
        port: str = "COM3",
        baudrate: int = 115200,
        timeout: float = 2.0,
    ):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.connection = None

    def open(self):
        self.connection = serial.Serial(
            port=self.port,
            baudrate=self.baudrate,
            timeout=0.2,
            write_timeout=self.timeout,
        )

        # Opening the USB serial port may reset the ESP32.
        time.sleep(2.0)

        self.connection.reset_input_buffer()
        return self

    def close(self):
        if self.connection is not None:
            self.connection.close()
            self.connection = None

    def __enter__(self):
        return self.open()

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()

    def send_command(self, command: str) -> str:
        if self.connection is None or not self.connection.is_open:
            raise DeviceError("Serial port is not open")

        if "\r" in command or "\n" in command:
            raise ValueError("Command must not contain newlines")

        self.connection.reset_input_buffer()

        try:
            self.connection.write(
                (command + "\n").encode("ascii")
            )
            self.connection.flush()
        except (serial.SerialException, UnicodeEncodeError) as exc:
            raise DeviceError(
                f"Failed to send command: {command}"
            ) from exc

        deadline = time.monotonic() + self.timeout

        while time.monotonic() < deadline:
            try:
                raw = self.connection.readline()
            except serial.SerialException as exc:
                raise DeviceError("Serial read failed") from exc

            if not raw:
                continue

            line = raw.decode(
                "utf-8", errors="replace"
            ).strip()

            # Ignore ESP-IDF boot messages.
            if not line.startswith("HIL:"):
                continue

            response = line.removeprefix("HIL:")

            if not response:
                raise DeviceProtocolError(
                    "Received empty HIL response"
                )

            return response

        raise DeviceTimeoutError(
            f"No HIL response to '{command}' "
            f"within {self.timeout:.1f} seconds"
        )

    def ping(self) -> bool:
        return self.send_command("PING") == "PONG"

    def get_status(self) -> str:
        response = self.send_command("GET_STATUS")

        if not response.startswith("STATUS="):
            raise DeviceProtocolError(response)

        return response.removeprefix("STATUS=")

    def get_sensor_info(self) -> dict:
        response = self.send_command("GET_SENSOR_INFO")
        return self._parse_fields(response)

    def read_measurement(self) -> SensorMeasurement:
        response = self.send_command("READ_SENSOR")
        fields = self._parse_fields(response)

        try:
            return SensorMeasurement(
                temperature_c=float(fields["TEMP_C"]),
                pressure_hpa=float(fields["PRESSURE_HPA"]),
            )
        except (KeyError, ValueError) as exc:
            raise DeviceProtocolError(
                f"Invalid measurement: {response}"
            ) from exc

    def reinit_sensor(self) -> bool:
        response = self.send_command("REINIT_SENSOR")

        if response == "SENSOR_REINIT=OK":
            return True

        if response == "ERROR=SENSOR_REINIT_FAILED":
            return False

        raise DeviceProtocolError(
            f"Unexpected recovery response: {response}"
        )

    @staticmethod
    def _parse_fields(response: str) -> dict:
        if response.startswith("ERROR="):
            raise DeviceProtocolError(response)

        fields = {}

        for item in response.split(";"):
            if "=" not in item:
                raise DeviceProtocolError(
                    f"Invalid field: {item}"
                )

            key, value = item.split("=", 1)
            fields[key] = value

        return fields
