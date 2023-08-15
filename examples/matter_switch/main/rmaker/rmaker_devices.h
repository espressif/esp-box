/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include "esp_log.h"

#define ESP_RMAKER_GPIO_R_NAME "gpio_r"
#define ESP_RMAKER_GPIO_G_NAME "gpio_g"
#define ESP_RMAKER_GPIO_B_NAME "gpio_b"
#define ESP_RMAKER_GPIO_NAME "gpio"
#define ESP_RMAKER_ACTIVE_LEVEL_NAME "active_level"
#define ESP_RMAKER_VOICE_NAME "voice"

#define ESP_RMAKER_PARAM_TYPE_GPIO "esp.param.gpio"
#define ESP_RMAKER_PARAM_TYPE_VOICE "esp.param.voice"
#define ESP_RMAKER_PARAM_TYPE_ACTIVE_LEVEL "esp.param.active_level"

typedef esp_err_t (*set_light_gpio_cb_t)(char *unique_name, int r, int g, int b);
typedef esp_err_t (*set_gpio_cb_t)(char *unique_name, int gpio_num, int active_level);
typedef esp_err_t (*set_light_color_cb_t)(char *unique_name, int hue, int saturation, int brightness);
typedef esp_err_t (*set_power_cb_t)(char *unique_name, bool power);
typedef esp_err_t (*set_voice_cb_t)(char *unique_name, char *voice_cmd);

typedef struct {
    char *unique_name; /* Unchangeable */
    char *name;        /* Allow changes */
    char *voice_cmd;
    int gpio_r;
    int gpio_g;
    int gpio_b;
    bool power;
    int brightness;
    int hue;
    int saturation;
} esp_box_light_param_t;

typedef struct {
    set_light_gpio_cb_t gpio_cb;
    set_light_color_cb_t color_cb;
    set_power_cb_t status_cb;
    set_voice_cb_t voice_cb;
} esp_box_light_cb_t;

typedef struct {
    char *unique_name; /* Unchangeable */
    char *name;        /* Allow changes */
    char *voice_cmd;
    int gpio;
    int active_level;
    bool power;
} esp_box_switch_param_t;

typedef struct {
    set_gpio_cb_t gpio_cb;
    set_power_cb_t status_cb;
    set_voice_cb_t voice_cb;
} esp_box_switch_cb_t;

typedef struct {
    char *unique_name; /* Unchangeable */
    char *name;        /* Allow changes */
    char *voice_cmd;
    int gpio;
    int active_level;
    bool power;
} esp_box_fan_param_t;

typedef struct {
    set_gpio_cb_t gpio_cb;
    set_power_cb_t status_cb;
    set_voice_cb_t voice_cb;
} esp_box_fan_cb_t;

void esp_box_light_device_create(esp_box_light_param_t param_list, esp_box_light_cb_t *cb);

void esp_box_switch_device_create(esp_box_switch_param_t param_list, esp_box_switch_cb_t *cb);

void esp_box_fan_device_create(esp_box_fan_param_t param_list, esp_box_fan_cb_t *cb);

#ifdef __cplusplus
}
#endif
