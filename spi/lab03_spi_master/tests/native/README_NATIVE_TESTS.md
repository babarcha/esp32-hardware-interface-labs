# SPI Lab 03 — host-native unit tests

This test suite compiles the **production** `main/spi_bus.c` with minimal mock ESP-IDF headers and fake SPI driver functions. No ESP32, SPI device, or ESP-IDF installation is required for these tests.

## Run on Linux / GitHub Actions

From the repository root:

```bash
gcc -std=c11 -Wall -Wextra -Werror -pedantic \
  -I spi/lab03_spi_master/tests/native/mocks \
  -I spi/lab03_spi_master/main \
  spi/lab03_spi_master/main/spi_bus.c \
  spi/lab03_spi_master/tests/native/test_spi_bus.c \
  -o /tmp/spi_native_tests
/tmp/spi_native_tests
```

The 12 test groups cover uninitialized state, SPI3 pin/DMA configuration, initialization and deinitialization failures, device registration validation and configuration, device removal, write/read/full-duplex transaction fields, driver error propagation, and reinitialization.

The tests inspect mock driver calls, including transaction lengths in **bits**, but do **not** establish physical SPI signaling or peripheral data correctness. They also do not validate all ESP-IDF constraints such as maximum transfer size or valid physical GPIO choices.

GitHub Actions: `.github/workflows/spi-native-tests.yml`.
