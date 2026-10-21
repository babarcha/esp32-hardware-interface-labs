import pytest


@pytest.mark.requirement("REQ-HIL-001")
def test_ping(device):
    assert device.ping() is True


@pytest.mark.requirement("REQ-HIL-002")
def test_sensor_ready(device):
    assert device.get_status() == "READY"
