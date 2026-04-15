#include "sensor_service.h"

#include <math.h>
#include <string.h>

#include "am2320.h"
#include "app_persist.h"
#include "driver/i2c.h"
#include "ds18b20.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "onewire_bus.h"
#include "state_log.h"

static SemaphoreHandle_t s_sensor_lock;
static app_sensor_snapshot_t s_snapshot;
static app_calibration_t s_calibration;
static onewire_bus_t s_onewire_bus;
static ds18b20_device_t s_ds18b20;
static am2320_device_t s_am2320;

static bool valid_temperature(float value) {
    return !isnan(value) && value > -55.0f && value < 125.0f;
}

static bool valid_humidity(float value) {
    return !isnan(value) && value >= 0.0f && value <= 100.0f;
}

static esp_err_t sensor_service_i2c_init(void) {
    const i2c_config_t config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = APP_I2C_SDA_GPIO,
        .scl_io_num = APP_I2C_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
        .clk_flags = 0,
    };

    ESP_RETURN_ON_ERROR(i2c_param_config(I2C_NUM_0, &config), "SENSOR", "i2c_param_config failed");
    return i2c_driver_install(I2C_NUM_0, config.mode, 0, 0, 0);
}

esp_err_t sensor_service_init(void) {
    s_sensor_lock = xSemaphoreCreateMutex();
    if (s_sensor_lock == NULL) {
        return ESP_ERR_NO_MEM;
    }

    memset(&s_snapshot, 0, sizeof(s_snapshot));
    ESP_RETURN_ON_ERROR(app_persist_load_calibration(&s_calibration), "SENSOR", "load calibration failed");

    ESP_RETURN_ON_ERROR(onewire_bus_init(&s_onewire_bus, APP_DS18B20_GPIO), "SENSOR", "onewire init failed");
    sensor_service_i2c_init();

    esp_err_t ds_err = ds18b20_init(&s_ds18b20, &s_onewire_bus);
    s_snapshot.ds18b20_present = (ds_err == ESP_OK);
    state_log_write(s_snapshot.ds18b20_present ? APP_LOG_LEVEL_INFO : APP_LOG_LEVEL_WARN,
                    "SENSOR", "DS18B20 %s", s_snapshot.ds18b20_present ? "detected" : "not detected");

    ESP_RETURN_ON_ERROR(am2320_init(&s_am2320, I2C_NUM_0), "SENSOR", "am2320 init failed");
    return sensor_service_poll_now();
}

esp_err_t sensor_service_poll_now(void) {
    float ds_temp = NAN;
    float am_temp = NAN;
    float am_humidity = NAN;

    esp_err_t ds_err = ds18b20_read_temperature_c(&s_ds18b20, &ds_temp);
    esp_err_t am_err = am2320_read(&s_am2320, &am_temp, &am_humidity);

    if (xSemaphoreTake(s_sensor_lock, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    s_snapshot.ds18b20_present = (ds_err == ESP_OK);
    s_snapshot.am2320_present = (am_err == ESP_OK);

    if (ds_err == ESP_OK && valid_temperature(ds_temp)) {
        s_snapshot.ds18b20_temperature_c = ds_temp + s_calibration.ds18b20_temp_offset_c;
    }
    if (am_err == ESP_OK && valid_temperature(am_temp)) {
        s_snapshot.am2320_temperature_c = am_temp + s_calibration.am2320_temp_offset_c;
    }
    if (am_err == ESP_OK && valid_humidity(am_humidity)) {
        s_snapshot.am2320_humidity_pct = am_humidity + s_calibration.am2320_humidity_offset_pct;
    }
    s_snapshot.last_update_ms = esp_timer_get_time() / 1000ULL;

    xSemaphoreGive(s_sensor_lock);

    state_log_write(ds_err == ESP_OK ? APP_LOG_LEVEL_INFO : APP_LOG_LEVEL_WARN,
                    "SENSOR", "DS18B20 read %s temp=%.2fC", ds_err == ESP_OK ? "ok" : "failed", ds_temp);
    state_log_write(am_err == ESP_OK ? APP_LOG_LEVEL_INFO : APP_LOG_LEVEL_WARN,
                    "SENSOR", "AM2320 read %s temp=%.2fC humidity=%.2f%%", am_err == ESP_OK ? "ok" : "failed", am_temp,
                    am_humidity);

    if (ds_err != ESP_OK && am_err != ESP_OK) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t sensor_service_get_snapshot(app_sensor_snapshot_t *snapshot, app_calibration_t *calibration) {
    if (snapshot == NULL || calibration == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(s_sensor_lock, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    *snapshot = s_snapshot;
    *calibration = s_calibration;
    xSemaphoreGive(s_sensor_lock);
    return ESP_OK;
}

esp_err_t sensor_service_set_calibration(const app_calibration_t *calibration) {
    if (calibration == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(s_sensor_lock, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    s_calibration = *calibration;
    xSemaphoreGive(s_sensor_lock);

    ESP_RETURN_ON_ERROR(app_persist_save_calibration(calibration), "SENSOR", "save calibration failed");
    state_log_write(APP_LOG_LEVEL_INFO, "SENSOR",
                    "Calibration updated ds=%.2f am_t=%.2f am_h=%.2f",
                    calibration->ds18b20_temp_offset_c,
                    calibration->am2320_temp_offset_c,
                    calibration->am2320_humidity_offset_pct);
    return ESP_OK;
}
