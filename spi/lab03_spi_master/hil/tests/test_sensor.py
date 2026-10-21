
import math


def parse_measurement(response: str) -> dict:
    result = {}

    for item in response.split(";"):
        key, separator, value = item.partition("=")

        if not separator:
            raise ValueError(
                f"Invalid measurement field: {item!r}"
            )

        result[key] = value

    return result


def test_sensor_info(device):
    response = device.command("GET_SENSOR_INFO")

    assert "SENSOR=BMP280" in response
    assert "BUS=SPI" in response
    assert "CHIP_ID=0x58" in response


def test_temperature(device):
    response = device.command("READ_SENSOR")
    values = parse_measurement(response)

    temperature = float(values["TEMP_C"])

    assert math.isfinite(temperature)
    assert -40.0 <= temperature <= 85.0


def test_pressure(device):
    response = device.command("READ_SENSOR")
    values = parse_measurement(response)

    pressure = float(values["PRESSURE_HPA"])

    assert math.isfinite(pressure)
    assert 300.0 <= pressure <= 1100.0


def test_measurement_format(device):
    response = device.command("READ_SENSOR")
    values = parse_measurement(response)

    assert set(values) == {
        "TEMP_C",
        "PRESSURE_HPA",
    }

    float(values["TEMP_C"])
    float(values["PRESSURE_HPA"])
