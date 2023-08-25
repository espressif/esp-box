/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once


/* Default screen brightness */
#define APP_DISP_DEFAULT_BRIGHTNESS  (50)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Add and show LVGL objects on display
 */
void app_disp_lvgl_show(void);

/**
 * @brief Initialize SPI Flash File System and show list of files on display
 */
void app_disp_fs_init(void);

/**
 * @brief Initialize audio
 */
void app_audio_init(void);

#ifdef __cplusplus
}
#endif
