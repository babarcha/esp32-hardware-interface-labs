
import time
import serial

PORT = "COM3"
BAUD_RATE = 115200


def command(ser, cmd):
    ser.write((cmd + "\n").encode())
    ser.flush()

    deadline = time.monotonic() + 3

    while time.monotonic() < deadline:
        response = ser.readline().decode(
            errors="replace"
        ).strip()

        if response.startswith("HIL:"):
            print(f"{cmd:20} -> {response}")
            return response

    raise TimeoutError(f"No HIL response for {cmd}")


def main():
    with serial.Serial(
        PORT,
        BAUD_RATE,
        timeout=1
    ) as ser:

        time.sleep(2)
        ser.reset_input_buffer()

        print("\n=== Pass 11.2: BMP280 Verification ===\n")

        command(ser, "PING")
        command(ser, "GET_STATUS")
        command(ser, "GET_SENSOR_INFO")

        print("\n=== Repeated Measurements ===\n")

        for i in range(10):
            print(f"Sample {i + 1:02d} | ", end="")
            command(ser, "READ_SENSOR")
            time.sleep(1)

        print("\n=== Sensor Reinitialization ===\n")

        command(ser, "REINIT_SENSOR")
        command(ser, "GET_STATUS")
        command(ser, "GET_SENSOR_INFO")
        command(ser, "READ_SENSOR")

        print("\n=== Verification sequence finished ===")


if __name__ == "__main__":
    main()
