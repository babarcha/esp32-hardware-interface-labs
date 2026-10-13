# I²C Lab 02 — Native unit tests

These tests compile the **production** `main/i2c_bus.c` using a minimal mock ESP-IDF header and fake driver functions. No ESP32, ESP-IDF installation, or external I²C target is needed.

## Coverage

Twelve test groups cover: pre-initialization state; driver installation failure; initialization and duplicate initialization; probe address boundaries; a 112-address scan; invalid device registration; valid registration/removal; write/read/combined-transfer invalid arguments; forwarding calls and timeouts; driver-error propagation; failed cleanup and reinitialization.

## Run from repository root on Linux or WSL with GCC

```bash
gcc -std=c11 -Wall -Wextra -Werror -pedantic \
  -I i2c/lab02_i2c_master/tests/native/mocks \
  -I i2c/lab02_i2c_master/main \
  i2c/lab02_i2c_master/main/i2c_bus.c \
  i2c/lab02_i2c_master/tests/native/test_i2c_bus.c \
  -o /tmp/i2c_native_tests
/tmp/i2c_native_tests
```

On Windows, the GitHub Actions workflow runs these tests automatically on Ubuntu, so you do not need a local GCC installation.

## Limitations

Mocks verify **wrapper control flow, argument validation, driver invocation and error propagation**. They do not verify ESP-IDF ABI compatibility, actual bus timing, pull-up strength, physical ACK/NACK, or sensor behavior. Later run firmware builds with ESP-IDF and physical tests when hardware becomes available.
