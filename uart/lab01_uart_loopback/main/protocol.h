#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stddef.h>
void protocol_process_command(const char *command, char *response, size_t response_size);
#endif
