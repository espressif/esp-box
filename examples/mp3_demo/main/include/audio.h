/**
 * @file audio.h
 * @brief 
 * @version 0.1
 * @date 2021-11-11
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

#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AUDIO_EVENT_NONE = 0,
    AUDIO_EVENT_PAUSE,
    AUDIO_EVENT_PLAY,
    AUDIO_EVENT_CHANGE,
    AUDIO_EVENT_FILE_SCAN_DONE,
    AUDIO_EVENT_MAX,
} audio_event_t;

typedef struct {
    audio_event_t audio_event;
    void *user_ctx;
} audio_cb_ctx_t;

/* Audio callback function type */
typedef void (*audio_cb_t)(audio_cb_ctx_t *);

/**
 * @brief Start MP3 player
 * 
 * @param file_path Folder containing MP3 file(s) 
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t mp3_player_start(char *file_path);

/**
 * @brief Play song
 * 
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t audio_play(void);

/**
 * @brief Pause the song
 * 
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t audio_pause(void);

/**
 * @brief Play next song
 * 
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t audio_play_next(void);

/**
 * @brief Play previous song
 * 
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t audio_play_prev(void);

/**
 * @brief Play the specified song
 * 
 * @param index Index of audio file
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t audio_play_index(size_t index);

/**
 * @brief Get the index of the audio being played
 * 
 * @return Index of audio being played 
 */
size_t audio_get_index(void);

/**
 * @brief Get file name of given index
 * 
 * @param index Index of audio file
 * @param base_path Base path add before file name. No base path added if NULL.
 * @return Name of audio file with given index. NULL if not exist.
 */
char *audio_get_name_from_index(size_t index, char *base_path);

/**
 * @brief Register callback for audio event
 * 
 * @param call_back Call back function
 * @param user_ctx User context
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t audio_callback_register(audio_cb_t call_back, void *user_ctx);

#ifdef __cplusplus
}
#endif
