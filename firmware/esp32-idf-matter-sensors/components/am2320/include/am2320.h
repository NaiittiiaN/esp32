#pragma once

#include <stdbool.h>

#include "driver/i2c.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    i2c_port_t i2c_port;
    uint8_t address;
    bool present;
} am2320_device_t;

esp_err_t am2320_init(am2320_device_t *device, i2c_port_t i2c_port);
esp_err_t am2320_read(am2320_device_t *device, float *temperature_c, float *humidity_pct);

#ifdef __cplusplus
}
#endif
