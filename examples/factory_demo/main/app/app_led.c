/**
 * @file app_led.c
 * @brief 
 * @version 0.1
 * @date 2021-09-27
 * 
 * @copyright Copyright 2021 Espressif Systems (Shanghai) Co. Ltd.
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include <string.h>
#include "app_led.h"
#include "bsp_board.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/rmt.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "lvgl.h"

#define CONFIG_EXAMPLE_STRIP_LED_NUMBER (8)

static const char *TAG = "app_led";
static led_strip_t *strip = NULL;

static led_state_t s_led_state = {
    .on = false,
    .h = 170,
    .s = 100,
    .v = 30,
    .gpio = GPIO_RMT_LED,
};

esp_err_t app_led_get_state(led_state_t *state)
{
    if (NULL == state) {
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(state, &s_led_state, sizeof(led_state_t));

    return ESP_OK;
}

esp_err_t app_led_init(gpio_num_t io_num)
{
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(io_num, RMT_CHANNEL_0);
    config.clk_div = 2;
    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    led_strip_config_t strip_config = 
        LED_STRIP_DEFAULT_CONFIG(CONFIG_EXAMPLE_STRIP_LED_NUMBER, (led_strip_dev_t)config.channel);
    strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!strip) {
        ESP_LOGE(TAG, "install WS2812 driver failed");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(strip->clear(strip, 100));

    return ESP_OK;
}

esp_err_t app_led_deinit(void)
{
    if (NULL == strip) {
        ESP_LOGE(TAG, "LED not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret_val = led_strip_denit(strip);
    strip = NULL;

    return ret_val;
}

esp_err_t app_led_set_all(uint8_t red, uint8_t green, uint8_t blue)
{
    esp_err_t ret_val = ESP_OK;

    if (NULL == strip) {
        ESP_LOGE(TAG, "LED not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    for (size_t i = 0; i < CONFIG_EXAMPLE_STRIP_LED_NUMBER; i++) {
        ret_val |= strip->set_pixel(strip, i, red, green, blue);
    }

    if ((red == 0) && (green == 0) && (blue == 0)) {
        s_led_state.on = false;
    } else {
        lv_color_hsv_t color_hsv = lv_color_rgb_to_hsv(red, green, blue);
        s_led_state.on = true;
        s_led_state.h = color_hsv.h;
        s_led_state.s = color_hsv.s;
        s_led_state.v = color_hsv.v;
    }

    ret_val |= strip->refresh(strip, 0);

    return ret_val;
}

esp_err_t app_pwm_led_init(void)
{
    esp_err_t ret_val = ESP_OK;

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_8_BIT,
        .freq_hz          = 8192,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ret_val |= ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel_red = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = GPIO_NUM_39,
        .duty           = 0,
        .hpoint         = 0
    };
    ret_val |= ledc_channel_config(&ledc_channel_red);

    ledc_channel_config_t ledc_channel_green = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_1,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = GPIO_NUM_40,
        .duty           = 0,
        .hpoint         = 0
    };
    ret_val |= ledc_channel_config(&ledc_channel_green);

    ledc_channel_config_t ledc_channel_blue = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_2,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = GPIO_NUM_41,
        .duty           = 0,
        .hpoint         = 0
    };
    ret_val |= ledc_channel_config(&ledc_channel_blue);

    return ESP_OK;
}

esp_err_t app_pwm_led_deinit(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t app_pwm_led_set_all(uint8_t red, uint8_t green, uint8_t blue)
{
    esp_err_t ret_val = ESP_OK;

    if ((red == 0) && (green == 0) && (blue == 0)) {
        s_led_state.on = false;
    } else {
        lv_color_hsv_t color_hsv = lv_color_rgb_to_hsv(red, green, blue);
        s_led_state.on = true;
        s_led_state.h = color_hsv.h;
        s_led_state.s = color_hsv.s;
        s_led_state.v = color_hsv.v;
    }

    if (s_led_state.on) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, red);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, green);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, blue);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
    } else {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
    }

    return ret_val;
}

esp_err_t app_pwm_led_set_all_hsv(uint16_t h, uint8_t s, uint8_t v)
{
    esp_err_t ret_val = ESP_OK;

    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;

    if (v == 0) {
        s_led_state.on = false;
    } else {
        s_led_state.h = h;
        s_led_state.s = s;
        s_led_state.v = v;
        s_led_state.on = true;
        lv_color_t color_rgb = lv_color_hsv_to_rgb(h, s, v);
        red = color_rgb.ch.red << 3;
        green = (color_rgb.ch.green_h << 5) + (color_rgb.ch.green_l << 2);
        blue = color_rgb.ch.blue << 3;
    }

    if (s_led_state.on) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, red);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, green);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, blue);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
    } else {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
    }

    return ret_val;
}
