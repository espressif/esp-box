/**
 * @file app_audio.h
 * @brief 
 * @version 0.1
 * @date 2021-09-22
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

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start audio playing task
 * 
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t app_audio_start(void);

/**
 * @brief 
 * 
 */
void audio_play_start(void);

/**
 * @brief Return if audio is playing
 * 
 * @return true Audio is playing
 * @return false Audio is not playing
 */
bool audio_is_playing(void);

/**
 * @brief Record audio to vfs device
 * 
 * @param time_ms Total time to record in millisecond
 * @param file_name Full file name with mount point
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t audio_record_to_file(size_t time_ms, const char *file_name);

#ifdef __cplusplus
}
#endif