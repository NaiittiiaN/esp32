#include "am2320.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define AM2320_I2C_ADDRESS 0x5C

static uint16_t am2320_crc16(const uint8_t *buffer, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buffer[pos];
        for (int i = 0; i < 8; i++) {
            if ((crc & 0x0001U) != 0U) {
                crc >>= 1;
                crc ^= 0xA001U;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

esp_err_t am2320_init(am2320_device_t *device, i2c_port_t i2c_port) {
    if (device == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    device->i2c_port = i2c_port;
    device->address = AM2320_I2C_ADDRESS;
    device->present = false;
    return ESP_OK;
}

esp_err_t am2320_read(am2320_device_t *device, float *temperature_c, float *humidity_pct) {
    if (device == NULL || temperature_c == NULL || humidity_pct == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device->address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(device->i2c_port, cmd, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);

    vTaskDelay(pdMS_TO_TICKS(2));

    uint8_t request[3] = {0x03, 0x00, 0x04};
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device->address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, request, sizeof(request), true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(device->i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK) {
        device->present = false;
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(2));

    uint8_t response[8] = {0};
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device->address << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, response, sizeof(response) - 1, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &response[sizeof(response) - 1], I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    err = i2c_master_cmd_begin(device->i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK) {
        device->present = false;
        return err;
    }

    if (response[0] != 0x03 || response[1] != 0x04) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    uint16_t expected_crc = (uint16_t)((response[7] << 8) | response[6]);
    uint16_t actual_crc = am2320_crc16(response, 6);
    if (expected_crc != actual_crc) {
        return ESP_ERR_INVALID_CRC;
    }

    uint16_t raw_humidity = (uint16_t)((response[2] << 8) | response[3]);
    uint16_t raw_temperature = (uint16_t)((response[4] << 8) | response[5]);

    float temp = (raw_temperature & 0x7FFFU) / 10.0f;
    if ((raw_temperature & 0x8000U) != 0U) {
        temp *= -1.0f;
    }

    *humidity_pct = raw_humidity / 10.0f;
    *temperature_c = temp;
    device->present = true;
    return ESP_OK;
}
