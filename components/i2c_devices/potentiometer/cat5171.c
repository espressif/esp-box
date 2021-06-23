/**
 * @file cat5171.c
 * @brief CAT5171 driver.
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

#include "cat5171.h"

#define CAT5171_I2C_ADDRESS (0x2d)

static const char *TAG = "cat5171";

static i2c_bus_device_handle_t cat5171_handle = NULL;

static esp_err_t cat5171_write_byte(uint8_t reg_addr, uint8_t data)
{
    return i2c_bus_write_byte(cat5171_handle, reg_addr, data);
}

esp_err_t cat5171_init(void)
{
    if (NULL != cat5171_handle) {
        return ESP_FAIL;
    }

    bsp_i2c_add_device(&cat5171_handle, CAT5171_I2C_ADDRESS);

    if (NULL == cat5171_handle) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t cat5171_set_resistance(uint8_t res)
{
    return cat5171_write_byte(0x00, res);
}
