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

#define N_SAMPLES 1024

/**
 * @brief write ring buffer
 *
 * @param buf pointer of buffer
 * @param size buffer length
 */
void rb_write(int16_t *buf, size_t size);

/**
 * @brief FFT Convert Init
 *
 * @return esp_err_t
 *         ESP_OK   Success
 *         ESP_FAIL Failed
 */
esp_err_t fft_convert_init(void);


#ifdef __cplusplus
}
#endif
