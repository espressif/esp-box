/**
 * @file tpl0401.c
 * @brief TPL0401 driver.
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

#include "tpl0401.h"

#define TPL0401_DEV_ADDR        (0x2E)
#define TPL0401_RESISTANCE_REG  (0x00)

static i2c_bus_device_handle_t tpl0401_handle = NULL;

static esp_err_t tpl0401_read_byte(uint8_t reg_addr, uint8_t *data)
{
    return i2c_bus_read_byte(tpl0401_handle, reg_addr, data);
}

static esp_err_t tpl0401_write_byte(uint8_t reg_addr, uint8_t data)
{
    return i2c_bus_write_byte(tpl0401_handle, reg_addr, data);
}

esp_err_t tpl0401_init(void)
{
    if (NULL != tpl0401_handle) {
        return ESP_FAIL;
    }

    bsp_i2c_add_device(&tpl0401_handle, TPL0401_DEV_ADDR);

    if (NULL == tpl0401_handle) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t tpl0401_set_resistance(uint8_t res)
{
    return tpl0401_write_byte(TPL0401_RESISTANCE_REG, res);
}
