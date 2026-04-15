#include "wifi_manager.h"

#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "lwip/inet.h"
#include "state_log.h"

#define WIFI_CONNECTED_BIT BIT0

static EventGroupHandle_t s_wifi_events;
static wifi_manager_status_t s_status;
static esp_netif_t *s_sta_netif;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    (void)arg;
    (void)event_data;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_status.connected = false;
        s_status.ssid[0] = '\0';
        s_status.ip[0] = '\0';
        state_log_write(APP_LOG_LEVEL_WARN, "WIFI", "Disconnected from Wi-Fi");
        xEventGroupClearBits(s_wifi_events, WIFI_CONNECTED_BIT);
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *event = (const ip_event_got_ip_t *)event_data;
        s_status.connected = true;
        snprintf(s_status.ip, sizeof(s_status.ip), IPSTR, IP2STR(&event->ip_info.ip));
        state_log_write(APP_LOG_LEVEL_INFO, "WIFI", "Got IP %s on SSID %s", s_status.ip, s_status.ssid);
        xEventGroupSetBits(s_wifi_events, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_manager_init(void) {
    s_wifi_events = xEventGroupCreate();
    if (s_wifi_events == NULL) {
        return ESP_ERR_NO_MEM;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    s_sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    memset(&s_status, 0, sizeof(s_status));
    (void)s_sta_netif;
    return ESP_OK;
}

esp_err_t wifi_manager_connect(void) {
    wifi_config_t wifi_config = {0};
    esp_err_t last_err = ESP_FAIL;

    for (int i = 0; i < APP_WIFI_NETWORK_COUNT; ++i) {
        snprintf((char *)wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid), "%s", g_app_wifi_credentials[i].ssid);
        snprintf((char *)wifi_config.sta.password, sizeof(wifi_config.sta.password), "%s", g_app_wifi_credentials[i].password);
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        wifi_config.sta.pmf_cfg.capable = true;
        wifi_config.sta.pmf_cfg.required = false;

        snprintf(s_status.ssid, sizeof(s_status.ssid), "%s", g_app_wifi_credentials[i].ssid);
        state_log_write(APP_LOG_LEVEL_INFO, "WIFI", "Connecting to SSID %s", s_status.ssid);

        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        last_err = esp_wifi_connect();
        if (last_err != ESP_OK) {
            continue;
        }

        EventBits_t bits = xEventGroupWaitBits(s_wifi_events, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE,
                                               pdMS_TO_TICKS(APP_WIFI_RETRY_TIMEOUT_MS));
        if ((bits & WIFI_CONNECTED_BIT) != 0) {
            wifi_ap_record_t record;
            if (esp_wifi_sta_get_ap_info(&record) == ESP_OK) {
                s_status.rssi = record.rssi;
            }
            return ESP_OK;
        }

        esp_wifi_disconnect();
        state_log_write(APP_LOG_LEVEL_WARN, "WIFI", "Failed to connect to SSID %s", s_status.ssid);
    }

    s_status.connected = false;
    return last_err;
}

wifi_manager_status_t wifi_manager_get_status(void) {
    return s_status;
}
