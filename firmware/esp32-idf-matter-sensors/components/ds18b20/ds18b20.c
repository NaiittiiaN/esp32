#include "ds18b20.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define DS18B20_CMD_SKIP_ROM 0xCC
#define DS18B20_CMD_CONVERT_T 0x44
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE

static esp_err_t ds18b20_start_conversion(ds18b20_device_t *device) {
    if (!onewire_bus_reset(device->bus)) {
        device->present = false;
        return ESP_ERR_NOT_FOUND;
    }
    onewire_bus_write_byte(device->bus, DS18B20_CMD_SKIP_ROM);
    onewire_bus_write_byte(device->bus, DS18B20_CMD_CONVERT_T);
    return ESP_OK;
}

esp_err_t ds18b20_init(ds18b20_device_t *device, onewire_bus_t *bus) {
    if (device == NULL || bus == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    device->bus = bus;
    device->present = onewire_bus_reset(bus);
    return device->present ? ESP_OK : ESP_ERR_NOT_FOUND;
}

esp_err_t ds18b20_read_temperature_c(ds18b20_device_t *device, float *temperature_c) {
    if (device == NULL || temperature_c == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ds18b20_start_conversion(device);
    if (err != ESP_OK) {
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(750));

    if (!onewire_bus_reset(device->bus)) {
        device->present = false;
        return ESP_ERR_NOT_FOUND;
    }

    onewire_bus_write_byte(device->bus, DS18B20_CMD_SKIP_ROM);
    onewire_bus_write_byte(device->bus, DS18B20_CMD_READ_SCRATCHPAD);

    uint8_t data[9];
    for (int i = 0; i < 9; ++i) {
        data[i] = onewire_bus_read_byte(device->bus);
    }

    if (onewire_bus_crc8(data, 8) != data[8]) {
        return ESP_ERR_INVALID_CRC;
    }

    int16_t raw = (int16_t)((data[1] << 8) | data[0]);
    *temperature_c = raw / 16.0f;
    device->present = true;
    return ESP_OK;
}
