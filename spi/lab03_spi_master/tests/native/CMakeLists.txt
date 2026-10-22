
cmake_minimum_required(VERSION 3.16)

project(spi_lab03_native_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

enable_testing()

set(PRODUCTION_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../../main"
)

set(MOCK_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/mocks"
)

# Production SPI bus tests.
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

# Production BMP280 driver tests.
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

# Enable strict diagnostics on GCC/Clang.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_spi_bus PRIVATE
        -Wall
        -Wextra
        -Werror
    )

    target_compile_options(test_bmp280_spi PRIVATE
        -Wall
        -Wextra
        -Werror
    )
endif()


# --------------------------------------------------
# Pass 10.9 — Optional GCC coverage instrumentation
# --------------------------------------------------

option(ENABLE_COVERAGE
    "Enable GCC source coverage instrumentation"
    OFF
)

if(ENABLE_COVERAGE)
    if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR
            "Coverage requires GCC for this lab"
        )
    endif()

    foreach(target IN ITEMS test_spi_bus test_bmp280_spi)
        target_compile_options(${target} PRIVATE
            -O0
            -g
            --coverage
        )

        target_link_options(${target} PRIVATE
            --coverage
        )
    endforeach()

    message(STATUS "Native C coverage enabled")
endif()
