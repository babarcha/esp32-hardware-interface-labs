def test_sensor_identity(device):
    info = device.get_sensor_info()

    assert info["SENSOR"] == "BMP280"
    assert info["ID"] == "0x58"


def test_sensor_i2c_address(device):
    info = device.get_sensor_info()

    # The BMP280 can use 0x76 or 0x77.
    assert info["ADDR"] in ("0x76", "0x77")
