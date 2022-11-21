/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_models.h"

#ifdef __cplusplus
extern "C" {
#endif


#define SR_CONTINUE_DET 1
#define SR_RUN_TEST 0 /**< Just for sr experiment in laboratory >*/
#if SR_RUN_TEST
#ifdef SR_CONTINUE_DET
#undef SR_CONTINUE_DET
#define SR_CONTINUE_DET 0
#endif
#endif

/**
 * @brief Speech command string
 *
 */
#define STR_DELIMITER       ";"

#ifdef CONFIG_SR_MN_CHINESE
#define STR_TIMEOUT         "超时"
#define STR_WAKEWORD        "Hi 乐鑫"
#define STR_LIGHT_RED       "调成红色"
#define STR_LIGHT_GREEN     "调成绿色"
#define STR_LIGHT_BLUE      "调成蓝色"
#define STR_LIGHT_WHITE     "调成白色"
#define STR_LIGHT_ON        "打开电灯"
#define STR_LIGHT_OFF       "关闭电灯"
#define STR_LIGHT_COLOR     "自定义颜色"
#define STR_PLAY_MUSIC      "播放音乐"
#define STR_NEXT_MUSIC      "切歌"
#define STR_PAUSE_MUSIC     "暂停播放"

#define VOICE_LIGHT_RED     CONFIG_CN_SPEECH_COMMAND_ID0
#define VOICE_LIGHT_GREEN   CONFIG_CN_SPEECH_COMMAND_ID1
#define VOICE_LIGHT_BLUE    CONFIG_CN_SPEECH_COMMAND_ID2
#define VOICE_LIGHT_WHITE   CONFIG_CN_SPEECH_COMMAND_ID3
#define VOICE_LIGHT_ON      CONFIG_CN_SPEECH_COMMAND_ID4
#define VOICE_LIGHT_OFF     CONFIG_CN_SPEECH_COMMAND_ID5
#define VOICE_LIGHT_COLOR   CONFIG_CN_SPEECH_COMMAND_ID6

#endif

#ifdef CONFIG_SR_MN_ENGLISH
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

typedef struct {
    wakenet_state_t wakenet_mode;
    esp_mn_state_t state;
    int command_id;
} sr_result_t;

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
