import time

import pytest

from serial_device import DeviceProtocolError


def test_long_command_rejected(device):
    # Firmware command buffer is 64 bytes.
    command = "X" * 100

    response = device.send_command(command)

    assert response == "ERROR=COMMAND_TOO_LONG"


@pytest.mark.requirement("REQ-HIL-008")
def test_command_after_overflow(device):
    # Check that the parser recovers after a long command.
    device.send_command("X" * 100)

    assert device.ping() is True


def test_many_ping_requests(device):
    for _ in range(100):
        assert device.ping() is True


def test_many_sensor_reads(device):
    for _ in range(20):
        measurement = device.read_measurement()

        assert -40.0 <= measurement.temperature_c <= 85.0
        assert 300.0 <= measurement.pressure_hpa <= 1100.0


def test_invalid_sensor_command(device):
    response = device.send_command("READ_UNKNOWN_SENSOR")

    assert response == "ERROR=UNKNOWN_COMMAND"


def test_reject_multiline_host_command(device):
    with pytest.raises(ValueError):
        device.send_command("PING\nREAD_SENSOR")


def test_sensor_info_after_stress(device):
    info = device.get_sensor_info()

    assert info["SENSOR"] == "BMP280"
    assert info["ID"] == "0x58"
