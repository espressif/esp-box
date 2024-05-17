/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

/** Major version number (X.x.x) */
#define MP3_DEMO_VERSION_MAJOR 0
/** Minor version number (x.X.x) */
#define MP3_DEMO_VERSION_MINOR 0
/** Patch version number (x.x.X) */
#define MP3_DEMO_VERSION_PATCH 1

typedef enum {
    AUDIO_PLAYER_I2S = 0,
    AUDIO_PLAYER_USB,
} audio_player_t;

/**
 * @brief get audio player type
 *
 */
audio_player_t get_audio_player_type(void);

/**
 * @brief get audio player handle
 *
 */
uac_host_device_handle_t get_audio_player_handle(void);

/**
 * Macro to convert version number into an integer
 *
 * To be used in comparisons, such as MP3_DEMO_VERSION >= MP3_DEMO_VERSION_VAL(4, 0, 0)
 */
#define MP3_DEMO_VERSION_VAL(major, minor, patch) ((major << 16) | (minor << 8) | (patch))

/**
 * Current version, as an integer
 *
 * To be used in comparisons, such as MP3_DEMO_VERSION >= MP3_DEMO_VERSION_VAL(4, 0, 0)
 */
#define MP3_DEMO_VERSION MP3_DEMO_VERSION_VAL(MP3_DEMO_VERSION_MAJOR, \
                                              MP3_DEMO_VERSION_MINOR, \
                                              MP3_DEMO_VERSION_PATCH)
