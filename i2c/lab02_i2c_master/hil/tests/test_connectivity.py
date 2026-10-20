def test_ping(device):
    assert device.ping() is True


def test_sensor_ready(device):
    assert device.get_status() == "READY"
