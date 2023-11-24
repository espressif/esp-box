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

/**
 * @brief
 *
 * @param data FFT data
 * @return esp_err_t
 *         ESP_OK   Success
 *         ESP_FAIL Failed
 */
esp_err_t display_draw(float *data);

/**
 * @brief Init lcd
 *
 * @return esp_err_t
 *         ESP_OK   Success
 *         ESP_FAIL Failed
 */
esp_err_t display_lcd_init(void);

#ifdef __cplusplus
}
#endif
