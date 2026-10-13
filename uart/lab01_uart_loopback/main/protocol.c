#include "protocol.h"
#include <stdio.h>
#include <string.h>

static unsigned int counter = 0;

void protocol_process_command(const char *command, char *response, size_t response_size)
{
    if (command == NULL || response == NULL || response_size == 0) return;
    if (strcmp(command, "PING") == 0) {
        snprintf(response, response_size, "PONG");
    } else if (strcmp(command, "GET_INFO") == 0) {
        snprintf(response, response_size, "MODEL=ESP32;FW=1.0.0");
    } else if (strcmp(command, "GET_COUNTER") == 0) {
        snprintf(response, response_size, "COUNTER=%u", ++counter);
    } else {
        snprintf(response, response_size, "ERROR=UNKNOWN_COMMAND");
    }
}
