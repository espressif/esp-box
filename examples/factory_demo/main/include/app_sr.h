/**
 * @file app_sr.h
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

typedef enum {
    SR_EVENT_WAKE_UP = 1 << 0,
    SR_EVENT_WORD_DETECT = 1 << 1,
    SR_EVENT_TIMEOUT = 1 << 2,
    SR_EVENT_ALL =
        SR_EVENT_WAKE_UP | SR_EVENT_WORD_DETECT | SR_EVENT_TIMEOUT,
} sr_event_t;

/**
 * @brief User defined command list
 * 
 */
typedef enum {
    SR_CMD_LIGHT_ON = 18,
    SR_CMD_LIGHT_OFF = 19,
    SR_CMD_SET_RED = 20,
    SR_CMD_SET_GREEN = 21,
    SR_CMD_SET_BLUE = 22,
    SR_CMD_SET_WHITE = 23,
    SR_CMD_SET_MAX = 24,
} sr_cmd_t;

/**
 * @brief Start speech recognition task
 * 
 * @param record_en Record audio to SD crad if set to `true`
 * @return 
 *    - ESP_OK: Success
 *    - ESP_ERR_NO_MEM: No enough memory for speech recognition
 *    - Others: Fail
 */
esp_err_t app_sr_start(bool record_en);

/**
 * @brief Get previous recognized command ID
 * 
 * @return int32_t Command index from 0
 */
int32_t app_sr_get_last_cmd_id(void);

#ifdef __cplusplus
}
#endif
