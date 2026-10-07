"""
UART Lab 01 hardware-in-the-loop tests.

Uses pytest and PySerial to validate the command/response protocol
running on a physical ESP32 connected through COM3.
"""

import time

import pytest
import serial


PORT = "COM3"
BAUD_RATE = 115200


@pytest.fixture(scope="module")
def uart():
    """
    Open one serial connection for the complete test module.

    Module scope avoids repeatedly opening COM3 and resetting the ESP32
    before every individual test.
    """
    ser = serial.Serial(
        port=PORT,
        baudrate=BAUD_RATE,
        timeout=0.2,
    )

    # Opening COM3 may reset the ESP32; allow the firmware to boot.
    time.sleep(2)

    # Remove boot output before starting protocol validation.
    ser.reset_input_buffer()

    yield ser

    # Release COM3 after all tests in this module have completed.
    ser.close()


def send_command(ser, command):
    """
    Send one newline-delimited command and return its diagnostic
    command echo and protocol response.

    The firmware produces transactions such as:

        CMD: PING
        PONG

    Synchronizing on the CMD line prevents unrelated serial output
    from being mistaken for the response to the current command.
    """

    # Start each transaction without stale data from a previous command.
    ser.reset_input_buffer()

    ser.write((command + "\n").encode("utf-8"))
    ser.flush()

    expected_debug = f"CMD: {command}"

    debug_line = None
    response = None

    # Bound the complete transaction so a failed DUT cannot hang pytest.
    deadline = time.time() + 2.0

    while time.time() < deadline:

        raw = ser.readline()

        if not raw:
            continue

        line = raw.decode(
            "utf-8",
            errors="ignore",
        ).strip()

        if not line:
            continue

        print(f"UART <- {line}")

        # Wait until the firmware confirms receipt of this command.
        if expected_debug in line:
            debug_line = expected_debug
            continue

        # The next meaningful line after the command echo is its response.
        if debug_line is not None:
            response = line
            break

    return debug_line, response


def test_ping(uart):
    """Verify basic host-to-DUT UART communication."""
    debug, response = send_command(uart, "PING")

    assert debug == "CMD: PING"
    assert response == "PONG"


def test_get_info(uart):
    """Verify the expected device model and firmware version."""
    debug, response = send_command(uart, "GET_INFO")

    assert debug == "CMD: GET_INFO"
    assert response == "MODEL=ESP32;FW=1.0.0"


def test_counter(uart):
    """Verify that consecutive GET_COUNTER requests increment by one."""
    _, first = send_command(uart, "GET_COUNTER")
    _, second = send_command(uart, "GET_COUNTER")

    assert first is not None
    assert second is not None

    first_value = int(first.split("=")[1])
    second_value = int(second.split("=")[1])

    assert second_value == first_value + 1


def test_unknown_command(uart):
    """Verify deterministic error handling for an unsupported command."""
    debug, response = send_command(uart, "HELLO")

    assert debug == "CMD: HELLO"
    assert response == "ERROR=UNKNOWN_COMMAND"
