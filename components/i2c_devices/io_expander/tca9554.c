/**
 * @file tca9554.c
 * @brief TCA9554 driver.
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

#include "tca9554.h"

#define TCA9554_ADDR                    (0x20)
#define TCA9554_INPUT_PORT_REG          (0x00)
#define TCA9554_OUTPUT_PORT_REG         (0x01)
#define TCA9554_POLARITY_INVERSION_REG  (0x02)
#define TCA9554_CONFIGURATION_REG       (0x03)

static i2c_bus_device_handle_t tca9554_handle = NULL;

static esp_err_t tca9554_read_byte(uint8_t reg_addr, uint8_t *data)
{
    return i2c_bus_read_byte(tca9554_handle, reg_addr, data);
}

static esp_err_t tca9554_write_byte(uint8_t reg_addr, uint8_t data)
{
    return i2c_bus_write_byte(tca9554_handle, reg_addr, data);
}

esp_err_t tca9554_init(void)
{
    if (NULL != tca9554_handle) {
        return ESP_FAIL;
    }

    bsp_i2c_add_device(&tca9554_handle, TCA9554_ADDR);

    if (NULL == tca9554_handle) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t tca9554_set_configuration(uint8_t val)
{
    return tca9554_write_byte(TCA9554_CONFIGURATION_REG, val);
}

esp_err_t tca9554_write_output_pins(uint8_t pin_val)
{
    return tca9554_write_byte(TCA9554_OUTPUT_PORT_REG, pin_val);
}

esp_err_t tca9554_read_output_pins(uint8_t *pin_val)
{
    return tca9554_read_byte(TCA9554_OUTPUT_PORT_REG, pin_val);
}

esp_err_t tca9554_read_input_pins(uint8_t *pin_val)
{
    return tca9554_read_byte(TCA9554_INPUT_PORT_REG, pin_val);
}
