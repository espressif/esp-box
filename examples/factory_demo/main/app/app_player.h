/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AUDIO_EVENT_NONE = 0,
    AUDIO_EVENT_PAUSE,
    AUDIO_EVENT_PLAY,
    AUDIO_EVENT_NEXT,
    AUDIO_EVENT_PREV,
    AUDIO_EVENT_CHANGE,
    AUDIO_EVENT_FILE_SCAN_DONE,
    AUDIO_EVENT_MAX,
} player_event_t;

typedef enum {
    PLAYER_STATE_IDLE,
    PLAYER_STATE_PLAYING,
    PLAYER_STATE_PAUSE,
} player_state_t;

typedef struct {
    player_event_t audio_event;
    void *user_ctx;
} player_cb_ctx_t;

/* Audio callback function type */
typedef void (*audio_cb_t)(player_cb_ctx_t *);

esp_err_t app_player_start(char *file_path);
esp_err_t app_player_play(void);
esp_err_t app_player_pause(void);
player_state_t app_player_get_state(void);
esp_err_t app_player_play_next(void);
esp_err_t app_player_play_prev(void);
esp_err_t app_player_play_index(size_t index);
size_t app_player_get_index(void);
const char *app_player_get_name_from_index(size_t index);
esp_err_t app_player_callback_register(audio_cb_t call_back, void *user_ctx);

#ifdef __cplusplus
}
#endif
