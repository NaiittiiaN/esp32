#include "app_config.h"

const app_wifi_credential_t g_app_wifi_credentials[APP_WIFI_NETWORK_COUNT] = {
    {.ssid = "Naiit-TP-Link", .password = "24031993"},
    {.ssid = "Tenda", .password = "24031993"},
};

const char *app_device_name(void) {
    return "ESP32 Matter Sensors";
}

const char *app_mdns_hostname(void) {
    return "esp32-matter-sensors";
}
