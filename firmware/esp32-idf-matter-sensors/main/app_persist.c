#include "app_persist.h"

#include <string.h>

#include "nvs.h"
#include "nvs_flash.h"

#define APP_NVS_NAMESPACE "appcfg"
#define APP_NVS_KEY_CALIB "calib"

esp_err_t app_persist_init(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

esp_err_t app_persist_load_calibration(app_calibration_t *calibration) {
    if (calibration == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(calibration, 0, sizeof(*calibration));

    nvs_handle_t handle;
    esp_err_t err = nvs_open(APP_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return err == ESP_ERR_NVS_NOT_FOUND ? ESP_OK : err;
    }

    size_t size = sizeof(*calibration);
    err = nvs_get_blob(handle, APP_NVS_KEY_CALIB, calibration, &size);
    nvs_close(handle);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        memset(calibration, 0, sizeof(*calibration));
        return ESP_OK;
    }

    return err;
}

esp_err_t app_persist_save_calibration(const app_calibration_t *calibration) {
    if (calibration == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(APP_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_blob(handle, APP_NVS_KEY_CALIB, calibration, sizeof(*calibration));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }

    nvs_close(handle);
    return err;
}
