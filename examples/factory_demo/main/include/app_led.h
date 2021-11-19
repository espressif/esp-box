/**
 * @file app_led.h
 * @brief 
 * @version 0.1
 * @date 2021-09-27
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

#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t h;
    uint8_t s;
    uint8_t v;
    bool    on;
    gpio_num_t gpio;
} led_state_t;

/**
 * @brief Init WS2812 LED(s)
 * 
 * @param io_num WS2812 IO num
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t app_led_init(gpio_num_t io_num);

/**
 * @brief Set color of all LED(s)
 * 
 * @param red   Red part of color
 * @param green Green part of color
 * @param blue  Blue part of color
 * @return 
 *    - ESP_OK: Success
 *    - ESP_ERR_INVALID_STATE: LED(s) not initialized
 *    - Others: Fail
 */
esp_err_t app_led_set_all(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Get state of LED
 * 
 * @param state Pointer to `led_state_t`
 * @return 
 *    - ESP_OK: Success
 *    - ESP_ERR_INVALID_ARGS: Invalid pointer
 */
esp_err_t app_led_get_state(led_state_t *state);

/**
 * @brief Init PWM LED(s)
 * 
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t app_pwm_led_init(void);

/**
 * @brief Deinit PWM LED
 * 
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t app_pwm_led_deinit(void);

/**
 * @brief Set all PWM LED(s) color
 * 
 * @param red   Red part of color
 * @param green Green part of color
 * @param blue  Blue part of color
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t app_pwm_led_set_all(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Set all PWM LED(s) color with HSV
 * 
 * @param h Hue part of color
 * @param s Saturation part of color
 * @param v Brightness part of color
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t app_pwm_led_set_all_hsv(uint16_t h, uint8_t s, uint8_t v);

#ifdef __cplusplus
}
#endif
