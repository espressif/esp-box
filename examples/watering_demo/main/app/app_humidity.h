/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*app_humidity_cb_t)(void *args);

esp_err_t app_humidity_init(void);
esp_err_t app_humidity_add_watcher(app_humidity_cb_t cb, void *args);
esp_err_t app_humidity_del_watcher(app_humidity_cb_t cb, void *args);
int  app_humidity_get_value(void);
#ifdef __cplusplus
}
#endif
