import serial
import time

PORT = "COM3"
BAUD_RATE = 115200

commands = [
    "PING",
    "GET_INFO",
    "GET_COUNTER",
    "GET_COUNTER",
    "HELLO",
]

print(f"Opening {PORT} at {BAUD_RATE} baud...")

with serial.Serial(PORT, BAUD_RATE, timeout=1) as ser:

    # Opening the serial port may reset the ESP32.
    time.sleep(2)

    # Discard boot messages.
    ser.reset_input_buffer()

    for command in commands:

        print(f"\nTX -> {command}")

        ser.write((command + "\n").encode("utf-8"))
        ser.flush()

        # Firmware currently produces:
        #
        # CMD: PING
        # PONG
        #
        # so read two lines.
        line1 = ser.readline().decode(
            "utf-8",
            errors="replace"
        ).strip()

        line2 = ser.readline().decode(
            "utf-8",
            errors="replace"
        ).strip()

        print(f"RX <- {line1}")
        print(f"RX <- {line2}")
