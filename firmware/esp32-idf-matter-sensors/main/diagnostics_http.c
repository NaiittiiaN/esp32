#include "diagnostics_http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_config.h"
#include "cJSON.h"
#include "esp_http_server.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "matter_bridge.h"
#include "mdns.h"
#include "sensor_service.h"
#include "state_log.h"
#include "wifi_manager.h"

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t app_js_start[] asm("_binary_app_js_start");
extern const uint8_t app_js_end[] asm("_binary_app_js_end");
extern const uint8_t styles_css_start[] asm("_binary_styles_css_start");
extern const uint8_t styles_css_end[] asm("_binary_styles_css_end");

static httpd_handle_t s_server;

static esp_err_t respond_blob(httpd_req_t *req, const char *content_type, const uint8_t *start, const uint8_t *end) {
    httpd_resp_set_type(req, content_type);
    return httpd_resp_send(req, (const char *)start, end - start);
}

static esp_err_t index_handler(httpd_req_t *req) {
    return respond_blob(req, "text/html", index_html_start, index_html_end);
}

static esp_err_t app_js_handler(httpd_req_t *req) {
    return respond_blob(req, "application/javascript", app_js_start, app_js_end);
}

static esp_err_t styles_css_handler(httpd_req_t *req) {
    return respond_blob(req, "text/css", styles_css_start, styles_css_end);
}

static esp_err_t status_handler(httpd_req_t *req) {
    app_sensor_snapshot_t snapshot;
    app_calibration_t calibration;
    sensor_service_get_snapshot(&snapshot, &calibration);
    wifi_manager_status_t wifi = wifi_manager_get_status();
    matter_bridge_status_t matter = matter_bridge_get_status();

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "deviceName", app_device_name());
    cJSON_AddStringToObject(root, "host", app_mdns_hostname());
    cJSON_AddNumberToObject(root, "uptimeMs", (double)(esp_timer_get_time() / 1000ULL));

    cJSON *wifi_json = cJSON_AddObjectToObject(root, "wifi");
    cJSON_AddBoolToObject(wifi_json, "connected", wifi.connected);
    cJSON_AddStringToObject(wifi_json, "ssid", wifi.ssid);
    cJSON_AddStringToObject(wifi_json, "ip", wifi.ip);
    cJSON_AddNumberToObject(wifi_json, "rssi", wifi.rssi);

    cJSON *matter_json = cJSON_AddObjectToObject(root, "matter");
    cJSON_AddBoolToObject(matter_json, "enabled", matter.enabled);
    cJSON_AddBoolToObject(matter_json, "commissioned", matter.commissioned);
    cJSON_AddStringToObject(matter_json, "mode", matter.mode);
    cJSON_AddStringToObject(matter_json, "onboardingCode", matter.onboarding_code);
    cJSON_AddStringToObject(matter_json, "qrCodeUrl", matter.qr_code_url);

    cJSON *sensors = cJSON_AddObjectToObject(root, "sensors");
    cJSON_AddBoolToObject(sensors, "ds18b20Present", snapshot.ds18b20_present);
    cJSON_AddBoolToObject(sensors, "am2320Present", snapshot.am2320_present);
    cJSON_AddNumberToObject(sensors, "ds18b20TemperatureC", snapshot.ds18b20_temperature_c);
    cJSON_AddNumberToObject(sensors, "am2320TemperatureC", snapshot.am2320_temperature_c);
    cJSON_AddNumberToObject(sensors, "am2320HumidityPct", snapshot.am2320_humidity_pct);
    cJSON_AddNumberToObject(sensors, "lastUpdateMs", (double)snapshot.last_update_ms);

    cJSON *calibration_json = cJSON_AddObjectToObject(root, "calibration");
    cJSON_AddNumberToObject(calibration_json, "ds18b20TempOffsetC", calibration.ds18b20_temp_offset_c);
    cJSON_AddNumberToObject(calibration_json, "am2320TempOffsetC", calibration.am2320_temp_offset_c);
    cJSON_AddNumberToObject(calibration_json, "am2320HumidityOffsetPct", calibration.am2320_humidity_offset_pct);

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    httpd_resp_set_type(req, "application/json");
    esp_err_t err = httpd_resp_sendstr(req, body);
    cJSON_free(body);
    return err;
}

