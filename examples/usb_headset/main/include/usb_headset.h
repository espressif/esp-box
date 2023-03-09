/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

#define SAMPLE_RATE     (48000)
#define DEFAULT_VOLUME  (80)
#define CHANNEL         (2)
#define WIDTH           (16)

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