from serial_device import (
    SerialDevice,
    DeviceTimeoutError,
    DeviceProtocolError,
)
import pytest

pytestmark = pytest.mark.simulation


class FakeSerial:
    """Minimal fake serial connection for unit tests."""

    def __init__(self, responses):
        self.responses = list(responses)
        self.is_open = True
        self.written = []

    def reset_input_buffer(self):
        pass

    def write(self, data):
        self.written.append(data)
        return len(data)

    def flush(self):
        pass

    def readline(self):
        if self.responses:
            return self.responses.pop(0)

        return b""


def make_device(responses, timeout=0.05):
    device = SerialDevice(
        port="FAKE",
        baudrate=115200,
        timeout=timeout,
    )

    device.connection = FakeSerial(responses)
    return device


def test_serial_ping_response():
    device = make_device([b"HIL:PONG\n"])

    assert device.ping() is True
    assert device.connection.written == [b"PING\n"]


def test_ignore_non_protocol_logs():
    device = make_device([
        b"I (100) boot: ESP-IDF\n",
        b"HIL:PONG\n",
    ])

    assert device.ping() is True


@pytest.mark.requirement("REQ-HIL-009")
def test_serial_timeout():
    device = make_device([])

    with pytest.raises(DeviceTimeoutError):
        device.send_command("PING")


def test_empty_hil_response():
    device = make_device([b"HIL:\n"])

    with pytest.raises(DeviceProtocolError):
        device.send_command("PING")


@pytest.mark.requirement("REQ-HIL-010")
def test_invalid_measurement_response():
    device = make_device([
        b"HIL:TEMP_C=INVALID;PRESSURE_HPA=1032.0\n"
    ])

    with pytest.raises(DeviceProtocolError):
        device.read_measurement()


def test_sensor_error_response():
    device = make_device([
        b"HIL:ERROR=SENSOR_IO\n"
    ])

    with pytest.raises(DeviceProtocolError):
        device.read_measurement()


def test_parse_sensor_info():
    device = make_device([
        b"HIL:SENSOR=BMP280;ID=0x58;ADDR=0x76\n"
    ])

    info = device.get_sensor_info()

    assert info["SENSOR"] == "BMP280"
    assert info["ID"] == "0x58"
    assert info["ADDR"] == "0x76"
