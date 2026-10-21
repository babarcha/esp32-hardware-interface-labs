
cmake_minimum_required(VERSION 3.16)

project(spi_lab03_native_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

enable_testing()

set(PRODUCTION_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../../main"
)

set(MOCK_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/mocks"
)

add_executable(test_spi_bus
    "${PRODUCTION_DIR}/spi_bus.c"
    "test_spi_bus.c"
)

target_include_directories(test_spi_bus PRIVATE
    "${PRODUCTION_DIR}"
    "${MOCK_DIR}"
)

add_test(
    NAME spi_bus_native
    COMMAND test_spi_bus
)

add_executable(test_bmp280_spi
    "${PRODUCTION_DIR}/bmp280_spi.c"
    "test_bmp280_spi.c"
)

target_include_directories(test_bmp280_spi PRIVATE
    "${PRODUCTION_DIR}"
    "${MOCK_DIR}"
)

add_test(
    NAME bmp280_spi_native
    COMMAND test_bmp280_spi
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_spi_bus PRIVATE
        -Wall -Wextra -Werror
    )

    target_compile_options(test_bmp280_spi PRIVATE
        -Wall -Wextra -Werror
    )
endif()
