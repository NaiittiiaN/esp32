#include "matter_bridge.h"

#include <stdio.h>
#include <string.h>

#include "state_log.h"

#if __has_include("esp_matter.h")
#define APP_HAS_ESP_MATTER 1
#include "esp_matter.h"
#else
#define APP_HAS_ESP_MATTER 0
#endif

static matter_bridge_status_t s_status;

esp_err_t matter_bridge_init(void) {
    memset(&s_status, 0, sizeof(s_status));

#if APP_HAS_ESP_MATTER
    s_status.enabled = true;
    snprintf(s_status.mode, sizeof(s_status.mode), "%s", "esp-matter");
    snprintf(s_status.onboarding_code, sizeof(s_status.onboarding_code), "%s", "generated-at-runtime");
    snprintf(s_status.qr_code_url, sizeof(s_status.qr_code_url), "%s", "generated-at-runtime");
    state_log_write(APP_LOG_LEVEL_INFO, "MATTER", "ESP-Matter component detected, integrate endpoints here");
    return ESP_OK;
#else
    s_status.enabled = false;
    s_status.commissioned = false;
    snprintf(s_status.mode, sizeof(s_status.mode), "%s", "stub");
    snprintf(s_status.onboarding_code, sizeof(s_status.onboarding_code), "%s", "esp_matter missing");
    snprintf(s_status.qr_code_url, sizeof(s_status.qr_code_url), "%s", "esp_matter missing");
    state_log_write(APP_LOG_LEVEL_WARN, "MATTER",
                    "ESP-Matter not vendored locally; running stub bridge until components/esp_matter is added");
    return ESP_OK;
#endif
}

esp_err_t matter_bridge_publish(const app_sensor_snapshot_t *snapshot) {
    if (snapshot == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

#if APP_HAS_ESP_MATTER
    state_log_write(APP_LOG_LEVEL_INFO, "MATTER",
                    "Publish ds=%.2fC am_t=%.2fC am_h=%.2f%%",
                    snapshot->ds18b20_temperature_c,
                    snapshot->am2320_temperature_c,
                    snapshot->am2320_humidity_pct);
    return ESP_OK;
#else
    state_log_write(APP_LOG_LEVEL_INFO, "MATTER",
                    "Stub publish ds=%.2fC am_t=%.2fC am_h=%.2f%%",
                    snapshot->ds18b20_temperature_c,
                    snapshot->am2320_temperature_c,
                    snapshot->am2320_humidity_pct);
    return ESP_OK;
#endif
}

matter_bridge_status_t matter_bridge_get_status(void) {
    return s_status;
}
