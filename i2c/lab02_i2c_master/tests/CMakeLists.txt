
cmake_minimum_required(VERSION 3.16)
project(bmp280_native_tests C)
enable_testing()

add_executable(test_bmp280_compensation
    test_bmp280_compensation.c
    ../main/bmp280_compensation.c
)
target_include_directories(test_bmp280_compensation PRIVATE ../main)

add_executable(test_i2c_bus
    native/test_i2c_bus.c
    ../main/i2c_bus.c
)
target_include_directories(test_i2c_bus PRIVATE native/mocks ../main)

foreach(target IN ITEMS test_bmp280_compensation test_i2c_bus)
    set_target_properties(${target} PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Werror)
    endif()
endforeach()

add_test(NAME bmp280_compensation COMMAND test_bmp280_compensation)
add_test(NAME i2c_bus COMMAND test_i2c_bus)


# Sensor-manager native tests with mocked sensor and FreeRTOS APIs.
add_executable(
    test_sensor_manager
    native/test_sensor_manager.c
    ../main/sensor_manager.c
)

target_include_directories(
    test_sensor_manager
    PRIVATE
    native/mocks
    ../main
)

set_target_properties(
    test_sensor_manager
    PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
)

if(MSVC)
    target_compile_options(test_sensor_manager PRIVATE /W4)
else()
    target_compile_options(
        test_sensor_manager
        PRIVATE -Wall -Wextra -Werror
    )
endif()

add_test(
    NAME sensor_manager
    COMMAND test_sensor_manager
)


# Optional GCC coverage instrumentation.
option(ENABLE_COVERAGE "Enable native C test coverage" OFF)

if(ENABLE_COVERAGE)
    if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "Coverage requires GCC")
    endif()

    foreach(target IN ITEMS
        test_bmp280_compensation
        test_i2c_bus
        test_sensor_manager
    )
        target_compile_options(${target} PRIVATE
            --coverage
            -O0
            -g
        )
        target_link_options(${target} PRIVATE --coverage)
    endforeach()
endif()
