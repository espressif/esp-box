/**
 * @file hdc1080.c
 * @brief HDC1080 driver.
 * @version 0.1
 * @date 2021-03-07
 * 
 * @copyright Copyright 2021 Espressif Systems (Shanghai) Co. Ltd.
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "hdc1080.h"

#define HDC1080_DEV_ADDR    (0x40)
#define TEMPERATURE_REG     (0x00)
#define HUMIDITY_REG        (0x01)
#define HDC1080_CONFIGURATION_REG   (0x02)

static i2c_bus_device_handle_t hdc1080_handle = NULL;

static esp_err_t hdc1080_write_byte(uint8_t reg_addr, uint8_t data)
{
    return i2c_bus_write_byte(hdc1080_handle, reg_addr, data);
}

static esp_err_t hdc1080_write_bytes(uint8_t reg_addr, size_t data_len, uint8_t *data)
{
    return i2c_bus_write_bytes(hdc1080_handle, reg_addr, data_len, data);
}

static esp_err_t hdc1080_read_bytes(uint8_t reg_addr, size_t data_len, uint8_t *data)
{
    return i2c_bus_read_bytes(hdc1080_handle, reg_addr, data_len, data);
}

esp_err_t hdc1080_init(void)
{
    if (NULL != hdc1080_handle) {
        return ESP_FAIL;
    }

    bsp_i2c_add_device(&hdc1080_handle, HDC1080_DEV_ADDR);

    hdc1080_set_mode();

    if (NULL == hdc1080_handle) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t hdc1080_reset(void)
{
    hdc1080_config_reg_t hdc1080_config = {
        .soft_reset = true,
        .heater = false,
        .acq_mode = hdc1080_acq_mode_both,
        .bat_status = 0,
        .temp_res = hdc1080_temp_res_11bit,
        .humid_res = hdc1080_humid_res_11bit,
    };

    return hdc1080_write_bytes(HDC1080_CONFIGURATION_REG, 2, &hdc1080_config.val);
}

esp_err_t hdc1080_set_mode(void)
{
    hdc1080_config_reg_t hdc1080_config = {
        .soft_reset = false,
        .heater = false,
        .acq_mode = hdc1080_acq_mode_both,
        .bat_status = 0,
        .temp_res = hdc1080_temp_res_11bit,
        .humid_res = hdc1080_humid_res_11bit,
    };

	return hdc1080_write_bytes(HDC1080_CONFIGURATION_REG, 2, &hdc1080_config.val);
}

esp_err_t hdc1080_get_config(hdc1080_config_reg_t config)
{
    return hdc1080_read_bytes(HDC1080_CONFIGURATION_REG, 2, &config);
}

esp_err_t hdc1080_start_measure(hdc1080_measure_type_t measure_type)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (HDC1080_DEV_ADDR << 1) | I2C_MASTER_WRITE, I2C_ACK_CHECK_EN);
    i2c_master_write_byte(cmd, (uint8_t) measure_type, I2C_ACK_CHECK_EN);

    i2c_bus_cmd_begin(hdc1080_handle, cmd);
    i2c_cmd_link_delete(cmd);

    return ESP_OK;
}

esp_err_t hdc1080_get_measure_data(uint8_t *data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (HDC1080_DEV_ADDR << 1) | I2C_MASTER_READ, I2C_ACK_CHECK_EN);
    i2c_master_read(cmd, data, 2, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    i2c_bus_cmd_begin(hdc1080_handle, cmd);
    i2c_cmd_link_delete(cmd);

    return ESP_OK;
}