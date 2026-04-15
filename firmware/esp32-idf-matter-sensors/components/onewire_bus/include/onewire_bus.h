#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gpio_num_t gpio_num;
} onewire_bus_t;

esp_err_t onewire_bus_init(onewire_bus_t *bus, gpio_num_t gpio_num);
bool onewire_bus_reset(onewire_bus_t *bus);
void onewire_bus_write_bit(onewire_bus_t *bus, uint8_t bit);
uint8_t onewire_bus_read_bit(onewire_bus_t *bus);
void onewire_bus_write_byte(onewire_bus_t *bus, uint8_t value);
uint8_t onewire_bus_read_byte(onewire_bus_t *bus);
uint8_t onewire_bus_crc8(const uint8_t *data, uint8_t len);

#ifdef __cplusplus
}
#endif
