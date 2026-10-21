import os

import pytest

from serial_device import SerialDevice


def pytest_addoption(parser):
    parser.addoption(
        "--port",
        action="store",
        default=os.getenv("HIL_PORT", "COM3"),
        help="ESP32 serial port",
    )

    parser.addoption(
        "--baudrate",
        action="store",
        type=int,
        default=115200,
        help="UART baud rate",
    )


@pytest.fixture(scope="session")
def device(request):
    port = request.config.getoption("--port")
    baudrate = request.config.getoption("--baudrate")

    with SerialDevice(
        port=port,
        baudrate=baudrate,
        timeout=3.0,
    ) as connected_device:
        yield connected_device


def pytest_collection_modifyitems(items):
    """
    Mark tests that are not explicitly simulated
    as hardware-dependent tests.
    """
    for item in items:
        if item.get_closest_marker("simulation") is None:
            item.add_marker(pytest.mark.hardware)
