/**
 * @file bsp_i2s.h
 * @brief 
 * @version 0.1
 * @date 2021-08-02
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

#include "esp_err.h"
#include "driver/i2s.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 
 * 
 * @param i2s_num 
 * @param sample_rate 
 * @return esp_err_t 
 */
esp_err_t bsp_i2s_init(i2s_port_t i2s_num, uint32_t sample_rate);

/**
 * @brief 
 * 
 * @param i2s_num 
 * @return esp_err_t 
 */
esp_err_t bsp_i2s_deinit(i2s_port_t i2s_num);

#ifdef __cplusplus
}
#endif
