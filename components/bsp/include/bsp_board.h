/**
 * @file bsp_board.h
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

#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Add dev board pin defination and check target.
 * 
 */

#if CONFIG_ESP32_S3_BOX_BOARD
    #include "esp32_s3_box.h"
#elif CONFIG_ESP_CUSTOM_BOARD
    #include "esp_custom_board.h"
#else 
    #error "Please select type of dev board"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Power module of dev board. This can be expanded in the future.
 * 
 */
typedef enum {
    POWER_MODULE_LCD = 1,       /*!< LCD power control */
    POWER_MODULE_AUDIO,         /*!< Audio PA power control */
    POWER_MODULE_ALL = 0xff,    /*!< All module power control */
} power_module_t;

/**
 * @brief Special config for dev board
 * 
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_board_init(void);

/**
 * @brief Control power of dev board
 * 
 * @param module Refer to `power_module_t`
 * @param on Turn on or off specified power module. On if true
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_board_power_ctrl(power_module_t module, bool on);

#ifdef __cplusplus
}
#endif
