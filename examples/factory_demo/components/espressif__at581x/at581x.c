/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "at581x.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"

#include "at581x_reg.h"

const static char *TAG = "AT581X";

/**
 * @brief Radar default setting
 */
#define PWR_40UA_SWITCH             true    /*!< FALSE: 68uA（AT5815:68uA, AT5812:9mA, TRUE: 40uA（AT5815 support） */
#define DEF_SELF_CHECK_TIME         2000    /*!< Power-on self-test time, range: 0~65536 ms */
#define DEF_PROTECT_TIME            1000    /*!< protection time, recommended 1000ms */
#define DEF_TRIGGER_BASE_TIME       500     /*!< Default: 500ms */
#define DEF_TRIGGER_KEEP_TIME       1500    /*!< Total trigger time = TRIGGER_BASE_TIME + DEF_TRIGGER_KEEP_TIME, minimum: 1 */
#define DEF_DELTA                   200     /*!< Delta value: 0~1023, the larger the value, the shorter the distance */
#define DEF_GAIN                    AT581X_STAGE_GAIN_3 /*!< 0x7B, within 2 meters */

/**
 * @brief Light sensor default setting (no light sensor by default)

 */
#define LIGHT_SENSOR_STATUS         false   /*!< FALSE: turn off, true: turn on */
#define LIGHT_SENSOR_VALUE_LOW      500     /*!< light sensor threshold, 0~1023, 10bit */
#define LIGHT_SENSOR_VALUE_HIGH     530     /*!< light sensor threshold, 0~1023, 10bit */
#define LIGHT_SENSOR_INIVERSE       0       /*!< 0 default, don't change */

typedef struct {
    i2c_port_t  i2c_port;
    uint8_t     i2c_addr;
} at581x_dev_t;

static inline esp_err_t at581x_write_reg(at581x_dev_handle_t dev, uint8_t reg_addr, uint8_t data)
{
    at581x_dev_t *sens = (at581x_dev_t *) dev;

    const uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_write_to_device(sens->i2c_port, (sens->i2c_addr) >> 1, write_buf, sizeof(write_buf), pdMS_TO_TICKS(1000));
}

static inline esp_err_t at581x_read_reg(at581x_dev_handle_t dev, uint8_t *data, size_t len)
{
    at581x_dev_t *sens = (at581x_dev_t *) dev;
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

esp_err_t at581x_soft_reset(at581x_dev_handle_t *handle)
{
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x00, 0x00), TAG, "I2C read/write error");
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x00, 0x01), TAG, "I2C read/write error");

    return ESP_OK;
}

esp_err_t at581x_set_freq_point(at581x_dev_handle_t *handle, uint8_t freq_0x5f, uint8_t freq_0x60)
{
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x61, 0xC2), TAG, "I2C read/write error");
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x5F, freq_0x5f), TAG, "I2C read/write error");
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x60, freq_0x60), TAG, "I2C read/write error");

    return ESP_OK;
}

esp_err_t at581x_set_self_check_time(at581x_dev_handle_t *handle, uint32_t self_check_time)
{
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x38, (uint8_t)(self_check_time)), TAG, "I2C read/write error");
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x39, (uint8_t)(self_check_time >> 8)), TAG, "I2C read/write error");

    return ESP_OK;
}

esp_err_t at581x_set_trigger_base_time(at581x_dev_handle_t *handle, uint64_t base_time)
{
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x3d, (uint8_t)(base_time)), TAG, "I2C read/write error");
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x3e, (uint8_t)(base_time >> 8)), TAG, "I2C read/write error");
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x3f, (uint8_t)(base_time >> 16)), TAG, "I2C read/write error");
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x40, (uint8_t)(base_time >> 24)), TAG, "I2C read/write error");

    return ESP_OK;
}

esp_err_t at581x_set_protect_time(at581x_dev_handle_t *handle, uint32_t protect_time)
{
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x4e, (uint8_t)(protect_time)), TAG, "I2C read/write error");
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x4f, (uint8_t)(protect_time >> 8)), TAG, "I2C read/write error");

    return ESP_OK;
}

esp_err_t at581x_set_distance(at581x_dev_handle_t *handle, bool pwr_40uA_switch, uint32_t delta, at581x_gain_t gain)
{
    if (pwr_40uA_switch) { // 40uA
        ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x68, (0x48 & 0xc7) | 0x38), TAG, "I2C read/write error");
        ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x67, (0xf3 & 0xf0) | 0x08), TAG, "I2C read/write error");
    } else { // 70uA
        ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x68, 0x48), TAG, "I2C read/write error");
        ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x67, 0xf3), TAG, "I2C read/write error");
    }
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x10, (uint8_t)(delta)), TAG, "I2C read/write error");
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x11, (uint8_t)(delta >> 8)), TAG, "I2C read/write error");

    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x5C, (0x0B | (gain << 4))), TAG, "I2C read/write error");

    return ESP_OK;
}

