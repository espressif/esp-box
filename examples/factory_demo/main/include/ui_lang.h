/**
 * @file ui_lang.h
 * @brief 
 * @version 0.1
 * @date 2021-11-15
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

#include "sdkconfig.h"

#ifdef CONFIG_CN_SPEECH_COMMAND_ID0
#define INDEX_NAME  "/index-zh.html"
#define FONT_CMD    font_cmd_cn_36
#define FONT_HINT   font_hint_16
#define STR_HINT_TITLE  "使用说明"
#define STR_HINT_OK     "确认"
#define STR_HINT_MSG    \
    "1. 请说 “Hi 乐鑫” 以唤醒设备；\n" \
    "2. 等待唤醒词出现在屏幕上；\n" \
    "3. 请说命令词，如 “打开/关闭电灯”。"

#endif

#ifdef CONFIG_EN_SPEECH_COMMAND_ID0
#define INDEX_NAME  "/index-en.html"
#define FONT_CMD    font_cmd_en_36
#define FONT_HINT   font_en_16
#define STR_HINT_TITLE  "Steps for usage"
#define STR_HINT_OK     "OK Let's Go"
#define STR_HINT_MSG    \
    "1: Say \"Hi E. S. P.\" to wake-up the device.\n"   \
    "2: Wait for the \"Hi ESP\" shows on screen.\n"     \
    "3: Say command, like \"turn on the light\"."

#endif
