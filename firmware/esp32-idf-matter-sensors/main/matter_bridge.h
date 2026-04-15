#pragma once

#include <stdbool.h>

#include "app_config.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool enabled;
    bool commissioned;
    char mode[24];
    char onboarding_code[32];
    char qr_code_url[128];
} matter_bridge_status_t;

esp_err_t matter_bridge_init(void);
esp_err_t matter_bridge_publish(const app_sensor_snapshot_t *snapshot);
matter_bridge_status_t matter_bridge_get_status(void);

#ifdef __cplusplus
}
#endif
