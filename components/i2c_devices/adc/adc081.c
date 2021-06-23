/**
 * @file adc081.c
 * @brief ADC081 driver header file.
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

#include "adc081.h"

#define ADC081_I2C_ADDRESS         (0b1010100)

#define CONVERSION_RESULT_REG       (0b00000000)
#define ALERT_STATUS                (0b00000001)
#define ADC081_CONFIGURATION_REG    (0b00000010)
#define LOW_LIMIT_REG               (0b00000011)
#define HIGH_LIMIT_REG              (0b00000100)
#define HYSTERESIS_REG              (0b00000101)
#define LOWEST_CONVERSION_REG       (0b00000110)
#define HIGHEST_CONVERSION_REG      (0b00000111)

static i2c_bus_device_handle_t adc081_handle;

static esp_err_t adc081_read_byte(uint8_t reg_addr, uint8_t *data)
{
    return i2c_bus_read_byte(adc081_handle, reg_addr, data);
}

static esp_err_t adc081_read_bytes(uint8_t reg_addr, size_t data_len, uint8_t *data)
{
    return i2c_bus_read_bytes(adc081_handle, reg_addr, data_len, data);
}

static esp_err_t adc081_write_byte(uint8_t reg_addr, uint8_t data)
{
    return i2c_bus_write_byte(adc081_handle, reg_addr, data);
}

static esp_err_t adc081_write_bytes(uint8_t reg_addr, size_t data_len, uint8_t *data)
{
    return i2c_bus_write_bytes(adc081_handle, reg_addr, data_len, data);
}

esp_err_t adc081_init(void)
{
    if (NULL != adc081_handle) {
        return ESP_FAIL;
    }

    bsp_i2c_add_device(&adc081_handle, ADC081_I2C_ADDRESS);

    if (NULL == adc081_handle) {
        return ESP_FAIL;
    }

    uint8_t val = 0;

    return ESP_OK;
}

esp_err_t adc081_config(adc081_configuration_reg_t *cfg)
{
    return adc081_write_byte(ADC081_CONFIGURATION_REG, cfg->val);
}

esp_err_t adc081_config_default(void)
{
    adc081_configuration_reg_t cfg = {
        .alert_flag_en = false,
        .alert_hold = false,
        .alert_pin_en = false,
        .cycle_time = adc081_cycle_time_2048x,
        .polarity = 0,
    };

    return adc081_write_byte(ADC081_CONFIGURATION_REG, cfg.val);
}

esp_err_t adc081_get_config(adc081_configuration_reg_t *cfg)
{
    return adc081_read_byte(ADC081_CONFIGURATION_REG, cfg);
}

esp_err_t adc081_get_converted_value(uint8_t *value)
{
    uint8_t reg_val[2];
    adc081_read_bytes(CONVERSION_RESULT_REG, sizeof(uint16_t), reg_val);
    *value = (reg_val[0] << 4) + (reg_val[1] >> 4);
    return ESP_OK;
}
