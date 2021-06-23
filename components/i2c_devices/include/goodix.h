/**
 * @file goodix.h
 * @brief 
 * @version 0.1
 * @date 2021-08-26
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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Goodix touch screen gesture bit mask.
 * 
 * @note 1. Gesture data is stored at 0x8059 - 0x805A.
 * @note 2. The gesture mask `GOODIX_GESTURE_DOWN` is LSB of 0x805A.
 * @note 3. The gesture mask `GOODIX_GESTURE_CHAR_C` is LSB of 0x8059.
 * 
 */
typedef enum {
    GOODIX_GESTURE_DOWN = 0,        /*!< LSB of 0x805A */
    GOODIX_GESTURE_DOUBLE_CLICK,
    GOODIX_GESTURE_CHAR_V,
    GOODIX_GESTURE_CHAR_GRATER,     /*!< Char '>' */
    GOODIX_GESTURE_CHAR_CARET,      /*!< Char '^' */
    GOODIX_GESTURE_CHAR_S,
    GOODIX_GESTURE_CHAR_Z,
    GOODIX_GESTURE_SLID_DRIVE,      /*!< MSB of 0x805A */
    /*!< ************ Byte divided here ************ */
    GOODIX_GESTURE_CHAR_C,          /*!< LSB of 0x8059 */
    GOODIX_GESTURE_CHAR_E,
    GOODIX_GESTURE_CHAR_M,
    GOODIX_GESTURE_CHAR_O,
    GOODIX_GESTURE_CHAR_W,
    GOODIX_GESTURE_RIGHT,
    GOODIX_GESTURE_UP,
    GOODIX_GESTURE_LEFT,            /*!< MSB of 0x8059 */
} goodix_tp_gesture_t;

/**
 * @brief 
 * 
 * @return esp_err_t 
 */
esp_err_t goodix_tp_init(void);

/**
 * @brief 
 * 
 * @param x 
 * @param y 
 * @param tp_num 
 * @return esp_err_t 
 */
esp_err_t goodix_tp_read(uint8_t *tp_num, uint16_t *x, uint16_t *y);

/**
 * @brief 
 * 
 * @param gesture 
 * @return esp_err_t 
 */
esp_err_t goodix_tp_read_gesture(goodix_tp_gesture_t *gesture);

#ifdef __cplusplus
}
#endif
