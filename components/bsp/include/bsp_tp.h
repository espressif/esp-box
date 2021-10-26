/**
 * @file bsp_tp.h
 * @brief 
 * @version 0.1
 * @date 2021-07-05
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
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Init touch panel
 * 
 * @return
 *    - ESP_OK: Success
 *    - ESP_ERR_NOT_SUPPORTED: Unsupported touch panel
 *    - Others: Fail
 */
esp_err_t bsp_tp_init(void);

/**
 * @brief Read data from touch panel
 * 
 * @param tp_num Touch point number
 * @param x X coordinate
 * @param y Y coordinate
 * @param btn_val Button mask value
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_tp_read(uint8_t *tp_num, uint16_t *x, uint16_t *y, uint8_t *btn_val);

#ifdef __cplusplus
}
#endif
