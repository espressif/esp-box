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
 * @brief Start lvgl task handle
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t app_lvgl_start(void);
#ifdef __cplusplus
}
#endif

