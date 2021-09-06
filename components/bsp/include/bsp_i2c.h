/**
 * @file bsp_i2c.h
 * @brief 
 * @version 0.1
 * @date 2021-06-25
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

#pragma once

#include "driver/i2c.h"
#include "esp_err.h"
#include "i2c_bus.h"

#ifndef I2C_ACK_CHECK_EN
#define I2C_ACK_CHECK_EN    1
#endif

#ifndef I2C_ACK_CHECK_DIS
#define I2C_ACK_CHECK_DIS   0
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize I2C bus of dev board
 * 
 * @param i2c_num I2C port num
 * @param clk_speed I2C clock speed 
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_i2c_init(i2c_port_t i2c_num, uint32_t clk_speed);

/**
 * @brief Prob given address on I2C bus
 * 
 * @param addr 7 bit Address to prob
 * @return
 *    - ESP_OK: Device found on I2C bus
 *    - ESP_ERR_INVALID_ARG: Invalid I2C address
 *    - ESP_ERR_INVALID_STATE: I2C bus not initialized
 *    - Others: Device not found on I2C bus or error occurred during I2C transmission
 */
esp_err_t bsp_i2c_probe_addr(uint8_t addr);

/**
 * @brief Add device to I2C bus
 * 
 * @param i2c_device_handle Handle of I2C device
 * @param dev_addr 7 bit address of device
 * @return
 *    - ESP_OK Success
 *    - Others: Refer to error code `esp_err.h`.
 */
esp_err_t bsp_i2c_add_device(i2c_bus_device_handle_t *i2c_device_handle, uint8_t dev_addr);

/**
 * @brief Get handle of I2C bus
 * 
 * @return Handle of i2c bus. NULL if not or failed initialized.
 */
i2c_bus_handle_t bsp_i2c_bus_get_handle(void);

#ifdef __cplusplus
}
#endif
