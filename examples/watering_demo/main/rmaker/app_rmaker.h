/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief
 *
 */
esp_err_t app_watering_rmaker_start(void);
void app_rainmaker_update_watering_state(void);
const char *app_rainmaker_get_device_name(void);
#ifdef __cplusplus
}
#endif
