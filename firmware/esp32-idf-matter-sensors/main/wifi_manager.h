#pragma once

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool connected;
    int8_t rssi;
    char ssid[33];
    char ip[16];
} wifi_manager_status_t;

esp_err_t wifi_manager_init(void);
esp_err_t wifi_manager_connect(void);
wifi_manager_status_t wifi_manager_get_status(void);

#ifdef __cplusplus
}
#endif
