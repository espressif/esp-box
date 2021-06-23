/**
 * @file app_sr.h
 * @brief 
 * @version 0.1
 * @date 2021-08-10
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
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SR_EVENT_WAKE = 0,
    SR_EVENT_WORD_DETECT,
} sr_event_flag_t;

/**
 * @brief 
 * 
 * @return esp_err_t 
 */
esp_err_t app_sr_start(void);

/**
 * @brief 
 * 
 * @param event_flag 
 * @return esp_err_t 
 */
esp_err_t app_sr_set_event_flag(sr_event_flag_t event_flag);

/**
 * @brief 
 * 
 * @param event_flag 
 * @return esp_err_t 
 */
esp_err_t app_sr_clear_event_flag(sr_event_flag_t event_flag);

/**
 * @brief 
 * 
 * @param event_flag 
 * @return true 
 * @return false 
 */
bool app_sr_get_event_flag(sr_event_flag_t event_flag);

/**
 * @brief 
 * 
 * @return int32_t 
 */
int32_t app_sr_get_command_id(void);

/**
 * @brief 
 * 
 * @param cmd_on 
 * @param cmd_off 
 * @return esp_err_t 
 */
esp_err_t app_sr_reset_multinet(const char *cmd_on, const char *cmd_off);

#ifdef __cplusplus
}
#endif
