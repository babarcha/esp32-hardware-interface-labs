
#ifndef MOCK_FREERTOS_H
#define MOCK_FREERTOS_H

/*
 * Host-native FreeRTOS compatibility stub.
 * A real scheduler is not required.
 */

#define pdMS_TO_TICKS(ms) (ms)

#endif /* MOCK_FREERTOS_H */
