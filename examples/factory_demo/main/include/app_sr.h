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

/**
 * @brief Speech command string
 * 
 */
#define STR_DELIMITER       ";"

#ifdef CONFIG_CN_SPEECH_COMMAND_ID0
#define STR_TIMEOUT         "超时"
#define STR_WAKEWORD        "Hi 乐鑫"
#define STR_LIGHT_RED       "调成红色"
#define STR_LIGHT_GREEN     "调成绿色"
#define STR_LIGHT_BLUE      "调成蓝色"
#define STR_LIGHT_WHITE     "调成白色"
#define STR_LIGHT_ON        "打开电灯"
#define STR_LIGHT_OFF       "关闭电灯"
#define STR_LIGHT_COLOR     "自定义颜色"

#define VOICE_LIGHT_RED     CONFIG_CN_SPEECH_COMMAND_ID0
#define VOICE_LIGHT_GREEN   CONFIG_CN_SPEECH_COMMAND_ID1
#define VOICE_LIGHT_BLUE    CONFIG_CN_SPEECH_COMMAND_ID2
#define VOICE_LIGHT_WHITE   CONFIG_CN_SPEECH_COMMAND_ID3
#define VOICE_LIGHT_ON      CONFIG_CN_SPEECH_COMMAND_ID4
#define VOICE_LIGHT_OFF     CONFIG_CN_SPEECH_COMMAND_ID5
#define VOICE_LIGHT_COLOR   CONFIG_CN_SPEECH_COMMAND_ID6

#endif

#ifdef CONFIG_EN_SPEECH_COMMAND_ID0
#define STR_TIMEOUT         "Timeout"
#define STR_WAKEWORD        "Hi ESP"
#define STR_LIGHT_RED       "Turn Red"
#define STR_LIGHT_GREEN     "Turn Green"
#define STR_LIGHT_BLUE      "Turn Blue"
#define STR_LIGHT_WHITE     "Turn White"
#define STR_LIGHT_ON        "Turn On The Light"
#define STR_LIGHT_OFF       "Turn Off The Light"
#define STR_LIGHT_COLOR     "Customize Color"

#define VOICE_LIGHT_RED     CONFIG_EN_SPEECH_COMMAND_ID0
#define VOICE_LIGHT_GREEN   CONFIG_EN_SPEECH_COMMAND_ID1
#define VOICE_LIGHT_BLUE    CONFIG_EN_SPEECH_COMMAND_ID2
#define VOICE_LIGHT_WHITE   CONFIG_EN_SPEECH_COMMAND_ID3
#define VOICE_LIGHT_ON      CONFIG_EN_SPEECH_COMMAND_ID4
#define VOICE_LIGHT_OFF     CONFIG_EN_SPEECH_COMMAND_ID5
#define VOICE_LIGHT_COLOR   CONFIG_EN_SPEECH_COMMAND_ID6

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
    SR_CMD_SET_RED = 0,
    SR_CMD_SET_GREEN,
    SR_CMD_SET_BLUE,
    SR_CMD_SET_WHITE,
    SR_CMD_LIGHT_ON,
    SR_CMD_LIGHT_OFF,
    SR_CMD_CUSTOM_COLOR,
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

/**
 * @brief Reset command list
 * 
 * @param command_list New command string
 * @return 
 *    - ESP_OK: Success
 *    - ESP_ERR_NO_MEM: No enough memory for err_id string
 *    - Others: Fail
 */
esp_err_t app_sr_reset_command_list(char *command_list);

#ifdef __cplusplus
}
#endif
