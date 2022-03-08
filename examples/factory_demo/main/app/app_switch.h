/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t app_switch_change_io(gpio_num_t gpio, bool act_level);
esp_err_t app_switch_set_power(bool power);
bool app_switch_get_state(void);

#ifdef __cplusplus
}
#endif
