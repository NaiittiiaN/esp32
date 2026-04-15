#pragma once

#include <stdbool.h>

#include "esp_err.h"
#include "onewire_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    onewire_bus_t *bus;
    bool present;
} ds18b20_device_t;

esp_err_t ds18b20_init(ds18b20_device_t *device, onewire_bus_t *bus);
esp_err_t ds18b20_read_temperature_c(ds18b20_device_t *device, float *temperature_c);

#ifdef __cplusplus
}
#endif
