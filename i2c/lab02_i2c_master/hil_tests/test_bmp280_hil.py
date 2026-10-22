
import math
import re
import time

import pytest


def test_ping(device):
    assert device("PING") == "HIL:PONG"


def test_sensor_status(device):
    assert device("GET_STATUS") == "HIL:STATUS=READY"


def test_sensor_identity(device):
    response = device("GET_SENSOR_INFO")

    assert "SENSOR=BMP280" in response
    assert "ID=0x58" in response
    assert "ADDR=0x76" in response


def parse_measurement(response):
    match = re.fullmatch(
        r"HIL:TEMP_C=(-?\d+(?:\.\d+)?);"
        r"PRESSURE_HPA=(-?\d+(?:\.\d+)?)",
        response,
    )

    assert match is not None, (
        f"Invalid measurement response: {response}"
    )

    temperature = float(match.group(1))
    pressure = float(match.group(2))

    assert math.isfinite(temperature)
    assert math.isfinite(pressure)

    return temperature, pressure


def test_sensor_measurement(device):
    temperature, pressure = parse_measurement(
        device("READ_SENSOR")
    )

    # Broad sanity limits, not calibration tolerances.
    assert -40 <= temperature <= 85
    assert 300 <= pressure <= 1100


@pytest.mark.parametrize("sample", range(10))
def test_repeated_measurements(device, sample):
    temperature, pressure = parse_measurement(
        device("READ_SENSOR")
    )

    assert -40 <= temperature <= 85
    assert 300 <= pressure <= 1100

    time.sleep(0.2)


def test_sensor_reinitialization(device):
    assert device("REINIT_SENSOR") == (
        "HIL:SENSOR_REINIT=OK"
    )

    assert device("GET_STATUS") == (
        "HIL:STATUS=READY"
    )

    parse_measurement(device("READ_SENSOR"))


def test_unknown_command(device):
    assert device("INVALID_COMMAND") == (
        "HIL:ERROR=UNKNOWN_COMMAND"
    )
