/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

#define DEFAULT_SAMPLE_RATE         (48000)
#define DEFAULT_VOLUME              (99)
// Currently the player and recorder should use the same channel and width
#define DEFAULT_RECORDER_CHANNEL    (1)
#define DEFAULT_RECORDER_WIDTH      (16)
#define DEFAULT_PLAYER_CHANNEL      (1)
#define DEFAULT_PLAYER_WIDTH        (16)
#define DEBUG_USB_HEADSET           (0)
#define DEBUG_SYSTEM_VIEW           (0)
/**
 * @brief Initialize the usb headset function
 *
 * @return esp_err_t
 *         ESP_OK   Success
 *         ESP_FAIL Failed
 */
esp_err_t usb_headset_init(void);

#ifdef __cplusplus
}
#endif
