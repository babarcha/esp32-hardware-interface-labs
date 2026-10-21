
from device.base import BaseDevice


class SimulatedDevice(BaseDevice):
    def __init__(self):
        self.connected = False
        self.ready = False
        self.temperature = 24.05
        self.pressure = 1023.49

    def connect(self) -> None:
        self.connected = True
        self.ready = True

    def command(self, request: str) -> str:
        if not self.connected:
            raise RuntimeError("Device is not connected")

        if request == "PING":
            return "PONG"

        if request == "GET_STATUS":
            return (
                "STATUS=READY"
                if self.ready
                else "STATUS=NOT_READY"
            )

        if request == "GET_SENSOR_INFO":
            return (
                "SENSOR=BMP280;"
                "BUS=SPI;"
                "CHIP_ID=0x58"
            )

        if request == "READ_SENSOR":
            if not self.ready:
                return "ERROR=SENSOR_NOT_READY"

            return (
                f"TEMP_C={self.temperature:.2f};"
                f"PRESSURE_HPA={self.pressure:.2f}"
            )

        if request == "REINIT_SENSOR":
            self.ready = True
            return "REINIT=OK"

        if len(request) > 95:
            return "ERROR=COMMAND_TOO_LONG"

        return "ERROR=UNKNOWN_COMMAND"

    def close(self) -> None:
        self.connected = False
        self.ready = False
