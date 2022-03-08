/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
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
