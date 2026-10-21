from serial_device import (
    SerialDevice,
    DeviceTimeoutError,
    DeviceProtocolError,
)
import pytest

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


def test_serial_timeout():
    device = make_device([])

    with pytest.raises(DeviceTimeoutError):
        device.send_command("PING")


def test_empty_hil_response():
    device = make_device([b"HIL:\n"])

    with pytest.raises(DeviceProtocolError):
        device.send_command("PING")


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


def test_stale_uart_response_is_ignored():
    from serial_device import SerialDevice

    class FakeSerial:
        is_open = True

        def __init__(self):
            self.responses = [
                b"HIL:PONG\n",
                b"HIL:STATUS=READY\n",
            ]

        def reset_input_buffer(self):
            pass

        def write(self, data):
            return len(data)

        def flush(self):
            pass

        def readline(self):
            if self.responses:
                return self.responses.pop(0)
            return b""

    client = SerialDevice(port="FAKE", timeout=0.5)
    client.connection = FakeSerial()

    assert client.get_status() == "READY"


def test_measurement_ignores_stale_status():
    from serial_device import SerialDevice

    class FakeSerial:
        is_open = True

        def __init__(self):
            self.responses = [
                b"HIL:STATUS=READY\n",
                b"HIL:TEMP_C=22.50;PRESSURE_HPA=1013.25\n",
            ]

        def reset_input_buffer(self):
            pass

        def write(self, data):
            return len(data)

        def flush(self):
            pass

        def readline(self):
            if self.responses:
                return self.responses.pop(0)
            return b""

    client = SerialDevice(port="FAKE", timeout=0.5)
    client.connection = FakeSerial()

    measurement = client.read_measurement()

    assert measurement.temperature_c == 22.50
    assert measurement.pressure_hpa == 1013.25


def test_only_stale_responses_eventually_timeout():
    import pytest
    from serial_device import (
        SerialDevice,
        DeviceTimeoutError,
    )

    class FakeSerial:
        is_open = True

        def reset_input_buffer(self):
            pass

        def write(self, data):
            return len(data)

        def flush(self):
            pass

        def readline(self):
            return b"HIL:PONG\n"

    client = SerialDevice(port="FAKE", timeout=0.05)
    client.connection = FakeSerial()

    with pytest.raises(DeviceTimeoutError):
        client.get_status()
