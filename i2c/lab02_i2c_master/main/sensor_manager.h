
#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <stdbool.h>
#include "esp_err.h"
#include "bme280.h"

typedef struct
{
    bmp280_t sensor;
    bool ready;
} sensor_manager_t;

void sensor_manager_init(sensor_manager_t *manager);

esp_err_t sensor_manager_reinit(sensor_manager_t *manager);

bmp280_t *sensor_manager_get_active(sensor_manager_t *manager);

bool sensor_manager_is_ready(const sensor_manager_t *manager);

void sensor_manager_mark_unavailable(sensor_manager_t *manager);

#endif
