#ifndef MOCK_FREERTOS_H
#define MOCK_FREERTOS_H
#include <stdint.h>
#define pdMS_TO_TICKS(ms) ((uint32_t)(ms) / 10U)
#endif
