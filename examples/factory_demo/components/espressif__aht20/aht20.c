/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "aht20.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"

#include "aht20_reg.h"

const static char *TAG = "AHT20";

typedef struct {
    i2c_port_t  i2c_port;
    uint8_t     i2c_addr;
} aht20_dev_t;

static inline esp_err_t aht20_write_reg(aht20_dev_handle_t dev, uint8_t reg_addr, uint8_t *data, uint8_t len)
{
    aht20_dev_t *sens = (aht20_dev_t *) dev;
    esp_err_t  ret;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ret = i2c_master_start(cmd);
    assert(ESP_OK == ret);
    ret = i2c_master_write_byte(cmd, sens->i2c_addr | I2C_MASTER_WRITE, true);
    assert(ESP_OK == ret);
    ret = i2c_master_write_byte(cmd, reg_addr, true);
    assert(ESP_OK == ret);
    if (len) {
        ret = i2c_master_write(cmd, data, len, true);
        assert(ESP_OK == ret);
    }
    ret = i2c_master_stop(cmd);
    assert(ESP_OK == ret);
    ret = i2c_master_cmd_begin(sens->i2c_port, cmd, 5000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}

static inline esp_err_t aht20_read_reg(aht20_dev_handle_t dev, uint8_t *data, size_t len)
{
    aht20_dev_t *sens = (aht20_dev_t *) dev;
    esp_err_t ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ret = i2c_master_start(cmd);
    assert(ESP_OK == ret);
    ret = i2c_master_write_byte(cmd, sens->i2c_addr | I2C_MASTER_READ, true);
    assert(ESP_OK == ret);
    ret = i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    assert(ESP_OK == ret);
    ret = i2c_master_stop(cmd);
    assert(ESP_OK == ret);
    ret = i2c_master_cmd_begin(sens->i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}

static uint8_t aht20_calc_crc(uint8_t *data, uint8_t len)
{
    uint8_t i;
    uint8_t byte;
    uint8_t crc = 0xFF;

    for (byte = 0; byte < len; byte++) {
        crc ^= data[byte];
        for (i = 8; i > 0; --i) {
            if ((crc & 0x80) != 0) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc = crc << 1;
            }
        }
    }

    return crc;
}

static esp_err_t aht20_reset_reg(aht20_dev_handle_t *handle, uint8_t addr)
{
    uint8_t buf[3];
    uint8_t regs[3];

    buf[0] = 0x00;
    buf[1] = 0x00;
    ESP_RETURN_ON_ERROR(aht20_write_reg(handle, addr, buf, 2), TAG, "I2C read/write error");
    vTaskDelay(pdMS_TO_TICKS(5));

    ESP_RETURN_ON_ERROR(aht20_read_reg(handle, regs, 3), TAG, "I2C read/write error");
    vTaskDelay(pdMS_TO_TICKS(10));

    buf[0] = regs[1];
    buf[1] = regs[2];
    ESP_RETURN_ON_ERROR(aht20_write_reg(handle, (0xB0 | addr), buf, 2), TAG, "I2C read/write error");

    return ESP_OK;
}

esp_err_t aht20_init_sensor(aht20_dev_handle_t *handle)
{
    uint8_t status;

    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid device handle pointer");

    ESP_RETURN_ON_ERROR(aht20_write_reg(handle, 0x71, NULL, 0), TAG, "I2C read/write error");

    ESP_RETURN_ON_ERROR(aht20_read_reg(handle, &status, 1), TAG, "I2C read/write error");
    if ((status & 0x18) != 0x18) {
        ESP_RETURN_ON_ERROR(aht20_reset_reg(handle, 0x1B), TAG, "0x1B reset error");

        ESP_RETURN_ON_ERROR(aht20_reset_reg(handle, 0x1C), TAG, "0x1C reset error");

        ESP_RETURN_ON_ERROR(aht20_reset_reg(handle, 0x1E), TAG, "0x1E reset error");
    }

    return ESP_OK;
}

esp_err_t aht20_read_temperature_humidity(aht20_dev_handle_t handle,
        uint32_t *temperature_raw, float *temperature_s,
        uint32_t *humidity_raw, uint8_t *humidity_s)
{
    uint8_t status;
    uint8_t buf[7];

    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid device handle pointer");

    buf[0] = 0x33;
    buf[1] = 0x00;
    ESP_RETURN_ON_ERROR(aht20_write_reg(handle, AHT20_START_MEASURMENT_CMD, buf, 2), TAG, "I2C read/write error");

    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_RETURN_ON_ERROR(aht20_read_reg(handle, &status, 1), TAG, "I2C read/write error");
    if ((status & 0x80) == 0) {
        ESP_RETURN_ON_ERROR(aht20_read_reg(handle, buf, 7), TAG, "I2C read/write error");

        ESP_RETURN_ON_ERROR((aht20_calc_crc(buf, 6) != buf[6]), TAG, "crc is error");

        *humidity_raw = (((uint32_t)buf[1]) << 16) |
                        (((uint32_t)buf[2]) << 8) |
                        (((uint32_t)buf[3]) << 0);
        *humidity_raw = (*humidity_raw) >> 4;

        *humidity_s = (uint8_t)((float)(*humidity_raw) / 1048576.0f * 100.0f);

        *temperature_raw = (((uint32_t)buf[3]) << 16) |
                           (((uint32_t)buf[4]) << 8) |
                           (((uint32_t)buf[5]) << 0);
        *temperature_raw = (*temperature_raw) & 0xFFFFF;

        *temperature_s = (float)(*temperature_raw) / 1048576.0f * 200.0f - 50.0f;
        return 0;
    } else {
        ESP_LOGI(TAG, "data is not ready");
        return 4;
    }
}

esp_err_t aht20_new_sensor(const aht20_i2c_config_t *i2c_conf, aht20_dev_handle_t *handle_out)
{
    ESP_LOGI(TAG, "%-15s: %d.%d.%d", CHIP_NAME, AHT20_VER_MAJOR, AHT20_VER_MINOR, AHT20_VER_PATCH);
    ESP_LOGI(TAG, "%-15s: %1.1f - %1.1fV", "SUPPLY_VOLTAGE", SUPPLY_VOLTAGE_MIN, SUPPLY_VOLTAGE_MAX);
    ESP_LOGI(TAG, "%-15s: %.2f - %.2f℃", "TEMPERATURE", TEMPERATURE_MIN, TEMPERATURE_MAX);

    ESP_RETURN_ON_FALSE(i2c_conf, ESP_ERR_INVALID_ARG, TAG, "invalid device config pointer");
    ESP_RETURN_ON_FALSE(handle_out, ESP_ERR_INVALID_ARG, TAG, "invalid device handle pointer");

    aht20_dev_t *handle = calloc(1, sizeof(aht20_dev_t));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "memory allocation for device handler failed");

    handle->i2c_port = i2c_conf->i2c_port;
    handle->i2c_addr = i2c_conf->i2c_addr;

    *handle_out = handle;
    return ESP_OK;
}

esp_err_t aht20_del_sensor(aht20_dev_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid device handle pointer");

    free(handle);

    return ESP_OK;
}