static esp_err_t logs_handler(httpd_req_t *req) {
    app_log_entry_t entries[APP_LOG_RING_SIZE];
    size_t count = state_log_snapshot(entries, APP_LOG_RING_SIZE);
    cJSON *root = cJSON_CreateArray();

    for (size_t i = 0; i < count; ++i) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddNumberToObject(item, "timestampMs", (double)entries[i].timestamp_ms);
        cJSON_AddStringToObject(item, "level", state_log_level_name(entries[i].level));
        cJSON_AddStringToObject(item, "tag", entries[i].tag);
        cJSON_AddStringToObject(item, "message", entries[i].message);
        cJSON_AddItemToArray(root, item);
    }

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    httpd_resp_set_type(req, "application/json");
    esp_err_t err = httpd_resp_sendstr(req, body);
    cJSON_free(body);
    return err;
}

static esp_err_t sensor_read_handler(httpd_req_t *req) {
    esp_err_t err = sensor_service_poll_now();
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "sensor poll failed");
        return err;
    }
    return status_handler(req);
}

static esp_err_t calibration_post_handler(httpd_req_t *req) {
    char buffer[256] = {0};
    int received = httpd_req_recv(req, buffer, sizeof(buffer) - 1);
    if (received <= 0) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "empty body");
    }

    cJSON *root = cJSON_Parse(buffer);
    if (root == NULL) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid json");
    }

    app_calibration_t calibration = {0};
    cJSON *item = NULL;

    item = cJSON_GetObjectItemCaseSensitive(root, "ds18b20TempOffsetC");
    calibration.ds18b20_temp_offset_c = cJSON_IsNumber(item) ? (float)item->valuedouble : 0.0f;
    item = cJSON_GetObjectItemCaseSensitive(root, "am2320TempOffsetC");
    calibration.am2320_temp_offset_c = cJSON_IsNumber(item) ? (float)item->valuedouble : 0.0f;
    item = cJSON_GetObjectItemCaseSensitive(root, "am2320HumidityOffsetPct");
    calibration.am2320_humidity_offset_pct = cJSON_IsNumber(item) ? (float)item->valuedouble : 0.0f;

    cJSON_Delete(root);

    esp_err_t err = sensor_service_set_calibration(&calibration);
    if (err != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "failed to save calibration");
    }

    return status_handler(req);
}

esp_err_t diagnostics_http_start(void) {
    mdns_init();
    mdns_hostname_set(app_mdns_hostname());
    mdns_instance_name_set(app_device_name());

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.max_uri_handlers = 12;

    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK) {
        return err;
    }

    const httpd_uri_t routes[] = {
        {.uri = "/", .method = HTTP_GET, .handler = index_handler, .user_ctx = NULL},
        {.uri = "/app.js", .method = HTTP_GET, .handler = app_js_handler, .user_ctx = NULL},
        {.uri = "/styles.css", .method = HTTP_GET, .handler = styles_css_handler, .user_ctx = NULL},
        {.uri = "/api/status", .method = HTTP_GET, .handler = status_handler, .user_ctx = NULL},
        {.uri = "/api/logs", .method = HTTP_GET, .handler = logs_handler, .user_ctx = NULL},
        {.uri = "/api/sensors/read", .method = HTTP_POST, .handler = sensor_read_handler, .user_ctx = NULL},
        {.uri = "/api/calibration", .method = HTTP_POST, .handler = calibration_post_handler, .user_ctx = NULL},
    };

    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); ++i) {
        ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &routes[i]));
    }

    state_log_write(APP_LOG_LEVEL_INFO, "HTTP", "Diagnostics UI started on / and /api/status");
    return ESP_OK;
}
