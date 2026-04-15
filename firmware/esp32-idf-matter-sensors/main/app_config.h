#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define APP_WIFI_NETWORK_COUNT 2
#define APP_LOG_RING_SIZE 64

#define APP_DS18B20_GPIO 4
#define APP_I2C_SDA_GPIO 19
#define APP_I2C_SCL_GPIO 21

#define APP_SENSOR_POLL_INTERVAL_MS 10000
#define APP_WIFI_RETRY_TIMEOUT_MS 20000

typedef struct {
    const char *ssid;
    const char *password;
} app_wifi_credential_t;

typedef struct {
    float ds18b20_temp_offset_c;
    float am2320_temp_offset_c;
    float am2320_humidity_offset_pct;
} app_calibration_t;

typedef struct {
    bool ds18b20_present;
    bool am2320_present;
    float ds18b20_temperature_c;
    float am2320_temperature_c;
    float am2320_humidity_pct;
    uint64_t last_update_ms;
} app_sensor_snapshot_t;

extern const app_wifi_credential_t g_app_wifi_credentials[APP_WIFI_NETWORK_COUNT];

const char *app_device_name(void);
const char *app_mdns_hostname(void);

#ifdef __cplusplus
}
#endif
