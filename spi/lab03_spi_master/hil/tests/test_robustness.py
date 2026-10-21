
def test_unknown_command(device):
    response = device.command("INVALID_COMMAND")

    assert response == "ERROR=UNKNOWN_COMMAND"


def test_empty_command_does_not_break_device(device):
    # Firmware intentionally ignores empty commands.
    # The serial client therefore times out waiting for
    # a response, while the simulator returns an error.

    try:
        device.command("")
    except TimeoutError:
        pass

    # The device must remain responsive afterward.
    assert device.command("PING") == "PONG"


def test_reinitialize_sensor(device):
    response = device.command("REINIT_SENSOR")

    assert response == "REINIT=OK"
    assert device.command("GET_STATUS") == "STATUS=READY"


def test_measurement_after_reinitialization(device):
    assert device.command("REINIT_SENSOR") == "REINIT=OK"

    response = device.command("READ_SENSOR")

    assert response.startswith("TEMP_C=")
    assert "PRESSURE_HPA=" in response


def test_command_too_long(device):
    response = device.command("X" * 120)

    assert response == "ERROR=COMMAND_TOO_LONG"
