
#ifndef MOCK_ESP_ERR_H
#define MOCK_ESP_ERR_H

/*
 * Minimal ESP-IDF error definitions
 * for host-native unit testing.
 */

typedef int esp_err_t;

#define ESP_OK 0
#define ESP_FAIL -1

#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_INVALID_STATE 0x103
#define ESP_ERR_TIMEOUT 0x107
#define ESP_ERR_INVALID_RESPONSE 0x108

#endif /* MOCK_ESP_ERR_H */
