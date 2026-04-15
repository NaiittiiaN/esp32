#include "onewire_bus.h"

#include "esp_rom_sys.h"

static inline void onewire_drive_low(gpio_num_t gpio_num) {
    gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(gpio_num, 0);
}

static inline void onewire_release(gpio_num_t gpio_num) {
    gpio_set_direction(gpio_num, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_level(gpio_num, 1);
}

esp_err_t onewire_bus_init(onewire_bus_t *bus, gpio_num_t gpio_num) {
    if (bus == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    bus->gpio_num = gpio_num;

    gpio_config_t config = {
        .pin_bit_mask = 1ULL << gpio_num,
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&config));
    gpio_set_level(gpio_num, 1);
    return ESP_OK;
}

bool onewire_bus_reset(onewire_bus_t *bus) {
    onewire_drive_low(bus->gpio_num);
    esp_rom_delay_us(480);
    onewire_release(bus->gpio_num);
    esp_rom_delay_us(70);
    int presence = gpio_get_level(bus->gpio_num);
    esp_rom_delay_us(410);
    return presence == 0;
}

void onewire_bus_write_bit(onewire_bus_t *bus, uint8_t bit) {
    onewire_drive_low(bus->gpio_num);
    if (bit) {
        esp_rom_delay_us(6);
        onewire_release(bus->gpio_num);
        esp_rom_delay_us(64);
    } else {
        esp_rom_delay_us(60);
        onewire_release(bus->gpio_num);
        esp_rom_delay_us(10);
    }
}

uint8_t onewire_bus_read_bit(onewire_bus_t *bus) {
    uint8_t bit;
    onewire_drive_low(bus->gpio_num);
    esp_rom_delay_us(6);
    onewire_release(bus->gpio_num);
    esp_rom_delay_us(9);
    bit = (uint8_t)gpio_get_level(bus->gpio_num);
    esp_rom_delay_us(55);
    return bit;
}

void onewire_bus_write_byte(onewire_bus_t *bus, uint8_t value) {
    for (int bit = 0; bit < 8; ++bit) {
        onewire_bus_write_bit(bus, value & 0x01U);
        value >>= 1;
    }
}

uint8_t onewire_bus_read_byte(onewire_bus_t *bus) {
    uint8_t value = 0;
    for (int bit = 0; bit < 8; ++bit) {
        value |= (onewire_bus_read_bit(bus) << bit);
    }
    return value;
}

uint8_t onewire_bus_crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; ++i) {
        uint8_t inbyte = data[i];
        for (uint8_t j = 0; j < 8; ++j) {
            uint8_t mix = (crc ^ inbyte) & 0x01U;
            crc >>= 1;
            if (mix) {
                crc ^= 0x8CU;
            }
            inbyte >>= 1;
        }
    }
    return crc;
}
