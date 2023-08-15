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

esp_err_t app_pwm_led_init(int gpio_r, int gpio_g, int gpio_b);
esp_err_t app_pwm_led_change_io(gpio_num_t gpio_r, gpio_num_t gpio_g, gpio_num_t gpio_b);
esp_err_t app_pwm_led_deinit(void);
esp_err_t app_pwm_led_set_all(uint8_t red, uint8_t green, uint8_t blue);
esp_err_t app_pwm_led_set_all_hsv(uint16_t h, uint8_t s, uint8_t v);
esp_err_t app_pwm_led_set_power(bool power);
bool app_pwm_led_get_state(void);
esp_err_t app_pwm_led_set_customize_color(uint16_t h, uint8_t s, uint8_t v);
esp_err_t app_pwm_led_get_customize_color(uint16_t *h, uint8_t *s, uint8_t *v);

#ifdef __cplusplus
}
#endif
