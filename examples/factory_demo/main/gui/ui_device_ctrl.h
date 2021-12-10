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

typedef enum{
    UI_DEV_LIGHT,
    UI_DEV_SWITCH,
    UI_DEV_FAN,
    UI_DEV_AIR,
}ui_dev_type_t;

void ui_device_ctrl_start(void (*fn)(void));
void ui_dev_ctrl_set_state(ui_dev_type_t type, bool state);

#ifdef __cplusplus
}
#endif
