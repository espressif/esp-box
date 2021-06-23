/**
 * @file bh1750.h
 * @brief BH1750 driver header file.
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

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"

#include "bsp_i2c.h"
#include "i2c_bus.h"

typedef enum {
    BH1750_CONTINUE_1LX_RES       = 0x10,   /*!< Command to set measure mode as Continuously H-Resolution mode*/
    BH1750_CONTINUE_HALFLX_RES    = 0x11,   /*!< Command to set measure mode as Continuously H-Resolution mode2*/
    BH1750_CONTINUE_4LX_RES       = 0x13,   /*!< Command to set measure mode as Continuously L-Resolution mode*/
    BH1750_ONETIME_1LX_RES        = 0x20,   /*!< Command to set measure mode as One Time H-Resolution mode*/
    BH1750_ONETIME_HALFLX_RES     = 0x21,   /*!< Command to set measure mode as One Time H-Resolution mode2*/
    BH1750_ONETIME_4LX_RES        = 0x23,   /*!< Command to set measure mode as One Time L-Resolution mode*/
} bh1750_cmd_measure_t;

#define BH1750_I2C_ADDRESS_DEFAULT   (0x5C)
typedef void *bh1750_handle_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create and init sensor object and return a sensor handle
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t bh1750_init(void);

/**
 * @brief Set bh1750 as power down mode (low current)
 *
 * @param sensor object handle of bh1750
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t bh1750_power_down(void);

/**
 * @brief Set bh1750 as power on mode
 *
 * @param sensor object handle of bh1750
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t bh1750_power_on(void);

/**
 * @brief Reset data register of bh1750
 *
 * @param sensor object handle of bh1750
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t bh1750_reset_data_register(void);

/**
 * @brief Get light intensity from bh1750
 *
 * @param sensor object handle of bh1750
 * @param cmd_measure the instruction to set measurement mode
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 * @note
 *        You should call this funtion to set measurement mode before call bh1750_get_data() to acquire data.
 *        If you set to onetime mode, you just can get one measurement data.
 *        If you set to continuously mode, you can call bh1750_get_data() to acquire data repeatedly.
 */
esp_err_t bh1750_set_measure_mode(bh1750_cmd_measure_t cmd_measure);

/**
 * @brief Get light intensity from bh1750
 *
 * @param sensor object handle of bh1750
 * @param data light intensity value got from bh1750
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 * @note
 *        You should get data after measurement time over, so take care of measurement time in different mode.
 */
esp_err_t bh1750_get_data(float *data);

/**
 * @brief Get light intensity from bh1750
 *
 * @param sensor object handle of bh1750
 * @param cmd_measure the instruction to set measurement mode
 * @param data light intensity value got from bh1750
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t bh1750_get_light_intensity(bh1750_cmd_measure_t cmd_measure, float *data);

/**
 * @brief Change measurement time
 *
 * @param sensor object handle of bh1750
 * @param measure_time measurement time
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t bh1750_change_measure_time(uint8_t measure_time);

/**
 * @brief Get light intensity from bh1750
 *
 * @param sensor object handle of bh1750
 * @param cmd_measure the instruction to set measurement mode
 * @param data light intensity value got from bh1750
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t bh1750_get_light_intensity(bh1750_cmd_measure_t cmd_measure, float *data);

#ifdef __cplusplus
}
#endif
