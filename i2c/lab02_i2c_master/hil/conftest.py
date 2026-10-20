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
