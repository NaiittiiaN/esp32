#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APP_LOG_LEVEL_INFO = 0,
    APP_LOG_LEVEL_WARN,
    APP_LOG_LEVEL_ERROR,
} app_log_level_t;

typedef struct {
    uint64_t timestamp_ms;
    app_log_level_t level;
    char tag[16];
    char message[160];
} app_log_entry_t;

void state_log_init(void);
void state_log_write(app_log_level_t level, const char *tag, const char *fmt, ...);
size_t state_log_snapshot(app_log_entry_t *entries, size_t max_entries);
const char *state_log_level_name(app_log_level_t level);

#ifdef __cplusplus
}
#endif
