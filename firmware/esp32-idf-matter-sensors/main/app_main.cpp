#include <stdio.h>

#include "app_config.h"
#include "app_persist.h"
#include "diagnostics_http.h"
#include "esp_check.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "matter_bridge.h"
#include "sensor_service.h"
#include "state_log.h"
#include "wifi_manager.h"

static void sensor_task(void *arg) {
    (void)arg;

    while (true) {
        app_sensor_snapshot_t snapshot;
        app_calibration_t calibration;

        if (sensor_service_poll_now() == ESP_OK && sensor_service_get_snapshot(&snapshot, &calibration) == ESP_OK) {
            matter_bridge_publish(&snapshot);
        }

        vTaskDelay(pdMS_TO_TICKS(APP_SENSOR_POLL_INTERVAL_MS));
    }
}

extern "C" void app_main(void) {
    ESP_ERROR_CHECK(app_persist_init());
    state_log_init();
    state_log_write(APP_LOG_LEVEL_INFO, "APP", "Booting %s", app_device_name());

    ESP_ERROR_CHECK(wifi_manager_init());
    ESP_ERROR_CHECK(wifi_manager_connect());

    ESP_ERROR_CHECK(sensor_service_init());
    ESP_ERROR_CHECK(matter_bridge_init());
    ESP_ERROR_CHECK(diagnostics_http_start());

    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
    state_log_write(APP_LOG_LEVEL_INFO, "APP", "System initialization complete");
}
