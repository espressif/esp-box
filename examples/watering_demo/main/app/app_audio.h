/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
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
esp_err_t app_audio_beep_init(void);

/**
 * @brief Start to play beep sound
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 *
 */
esp_err_t app_audio_beep_play_start(void);

/**
 * @brief Return if audio is playing
 *
 * @return true Audio is playing
 * @return false Audio is not playing
 */
bool app_audio_beep_is_playing(void);

#ifdef __cplusplus
}
#endif
