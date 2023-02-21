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
#define NVS_NAMESPACE_APP_WATERING_CFG "app_cfg"

esp_err_t app_nvs_set_watering_time(int time);
esp_err_t app_nvs_get_watering_time(int *time);

esp_err_t app_nvs_set_lower_humidity(int humidity);
esp_err_t app_nvs_get_lower_humidity(int *humidity);

esp_err_t app_nvs_set_auto_watering_enable(bool on);
esp_err_t app_nvs_get_auto_watering_enable(bool *on);
#ifdef __cplusplus
}
#endif
