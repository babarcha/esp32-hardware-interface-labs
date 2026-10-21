import pytest


@pytest.mark.requirement("REQ-HIL-006")
def test_unknown_command(device):
    response = device.send_command("UNKNOWN")

    assert response == "ERROR=UNKNOWN_COMMAND"


def test_empty_command(device):
    response = device.send_command("")

    assert response == "ERROR=UNKNOWN_COMMAND"


def test_repeated_ping(device):
    for _ in range(10):
        assert device.ping() is True
