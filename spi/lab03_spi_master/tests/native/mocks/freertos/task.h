
#ifndef MOCK_FREERTOS_TASK_H
#define MOCK_FREERTOS_TASK_H

#include <stdint.h>

/*
 * Implemented by the native test executable.
 * Counts delay calls without sleeping.
 */
void vTaskDelay(uint32_t ticks);

#endif /* MOCK_FREERTOS_TASK_H */
