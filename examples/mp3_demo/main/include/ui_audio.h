/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include "file_iterator.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create audio player UI
 *
 */
void ui_audio_start(file_iterator_instance_t *i);

/**
 * @brief get system volume
 *
 */
uint8_t get_sys_volume();

/**
 * @brief get current music index
 *
 */
uint8_t get_current_music_index();

#ifdef __cplusplus
}
#endif
