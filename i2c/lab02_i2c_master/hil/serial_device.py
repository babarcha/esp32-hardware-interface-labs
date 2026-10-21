
"""
ESP32 BMP280 Hardware-in-the-Loop serial client.

Pass 9.7:
- UART command/response handling
- Response-type validation
- Stale response filtering
- Overall command timeout
- BMP280 measurement parsing
- Sensor recovery support
"""

from dataclasses import dataclass
import time

import serial


class DeviceError(Exception):
    """Base exception for device communication errors."""


class DeviceTimeoutError(DeviceError):
    """Raised when a command receives no matching response."""


class DeviceProtocolError(DeviceError):
    """Raised when the device returns an invalid protocol response."""


@dataclass
class SensorMeasurement:
    temperature_c: float
    pressure_hpa: float


class SerialDevice:
    """UART interface to the ESP32 BMP280 HIL firmware."""

    def __init__(
        self,
        port: str,
        baudrate: int = 115200,
        timeout: float = 1.0,
    ):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.connection = None

    def open(self):
        """Open the serial connection."""

        if self.connection is not None:
            return

        self.connection = serial.Serial(
            port=self.port,
            baudrate=self.baudrate,
            timeout=self.timeout,
            write_timeout=self.timeout,
        )

    def close(self):
        """Close the serial connection."""

        if self.connection is not None:
            self.connection.close()
            self.connection = None

    def __enter__(self):
        self.open()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()

    @staticmethod
    def _response_matches_command(
        command: str,
        response: str,
    ) -> bool:
        """Determine whether a response matches the command."""

        if response.startswith("ERROR="):
            return True

        expected_prefixes = {
            "PING": ("PONG",),
            "GET_STATUS": ("STATUS=",),
            "GET_SENSOR_INFO": ("SENSOR=",),
            "READ_SENSOR": ("TEMP_C=",),
            "REINIT_SENSOR": ("SENSOR_REINIT=",),
        }

        prefixes = expected_prefixes.get(command)

        if prefixes is None:
            # Unknown commands are handled by the firmware.
            return True

        return response.startswith(prefixes)

    def send_command(self, command: str) -> str:
        """Send a command and wait for its matching HIL response."""

        if self.connection is None:
            raise DeviceError("Serial connection is not open")

        if not isinstance(command, str):
            raise TypeError("Command must be a string"
                            )

        if "\n" in command or "\r" in command:
            raise ValueError(
                "Command must not contain newline characters"
            )

        deadline = time.monotonic() + self.timeout

        payload = (command + "\n").encode("ascii")

        try:
            self.connection.write(payload)
            self.connection.flush()
        except serial.SerialException as exc:
            raise DeviceError(
                f"Serial write failed: {exc}"
            ) from exc

        while time.monotonic() < deadline:
            try:
                raw = self.connection.readline()
            except serial.SerialException as exc:
                raise DeviceError(
                    f"Serial read failed: {exc}"
                ) from exc

            if not raw:
                continue

            line = raw.decode(
                "utf-8",
                errors="replace",
            ).strip()

            # Ignore ESP-IDF boot and diagnostic logs.
            if not line.startswith("HIL:"):
                continue

            response = line.removeprefix("HIL:")

            if not response:
                raise DeviceProtocolError(
                    "Received empty HIL response"
                )

            if not self._response_matches_command(
                command,
                response,
            ):
                # Ignore a response from a previous command.
                continue

            return response

        raise DeviceTimeoutError(
            f"Timeout waiting for response to {command!r}"
        )

    @staticmethod
    def _parse_fields(response: str) -> dict:
        """Parse semicolon-separated KEY=VALUE fields."""

        fields = {}

        for part in response.split(";"):
            if "=" not in part:
                continue

            key, value = part.split("=", 1)
            fields[key] = value

        return fields

    def ping(self) -> bool:
        """Check whether the firmware responds to PING."""

        response = self.send_command("PING")
        return response == "PONG"

    def get_status(self) -> str:
        """Return READY or NOT_READY."""

        response = self.send_command("GET_STATUS")

        if not response.startswith("STATUS="):
            raise DeviceProtocolError(response)

        return response.removeprefix("STATUS=")

    def get_sensor_info(self) -> dict:
        """Return the BMP280 identity and I2C address."""

        response = self.send_command("GET_SENSOR_INFO")

        if not response.startswith("SENSOR="):
            raise DeviceProtocolError(response)

        return self._parse_fields(response)

    def read_measurement(self) -> SensorMeasurement:
        """Read temperature and pressure from the BMP280."""

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
        """Request BMP280 reinitialization."""

        response = self.send_command("REINIT_SENSOR")

        if response == "SENSOR_REINIT=OK":
            return True

        if response.startswith("ERROR="):
            return False

        raise DeviceProtocolError(
            f"Invalid recovery response: {response}"
        )
