
import pytest

from device.serial_device import SerialDevice
from device.simulated_device import SimulatedDevice


def pytest_addoption(parser):
    parser.addoption(
        "--device",
        action="store",
        choices=["sim", "serial"],
        default="sim",
        help="Select simulated or physical ESP32 device",
    )

    parser.addoption(
        "--port",
        action="store",
        default="COM3",
        help="Serial port for physical ESP32",
    )


@pytest.fixture
def device(request):
    mode = request.config.getoption("--device")
    port = request.config.getoption("--port")

    if mode == "serial":
        selected = SerialDevice(port=port)
    else:
        selected = SimulatedDevice()

    with selected:
        yield selected
