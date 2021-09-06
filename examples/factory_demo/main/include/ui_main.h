/**
 * @file ui_main.h
 * @brief 
 * @version 0.1
 * @date 2021-09-19
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

/**
 * @brief Start main UI
 * 
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t ui_main_start(void);

/**
 * @brief Set mute state
 * 
 * @param mute Mute state
 */
void ui_mute_set_state(bool mute);

/**
 * @brief Start speech recognition animate
 * 
 */
void sr_anim_start(void);

/**
 * @brief Stop speech recognition animate
 * 
 */
void sr_anim_stop(void);

/**
 * @brief Set text on speech recognition page
 * 
 * @param text Text to show
 */
void sr_anim_set_text(char *text);

#ifdef __cplusplus
}
#endif
