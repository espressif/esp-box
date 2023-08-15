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

/**
 * @brief
 *
 */
void ui_sr_anim_init(void);

/**
 * @brief
 *
 */
void sr_anim_start(void);

/**
 * @brief
 *
 */
void sr_anim_stop(void);

/**
 * @brief
 *
 * @param text
 */
void sr_anim_set_text(char *text);

#ifdef __cplusplus
}
#endif
