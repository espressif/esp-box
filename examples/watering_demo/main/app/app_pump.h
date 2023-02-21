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
#define APP_PUMP_WATERING_DEFAULT_TIME   (10)
#define APP_PUMP_WATERING_LOWER_HUMIDITY (40)
#define APP_PUMP_WATERING_AUTO_ENABLE    (1)

enum {
    APP_PUMP_CMD_FROM_BUTTON,
    APP_PUMP_CMD_FROM_SPEECH,
    APP_PUMP_CMD_FROM_CLOUD,
};

typedef void (*app_pump_cb_t)(void *args);

esp_err_t app_pump_init(void);
esp_err_t app_pump_watering_start(void);
esp_err_t app_pump_watering_stop(void);
/**
 * @brief pump callbacks
 *
 * during_cb will be call every 1 second
 */

void app_pump_add_cb_before_watering(app_pump_cb_t cb, void *args);
void app_pump_add_cb_during_watering(app_pump_cb_t cb, void *args);
void app_pump_add_cb_after_watering(app_pump_cb_t cb, void *args);

void app_pump_watering_stop_isr(void);
int app_pump_is_watering(void);
int app_pump_watering_remaining_time(void);
int app_pump_curr_watering_time(void);
int app_pump_get_watering_time(void);
int app_pump_set_watering_time(int max_time);
int app_pump_get_auto_watering_enable(void);
int app_pump_set_auto_watering_enable(int on);
int app_pump_get_lower_humidity(void);
int app_pump_set_lower_humidity(int min);

#ifdef __cplusplus
}
#endif
