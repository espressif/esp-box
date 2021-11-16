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

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start boot animate
 * 
 * @param scr Parent object
 */
void boot_animate_start(lv_obj_t *scr);

/**
 * @brief Start clock update
 * 
 */
void ui_clock_update_start(void);

/**
 * @brief Start main UI
 * 
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t ui_main_start(void);

/**
 * @brief Init status bar
 * 
 * @param color Color of text
 */
void ui_status_bar_init(lv_color_t color);

/**
 * @brief Set time text of status bar
 * 
 * @param text String of current time. Should be static.
 */
void ui_status_bar_set_time(char *text);

/**
 * @brief Show or hide clock page
 * 
 * @param show True if want to show this page. False to hide.
 */
void ui_clock(bool show);

/**
 * @brief Set time text of clock page
 * 
 * @param text String of current time. Should be static.
 */
void ui_clock_set_time(char *text);

/**
 * @brief Show or hide hint
 * 
 * @param show True if want to show hint. False to hide.
 */
void ui_hint(bool show);

/**
 * @brief Show or hide device control page
 * 
 * @param show True if want to show this page. False to hide.
 */
void ui_dev_ctrl(bool show);

/**
 * @brief Show or hide LED control page
 * 
 * @param show True if want to show this page. False to hide.
 */
void ui_led(bool show);

/**
 * @brief This will update LED button state
 * 
 */
void ui_dev_ctrl_update_state(void);

/**
 * @brief Show or hide network config page
 * 
 * @param show True if want to show this page. False to hide.
 */
void ui_network(bool show);

/**
 * @brief Init mute state indicator
 * 
 */
void ui_mute_init(void);

/**
 * @brief Set mute state
 * 
 * @param mute Mute state
 */
void ui_mute_set_state(bool mute);

/**
 * @brief Init speech animate
 * 
 */
void ui_sr_anim_init(void);

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

/**
 * @brief Notify AP connect and disconnect event to UI
 * 
 * @param connect True if AP connected.
 */
void ui_network_set_state(bool connect);

#ifdef __cplusplus
}
#endif
