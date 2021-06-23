/**
 * @file indev_mouse.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2021-08-25
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

#ifdef __cplusplus
extern "C" {
#endif

typedef strcuct {
    uint16_t x_range;
    uint16_t y_range;
} indev_mouse_config_t;

typedef struct {
    int16_t dx;
    int16_t dy;
    int16_t dz;
    int16_t key;
} indev_mouse_report_data_t;

typedef struct {
    uint16_t x;
    uint16_t y;
    int16_t z;
    int16_t key;
} indev_mouse_satatus_data_t;

/**
 * @brief 
 * 
 * @return esp_err_t 
 */
esp_err_t indev_mouse_init(void);

/**
 * @brief 
 * 
 * @param x_range 
 * @param y_range 
 * @return esp_err_t 
 */
esp_err_t indev_mouse_set_range(uint16_t x_range, uint16_t y_range);

/**
 * @brief 
 * 
 * @param x_range 
 * @param y_range 
 * @return esp_err_t 
 */
esp_err_t indev_mouse_get_range(uint16_t *x_range, uint16_t *y_range);

/**
 * @brief 
 * 
 * @param data 
 * @return esp_err_t 
 */
esp_err_t indev_mouse_report_data(indev_mouse_report_data_t *data);

/**
 * @brief 
 * 
 * @param data 
 * @return esp_err_t 
 */
esp_err_t indev_mouse_get_status_data(indev_mouse_satatus_data_t *data);

#ifdef __cplusplus
}
#endif
