# UART Lab 01 — Native unit tests (no ESP32 required)

## Install paths

- `test_protocol.c` -> `uart/lab01_uart_loopback/tests/native/test_protocol.c`
- `uart-native-tests.yml` -> `.github/workflows/uart-native-tests.yml`

Do not replace `tests/test_uart_protocol.py`: those are physical UART HIL tests.

## Windows: run using WSL Ubuntu (with GCC)

From a WSL shell, navigate to your repository and execute:

```bash
cd /mnt/c/Users/babar/esp32-hardware-interface-labs
gcc -std=c11 -Wall -Wextra -Werror -pedantic \
  -I uart/lab01_uart_loopback/main \
  uart/lab01_uart_loopback/main/protocol.c \
  uart/lab01_uart_loopback/tests/native/test_protocol.c \
  -o /tmp/uart_protocol_tests
/tmp/uart_protocol_tests
```

You can also run the same GCC commands using MSYS2/MinGW on Windows, with a Windows output filename. GCC is not included in ordinary PowerShell by default. GitHub Actions runs the tests on Ubuntu without requiring a local compiler.

## What is tested?

Eleven independent assertions cover PING, GET_INFO, unknown commands, case sensitivity, empty commands, whitespace, monotonic counter increments, truncated output, one-byte buffer, zero-capacity output, and null arguments.

These tests directly compile the existing firmware `protocol.c`, not a reimplementation in Python. They do not test UART framing, `uart_bus.c`, COM3, or physical hardware. The counter test uses relative increments so it does not assume it starts at zero after other tests.

## GitHub CI

Commit both files. The workflow runs on relevant pushes, pull requests, and manual dispatch. It compiles with warnings treated as errors and fails the job if an assertion fails.
