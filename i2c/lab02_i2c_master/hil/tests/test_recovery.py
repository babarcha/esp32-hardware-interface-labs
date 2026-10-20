
from serial_device import SerialDevice


def test_reinit_connected_sensor(device):
    assert device.reinit_sensor() is True
    assert device.get_status() == "READY"


def test_measurement_after_reinit(device):
    assert device.reinit_sensor() is True

    measurement = device.read_measurement()

    assert -40.0 <= measurement.temperature_c <= 85.0
    assert 300.0 <= measurement.pressure_hpa <= 1100.0


def test_repeated_reinitialization(device):
    for _ in range(5):
        assert device.reinit_sensor() is True

    assert device.ping() is True
    assert device.get_status() == "READY"


def test_recovery_failure_response():
    class FakeSerial:
        is_open = True

        def reset_input_buffer(self):
            pass

        def write(self, data):
            return len(data)

        def flush(self):
            pass

        def readline(self):
            return b"HIL:ERROR=SENSOR_REINIT_FAILED\n"

    client = SerialDevice(port="FAKE")
    client.connection = FakeSerial()

    assert client.reinit_sensor() is False
