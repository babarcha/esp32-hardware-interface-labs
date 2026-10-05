#include <stdio.h>

#include "esp_err.h"
#include "driver/twai.h"

#include "twai_bus.h"

static int tests_passed = 0;
static int tests_failed = 0;

static void check_result(
    const char *name,
    esp_err_t actual,
    esp_err_t expected)
{
    printf("%-38s : ", name);

    if (actual == expected)
    {
        printf("PASS\n");
        tests_passed++;
    }
    else
    {
        printf(
            "FAIL  expected=%s actual=%s\n",
            esp_err_to_name(expected),
            esp_err_to_name(actual));

        tests_failed++;
    }
}

void app_main(void)
{
    printf("\n");
    printf("CAN/TWAI Lab 04 - Pass 7: Boundary Validation\n");
    printf("----------------------------------------------\n\n");

    esp_err_t result;

    twai_message_t message = {0};

    message.identifier = 0x123;
    message.extd = 0;
    message.rtr = 0;
    message.data_length_code = 3;

    message.data[0] = 0xAA;
    message.data[1] = 0xBB;
    message.data[2] = 0xCC;

    /*
     * TEST 1
     * Controller has not been initialized.
     */
    result = twai_bus_transmit(&message, 100);

    check_result(
        "Transmit before initialization",
        result,
        ESP_ERR_INVALID_STATE);

    /*
     * Initialize and start.
     */
    result = twai_bus_init();

    if (result != ESP_OK)
    {
        printf(
            "FATAL: initialization failed: %s\n",
            esp_err_to_name(result));

        return;
    }

    result = twai_bus_start();

    if (result != ESP_OK)
    {
        printf(
            "FATAL: start failed: %s\n",
            esp_err_to_name(result));

        twai_bus_deinit();
        return;
    }

    /*
     * TEST 2
     */
    result = twai_bus_start();

    check_result(
        "Start controller twice",
        result,
        ESP_ERR_INVALID_STATE);

    /*
     * TEST 3
     */
    result = twai_bus_transmit(NULL, 100);

    check_result(
        "Transmit NULL message",
        result,
        ESP_ERR_INVALID_ARG);

    /*
     * TEST 4
     *
     * Classical CAN maximum DLC = 8.
     */
    message.data_length_code = 9;

    result = twai_bus_transmit(&message, 100);

    check_result(
        "DLC = 9",
        result,
        ESP_ERR_INVALID_ARG);

    message.data_length_code = 3;

    /*
     * TEST 5
     *
     * Standard 11-bit identifier:
     *
     * maximum = 0x7FF
     * 0x800 is invalid.
     */
    message.extd = 0;
    message.identifier = 0x800;

    result = twai_bus_transmit(&message, 100);

    check_result(
        "Standard ID = 0x800",
        result,
        ESP_ERR_INVALID_ARG);

    /*
     * TEST 6
     *
     * Extended CAN identifier maximum:
     *
     * 0x1FFFFFFF
     *
     * Therefore 0x20000000 is invalid.
     */
    message.extd = 1;
    message.identifier = 0x20000000;

    result = twai_bus_transmit(&message, 100);

    check_result(
        "Extended ID = 0x20000000",
        result,
        ESP_ERR_INVALID_ARG);

    /*
     * Restore valid frame.
     */
    message.extd = 0;
    message.identifier = 0x123;

    /*
     * TEST 7
     *
     * Our abstraction should not allow the
     * installed driver to be removed while
     * the controller is running.
     */
    result = twai_bus_deinit();

    check_result(
        "Deinit while running",
        result,
        ESP_ERR_INVALID_STATE);

    /*
     * Normal stop.
     */
    result = twai_bus_stop();

    if (result != ESP_OK)
    {
        printf(
            "FATAL: controller stop failed: %s\n",
            esp_err_to_name(result));

        return;
    }

    /*
     * TEST 8
     */
    result = twai_bus_stop();

    check_result(
        "Stop controller twice",
        result,
        ESP_ERR_INVALID_STATE);

    /*
     * TEST 9
     *
     * Receive requires a running controller.
     */
    twai_message_t rx_message = {0};

    result = twai_bus_receive(
        &rx_message,
        100);

    check_result(
        "Receive after stop",
        result,
        ESP_ERR_INVALID_STATE);

    /*
     * TEST 10
     *
     * Driver is installed but controller is
     * stopped, so deinitialization is legal.
     */
    result = twai_bus_deinit();

    check_result(
        "Normal deinitialization",
        result,
        ESP_OK);

    /*
     * Summary.
     */
    printf("\n");
    printf("Validation summary\n");
    printf("------------------\n");
    printf("Passed : %d\n", tests_passed);
    printf("Failed : %d\n", tests_failed);

    if (tests_failed == 0)
    {
        printf("RESULT : ALL TESTS PASSED\n");
    }
    else
    {
        printf("RESULT : VALIDATION FAILED\n");
    }
}