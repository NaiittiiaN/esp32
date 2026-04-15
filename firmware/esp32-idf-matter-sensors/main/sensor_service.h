#pragma once

#include "app_config.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t sensor_service_init(void);
esp_err_t sensor_service_poll_now(void);
esp_err_t sensor_service_get_snapshot(app_sensor_snapshot_t *snapshot, app_calibration_t *calibration);
esp_err_t sensor_service_set_calibration(const app_calibration_t *calibration);

#ifdef __cplusplus
}
#endif
