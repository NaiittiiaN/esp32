#include "state_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static app_log_entry_t s_entries[APP_LOG_RING_SIZE];
static size_t s_next_index;
static size_t s_entry_count;
static SemaphoreHandle_t s_lock;

void state_log_init(void) {
    if (s_lock == NULL) {
        s_lock = xSemaphoreCreateMutex();
    }
}

const char *state_log_level_name(app_log_level_t level) {
    switch (level) {
        case APP_LOG_LEVEL_INFO:
            return "INFO";
        case APP_LOG_LEVEL_WARN:
            return "WARN";
        case APP_LOG_LEVEL_ERROR:
            return "ERROR";
        default:
            return "UNK";
    }
}

void state_log_write(app_log_level_t level, const char *tag, const char *fmt, ...) {
    if (s_lock == NULL) {
        state_log_init();
    }

    app_log_entry_t entry = {0};
    entry.timestamp_ms = esp_timer_get_time() / 1000ULL;
    entry.level = level;
    snprintf(entry.tag, sizeof(entry.tag), "%s", tag != NULL ? tag : "APP");

    va_list args;
    va_start(args, fmt);
    vsnprintf(entry.message, sizeof(entry.message), fmt, args);
    va_end(args);

    if (xSemaphoreTake(s_lock, pdMS_TO_TICKS(50)) == pdTRUE) {
        s_entries[s_next_index] = entry;
        s_next_index = (s_next_index + 1U) % APP_LOG_RING_SIZE;
        if (s_entry_count < APP_LOG_RING_SIZE) {
            s_entry_count++;
        }
        xSemaphoreGive(s_lock);
    }

    switch (level) {
        case APP_LOG_LEVEL_WARN:
            ESP_LOGW(entry.tag, "%s", entry.message);
            break;
        case APP_LOG_LEVEL_ERROR:
            ESP_LOGE(entry.tag, "%s", entry.message);
            break;
        default:
            ESP_LOGI(entry.tag, "%s", entry.message);
            break;
    }
}

size_t state_log_snapshot(app_log_entry_t *entries, size_t max_entries) {
    if (entries == NULL || max_entries == 0 || s_lock == NULL) {
        return 0;
    }

    size_t copied = 0;
    if (xSemaphoreTake(s_lock, pdMS_TO_TICKS(50)) == pdTRUE) {
        size_t start = (s_entry_count == APP_LOG_RING_SIZE) ? s_next_index : 0;
        for (size_t i = 0; i < s_entry_count && copied < max_entries; ++i) {
            size_t index = (start + i) % APP_LOG_RING_SIZE;
            entries[copied++] = s_entries[index];
        }
        xSemaphoreGive(s_lock);
    }

    return copied;
}
