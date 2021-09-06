/**
 * @file ft5x06.h
 * @brief FT5x06 driver header file.
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

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ft5x06_gesture_none         = 0x00,
    ft5x06_gesture_move_up      = 0x10,
    ft5x06_gesture_move_left    = 0x14,
    ft5x06_gesture_move_down    = 0x18,
    ft5x06_gesture_move_right   = 0x1c,
    ft5x06_gesture_zoom_in      = 0x48,
    ft5x06_gesture_zoom_out     = 0x49,
} ft5x06_gesture_t;

/**
 * @brief Init FT5x06 series touch panel
 * 
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t ft5x06_init(void);

/**
 * @brief Read touch point from FT5x06
 * 
 * @param touch_points_num Touch point number
 * @param x X coordinate
 * @param y Y coordinate
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t ft5x06_read_pos(uint8_t *touch_points_num, uint16_t *x, uint16_t *y);

/**
 * @brief Read guesture from FT5x06
 * 
 * @param gesture Gesture read from FT5x06
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t fx5x06_read_gesture(ft5x06_gesture_t *gesture);

#ifdef __cplusplus
}
#endif
