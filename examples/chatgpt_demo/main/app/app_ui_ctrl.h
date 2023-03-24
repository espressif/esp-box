/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_CTRL_PANEL_SLEEP = 0,
    UI_CTRL_PANEL_LISTEN,
    UI_CTRL_PANEL_GET,
    UI_CTRL_PANEL_REPLY,
} ui_ctrl_panel_t;

typedef enum {
    UI_CTRL_LABEL_LISTEN_SPEAK = 0,
    UI_CTRL_LABEL_REPLY_QUESTION,
    UI_CTRL_LABEL_REPLY_CONTENT,
} ui_ctrl_label_t;

void ui_ctrl_init(void);

void ui_ctrl_show_panel(ui_ctrl_panel_t panel, uint16_t timeout);

void ui_ctrl_label_show_text(ui_ctrl_label_t label, const char *text);

void ui_sleep_show_animation(void);

void ui_ctrl_reply_set_audio_start_flag(bool result);

bool ui_ctrl_reply_get_audio_start_flag(void);

void ui_ctrl_reply_set_audio_end_flag(bool result);

void ui_ctrl_guide_jump(void);

#ifdef __cplusplus
}
#endif