esp_err_t at581x_set_trigger_keep_time(at581x_dev_handle_t *handle, uint64_t keep_time)
{
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x41, 0x01), TAG, "I2C read/write error");
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x42, (uint8_t)(keep_time)), TAG, "I2C read/write error");
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x43, (uint8_t)(keep_time >> 8)), TAG, "I2C read/write error");
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x44, (uint8_t)(keep_time >> 16)), TAG, "I2C read/write error");
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x45, (uint8_t)(keep_time >> 24)), TAG, "I2C read/write error");

    return ESP_OK;
}

esp_err_t at581x_set_light_sensor_threshold(at581x_dev_handle_t *handle,
                                     bool onoff,
                                     uint32_t light_sensor_value_high,
                                     uint32_t light_sensor_value_low,
                                     uint32_t light_sensor_iniverse)
{
    if (onoff) {
        ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x66, 0x42), TAG, "I2C read/write error");
    } else {
        ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x66, 0x4a), TAG, "I2C read/write error");
    }

    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x34, (uint8_t)(light_sensor_value_low)), TAG, "I2C read/write error");
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x35, (uint8_t)(light_sensor_value_high)), TAG, "I2C read/write error");
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x36, (uint8_t)((light_sensor_value_low >> 8) | ((light_sensor_value_high >> 8) << 2) | (light_sensor_iniverse << 4))), TAG, "I2C read/write error");

    return ESP_OK;
}

esp_err_t at581x_set_rf_onoff(at581x_dev_handle_t *handle, bool onoff)
{
    if (onoff) {
        ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x5d, 0x45), TAG, "I2C read/write error");
        ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x62, 0x55), TAG, "I2C read/write error");
        ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x51, 0xa0), TAG, "I2C read/write error");
    } else {
        ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x5d, 0x46), TAG, "I2C read/write error");
        ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x62, 0xaa), TAG, "I2C read/write error");
        ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x51, 0x50), TAG, "I2C read/write error");
    }

    return ESP_OK;
}

esp_err_t set_detect_window(at581x_dev_handle_t *handle, uint8_t window_length, uint8_t window_threshold)
{
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x31, window_length), TAG, "I2C read/write error");
    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x32, window_threshold), TAG, "I2C read/write error");

    return ESP_OK;
}

esp_err_t at581x_init_sensor(at581x_dev_handle_t *handle)
{
    esp_err_t ret = ESP_OK;

    ret |= at581x_set_freq_point(handle, FREQ_0X5F_5869MHZ, FREQ_0X60_5869MHZ);
    ret |= at581x_set_distance(handle, PWR_40UA_SWITCH, DEF_DELTA, DEF_GAIN);
    ret |= at581x_set_trigger_base_time(handle, DEF_TRIGGER_BASE_TIME);
    ret |= at581x_set_trigger_keep_time(handle, DEF_TRIGGER_KEEP_TIME);
    ret |= at581x_set_self_check_time(handle, DEF_SELF_CHECK_TIME);
    ret |= at581x_set_protect_time(handle, DEF_PROTECT_TIME);

    // at581x_set_light_sensor_threshold(LIGHT_SENSOR_STATUS, LIGHT_SENSOR_VALUE_LOW, LIGHT_SENSOR_VALUE_HIGH, LIGHT_SENSOR_INIVERSE);

    ESP_RETURN_ON_ERROR(at581x_write_reg(handle, 0x55, 0x04), TAG, "I2C read/write error");

    at581x_soft_reset(handle);

    return ret;
}

esp_err_t at581x_new_sensor(const at581x_i2c_config_t *i2c_conf, at581x_dev_handle_t *handle_out)
{
    ESP_LOGI(TAG, "%-15s: %d.%d.%d", CHIP_NAME, AT581X_VER_MAJOR, AT581X_VER_MINOR, AT581X_VER_PATCH);

    ESP_RETURN_ON_FALSE(i2c_conf, ESP_ERR_INVALID_ARG, TAG, "invalid device config pointer");
    ESP_RETURN_ON_FALSE(handle_out, ESP_ERR_INVALID_ARG, TAG, "invalid device handle pointer");

    at581x_dev_t *handle = calloc(1, sizeof(at581x_dev_t));
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_NO_MEM, TAG, "memory allocation for device handler failed");

    handle->i2c_port = i2c_conf->i2c_port;
    handle->i2c_addr = i2c_conf->i2c_addr;

    *handle_out = handle;
    return ESP_OK;
}

esp_err_t at581x_del_sensor(at581x_dev_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid device handle pointer");

    free(handle);

    return ESP_OK;
}
