# CAN/TWAI Lab 04 — native unit tests

The test harness compiles the **real** `main/twai_bus.c` against minimal mock
ESP-IDF TWAI and FreeRTOS headers. Mock driver functions record calls and
inject return codes. No ESP32, CAN transceiver, or CAN peer is required.

## Run from repository root (Linux or WSL with GCC)

```sh
gcc -std=c11 -Wall -Wextra -Werror -pedantic \
  -I can/lab04_twai/tests/native/mocks \
  -I can/lab04_twai/main \
  can/lab04_twai/main/twai_bus.c \
  can/lab04_twai/tests/native/test_twai_bus.c \
  -o /tmp/twai_native_tests
/tmp/twai_native_tests
```

Fourteen test groups cover driver/controller lifecycle, configuration,
invalid arguments, DLC and CAN identifier limits, transmit/receive
forwarding, millisecond-to-tick conversion, error propagation and recovery.

The mock FreeRTOS tick rate is intentionally **100 Hz** (10 ms per tick).
This checks wrapper conversion behavior, not real scheduling precision.

**Not covered:** CAN transceiver behavior, arbitration, ACK, bus errors,
real CAN frames, physical signaling or on-device FreeRTOS timing.
