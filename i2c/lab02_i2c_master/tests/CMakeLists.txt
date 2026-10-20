
cmake_minimum_required(VERSION 3.16)

project(bmp280_native_tests C)

enable_testing()

add_executable(
    test_bmp280_compensation
    test_bmp280_compensation.c
    ../main/bmp280_compensation.c
)

target_include_directories(
    test_bmp280_compensation
    PRIVATE
    ../main
)

set_target_properties(
    test_bmp280_compensation
    PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
)

if(MSVC)
    target_compile_options(
        test_bmp280_compensation
        PRIVATE /W4
    )
else()
    target_compile_options(
        test_bmp280_compensation
        PRIVATE -Wall -Wextra -Werror
    )
endif()

add_test(
    NAME bmp280_compensation
    COMMAND test_bmp280_compensation
)
