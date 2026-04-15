#pragma once

#include "app_config.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t app_persist_init(void);
esp_err_t app_persist_load_calibration(app_calibration_t *calibration);
esp_err_t app_persist_save_calibration(const app_calibration_t *calibration);

#ifdef __cplusplus
}
#endif
