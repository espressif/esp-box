/**
 * @file
 * @version 0.1
 *
 * @copyright Copyright 2021 Espressif Systems (Shanghai) Co. Ltd.
 * @copyright Copyright 2022 Chris Morgan <chmorgan@gmail.com>
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

/**
 * Design notes
 *
 * - There is a distinct event for playing -> playing state transitions.
 * COMPLETED_PLAYING_NEXT is helpful for users of the audio player to know
 * the difference between playing and transitioning to another audio file
 * vs. detecting that the audio file transitioned by looking at
 * events indicating IDLE and then PLAYING within a short period of time.
 *
 * State machine diagram
 *
 * cb is the callback function registered with audio_player_callback_register()
 *
 *             cb(PLAYING)                     cb(PLAYING)
 *   _______________________________     ____________________________________
 *   |                             |     |                                  |
 *   |                             |     |                                  |
 *   |         cb(IDLE)            V     V             cb(PAUSE)            |
 * Idle <------------------------  Playing  ----------------------------> Pause
 *   ^                             |_____^                                  |
 *   |                      cb(COMPLETED_PLAYING_NEXT)                      |
 *   |                                                                      |
 *   |______________________________________________________________________|
 *                                cb(IDLE)
 *
 */

#pragma once

#include <stddef.h>
#include <stdio.h>
#include "esp_err.h"
#include "driver/i2s.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AUDIO_PLAYER_STATE_IDLE,
    AUDIO_PLAYER_STATE_PLAYING,
    AUDIO_PLAYER_STATE_PAUSE,
    AUDIO_PLAYER_STATE_SHUTDOWN
} audio_player_state_t;

/**
 * @brief Get the audio player state
 *
 * @return the present audio_player_state_t
 */
audio_player_state_t audio_player_get_state();

typedef enum {
    AUDIO_PLAYER_CALLBACK_EVENT_IDLE, /**< Player is idle, not playing audio */
    AUDIO_PLAYER_CALLBACK_EVENT_COMPLETED_PLAYING_NEXT, /**< Player is playing and playing a new audio file */
    AUDIO_PLAYER_CALLBACK_EVENT_PLAYING, /**< Player is playing */
    AUDIO_PLAYER_CALLBACK_EVENT_PAUSE, /**< Player is pausing */
    AUDIO_PLAYER_CALLBACK_EVENT_SHUTDOWN, /**< Player is shutting down */
    AUDIO_PLAYER_CALLBACK_EVENT_UNKNOWN_FILE_TYPE, /**< File type is unknown */
    AUDIO_PLAYER_CALLBACK_EVENT_UNKNOWN /**< Unknown event */
} audio_player_callback_event_t;

typedef struct {
    audio_player_callback_event_t audio_event;
    void *user_ctx;
} audio_player_cb_ctx_t;

/** Audio callback function type */
typedef void (*audio_player_cb_t)(audio_player_cb_ctx_t *);

/**
 * @brief Play mp3 audio file.
 *
 * Will interrupt a present playback and start the new playback
 * as soon as possible.
 *
 * @param fp - If ESP_OK is returned, will be fclose()ed by the audio system
 *             when the playback has completed or in the event of a playback error.
 *             If not ESP_OK returned then should be fclose()d by the caller.
 * @return
 *    - ESP_OK: Success in queuing play request
 *    - Others: Fail
 */
esp_err_t audio_player_play(FILE *fp);

/**
 * @brief Pause playback
 *
 * @return
 *    - ESP_OK: Success in queuing pause request
 *    - Others: Fail
 */
esp_err_t audio_player_pause(void);

/**
 * @brief Resume playback
 *
 * Has no effect if playback is not in progress
 * @return esp_err_t
 *    - ESP_OK: Success in queuing resume request
 *    - Others: Fail
 */
esp_err_t audio_player_resume(void);

/**
 * @brief Stop playback
 *
 * Has no effect if playback is already stopped
 * @return esp_err_t
 *    - ESP_OK: Success in queuing resume request
 *    - Others: Fail
 */
esp_err_t audio_player_stop(void);

/**
 * @brief Register callback for audio event
 *
 * @param call_back Call back function
 * @param user_ctx User context
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t audio_player_callback_register(audio_player_cb_t call_back, void *user_ctx);

typedef enum {
    AUDIO_PLAYER_MUTE,
    AUDIO_PLAYER_UNMUTE
} AUDIO_PLAYER_MUTE_SETTING;

typedef esp_err_t (*audio_player_mute_fn)(AUDIO_PLAYER_MUTE_SETTING setting);

/**
 * @brief Initialize hardware, allocate memory, create and start audio task.
 * Call before any other 'audio' functions.
 *
 * @param port - The i2s port for output
 * @return esp_err_t
 */
esp_err_t audio_player_new(i2s_port_t port,
                           audio_player_mute_fn mute_fn);

/**
 * @brief Shut down audio task, free allocated memory.
 *
 * @return esp_err_t ESP_OK upon success, ESP_FAIL if unable to shutdown due to retries exhausted
 */
esp_err_t audio_player_delete();

#ifdef __cplusplus
}
#endif
