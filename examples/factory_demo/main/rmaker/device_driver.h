/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t app_driver_light_init(char* unique_name, int gpio_r, int gpio_g, int gpio_b, int h, int s, int v, int power, char* voice);
esp_err_t app_driver_set_light_color(char* unique_name, int h, int s, int v);
esp_err_t app_driver_set_light_power(char* unique_name, bool status);
esp_err_t app_driver_set_light_gpio(char* unique_name, int r, int g, int b);
esp_err_t app_driver_set_light_voice(char* unique_name, char* voice);
esp_err_t app_driver_report_light_status(char* unique_name, bool status, int h, int s, int v);

esp_err_t app_driver_fan_init(char* unique_name, int gpio, int power, char* voice);
esp_err_t app_driver_set_fan_power(char* unique_name, bool status);
esp_err_t app_driver_set_fan_gpio(char* unique_name, int gpio, int active_level);
esp_err_t app_driver_set_fan_voice(char* unique_name, char* voice);
esp_err_t app_driver_report_fan_status(char* unique_name, bool status);

esp_err_t app_driver_switch_init(char* unique_name, int gpio, int active_level, int power, char* voice);
esp_err_t app_driver_set_switch_power(char* unique_name, bool status);
esp_err_t app_driver_set_switch_gpio(char* unique_name, int gpio, int active_level);
esp_err_t app_driver_set_switch_voice(char* unique_name, char* voice);
esp_err_t app_driver_report_switch_status(char* unique_name, bool status);
#ifdef __cplusplus
}
#endif
