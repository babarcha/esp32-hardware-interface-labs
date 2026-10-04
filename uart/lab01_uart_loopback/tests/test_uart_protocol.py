import time

import pytest
import serial


PORT = "COM3"
BAUD_RATE = 115200


@pytest.fixture(scope="module")
def uart():
    ser = serial.Serial(
        port=PORT,
        baudrate=BAUD_RATE,
        timeout=0.2,
    )

    # Opening COM3 can reset the ESP32.
    time.sleep(2)

    ser.reset_input_buffer()

    yield ser

    ser.close()


def send_command(ser, command):
    """
    Send one command and collect UART lines until the
    corresponding CMD debug line and response are found.
    """

    ser.reset_input_buffer()

    ser.write((command + "\n").encode("utf-8"))
    ser.flush()

    expected_debug = f"CMD: {command}"

    debug_line = None
    response = None

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

        # Synchronize on our command echo.
        if expected_debug in line:
            debug_line = expected_debug
            continue

        # Once the command has been recognized,
        # the next meaningful line is its response.
        if debug_line is not None:
            response = line
            break

    return debug_line, response


def test_ping(uart):
    debug, response = send_command(uart, "PING")

    assert debug == "CMD: PING"
    assert response == "PONG"


def test_get_info(uart):
    debug, response = send_command(uart, "GET_INFO")

    assert debug == "CMD: GET_INFO"
    assert response == "MODEL=ESP32;FW=1.0.0"


def test_counter(uart):
    _, first = send_command(uart, "GET_COUNTER")
    _, second = send_command(uart, "GET_COUNTER")

    assert first is not None
    assert second is not None

    first_value = int(first.split("=")[1])
    second_value = int(second.split("=")[1])

    assert second_value == first_value + 1


def test_unknown_command(uart):
    debug, response = send_command(uart, "HELLO")

    assert debug == "CMD: HELLO"
    assert response == "ERROR=UNKNOWN_COMMAND"
