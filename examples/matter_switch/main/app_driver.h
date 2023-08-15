/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define LIGHT_ENDPOINT_ID 1
#define FAN_ENDPOINT_ID 2
#define SWITCH_ENDPOINT_ID 3

typedef void (*on_change_cb_t)(bool power);

void app_driver_bound_on_off(uint16_t endpoint, bool on);

#ifdef __cplusplus
}
#endif
