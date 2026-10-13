/* Native host-side tests for uart/lab01_uart_loopback/main/protocol.c.
 * No ESP32, ESP-IDF, UART hardware, or external test framework required.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "protocol.h"

static int tests_run = 0;

static void expect_response(const char *command, const char *expected)
{
    char response[128] = {0};
    protocol_process_command(command, response, sizeof(response));
    assert(strcmp(response, expected) == 0);
    tests_run++;
}

static void test_ping(void) { expect_response("PING", "PONG"); }
static void test_info(void) { expect_response("GET_INFO", "MODEL=ESP32;FW=1.0.0"); }
static void test_unknown(void) { expect_response("HELLO", "ERROR=UNKNOWN_COMMAND"); }
static void test_case_sensitivity(void) { expect_response("ping", "ERROR=UNKNOWN_COMMAND"); }
static void test_empty_command(void) { expect_response("", "ERROR=UNKNOWN_COMMAND"); }
static void test_whitespace(void) { expect_response("PING ", "ERROR=UNKNOWN_COMMAND"); }

static void test_counter_increments(void)
{
    char first[128] = {0}, second[128] = {0};
    unsigned int a = 0, b = 0;
    protocol_process_command("GET_COUNTER", first, sizeof(first));
    protocol_process_command("GET_COUNTER", second, sizeof(second));
    assert(sscanf(first, "COUNTER=%u", &a) == 1);
    assert(sscanf(second, "COUNTER=%u", &b) == 1);
    assert(b == a + 1);
    tests_run++;
}

static void test_small_buffer(void)
{
    char response[3] = {'X', 'X', 'X'};
    protocol_process_command("PING", response, sizeof(response));
    assert(strcmp(response, "PO") == 0);
    tests_run++;
}

static void test_one_byte_buffer(void)
{
    char response[2] = {'X', 'Y'};
    protocol_process_command("PING", response, 1);
    assert(response[0] == '\0');
    assert(response[1] == 'Y');
    tests_run++;
}

static void test_zero_capacity(void)
{
    char response[2] = {'X', 'Y'};
    protocol_process_command("PING", response, 0);
    assert(response[0] == 'X' && response[1] == 'Y');
    tests_run++;
}

static void test_null_arguments(void)
{
    char response[128] = "unchanged";
    protocol_process_command(NULL, response, sizeof(response));
    assert(strcmp(response, "unchanged") == 0);
    protocol_process_command("PING", NULL, 128);
    tests_run++;
}

int main(void)
{
    test_ping();
    test_info();
    test_unknown();
    test_case_sensitivity();
    test_empty_command();
    test_whitespace();
    test_counter_increments();
    test_small_buffer();
    test_one_byte_buffer();
    test_zero_capacity();
    test_null_arguments();
    printf("PASS: %d native UART protocol tests\n", tests_run);
    return 0;
}
