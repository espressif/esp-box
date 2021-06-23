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

#if CONFIG_ESP32_S3_HMI_DEVKIT_BOARD
    #include "bsp_board_esp32s3_hmi_devkit_1.h"
#elif CONFIG_ESP32_S3_LCD_CAM_BOARD
    #include "bsp_board_esp32s3_lcd_cam.h"
#elif CONFIG_ESP32_S2_HMI_DEVKIT_BOARD
    #include "bsp_board_esp32s2_hmi_devkit_1.h"
#elif CONFIG_ESP32_S2_KALUGA_BOARD
    #include "bsp_board_esp32s2_kaluga_1.h"
#elif CONFIG_ESP32_WROVER_KIT_BOARD
    #include "bsp_board_esp32_wrover_kit.h"
#elif CONFIG_ESP32_C3_DEVKIT_BOARD
    #include "bsp_board_esp32_c3_devkit.h"
#elif CONFIG_ESP32_S3_CUBE_BOARD
    #include "bsp_board_esp32_s3_cube.h"
#else 
    //CONFIG_ESP_CUSTOM_BOARD
    // #include ""
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Power module of dev board.
 *      This can be expanded in the future use.
 * 
 */
typedef enum {
    POWER_MODULE_LCD = 1,       /*!< LCD power control */
    POWER_MODULE_AUDIO,         /*!< Audio PA power control */
    POWER_MODULE_ALL = 0xff,    /*!< All module power control */
} power_module_t;

typedef struct {
    esp_err_t (*init)(void *);                      /*!< Initialize function of dev board. Skip if NULL. */
    esp_err_t (*pwr_ctrl)(power_module_t, bool);    /*!< Power control of dev board. NULL if not support. */
    esp_err_t (*enter_low_power)(void);             /*!< Enter low power mode. NULL if not support. */
} bsp_board_t;

typedef bsp_board_t *bsp_board_handle_t;    /*!< Handle to board control object */

/**
 * @brief Borad initialize.
 *   It will call dev board's 
 * 
 * @return
 *    - ESP_OK Success
 *    - Others: Refer to error code `esp_err.h`.
 */
esp_err_t bsp_board_init(void);

/**
 * @brief Control power of dev board.
 * 
 * @param module Refer to `power_module_t`.
 * @param on Turn on or off specified power module. On if true.
 * @return
 *    - ESP_OK Success
 *    - Others: Refer to error code `esp_err.h`.
 */
esp_err_t bsp_board_power_ctrl(power_module_t module, bool on);

#ifdef __cplusplus
}
#endif
