/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "touch_element/touch_button.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TOUCH_TAG    "Touch"
#define TOUCH_CHANNEL           TOUCH_PAD_NUM9
#define TOUCH_CHANNEL_SENSOR    0.1F

void touch_sensor_init();

#ifdef __cplusplus
}
#endif
