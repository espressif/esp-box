/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "app_led.h"
#include "bsp_board.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/rmt.h"
#include "led_strip.h"
#include "ui_main.h"
#include "ui_device_ctrl.h"

static const char *TAG = "app_led";

typedef struct {
    uint16_t h;
    uint8_t s;
    uint8_t v;
} hsv_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    bool    on;
} led_state_t;

static led_state_t g_led_state = {
    .on = false,
    .r = 180,
    .g = 180,
    .b = 180,
};

#define LEDPWM_CNT_TOP 256
static uint8_t g_gamma_table[LEDPWM_CNT_TOP + 1];
static hsv_t g_customize_color = {0};
static gpio_num_t g_last_led_io[3] = {GPIO_NUM_NC};
static bool g_initialized = 0;

/**
 * @brief Simple helper function, converting HSV color space to RGB color space
 *
 * Wiki: https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 */
static void led_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    h %= 360; /**< h -> [0,360] */
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    /**< RGB adjustment amount by hue */
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;

    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;

    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;

    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;

    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;

    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

void led_rgb2hsv(uint8_t r, uint8_t g, uint8_t b, uint16_t *h, uint8_t *s, uint8_t *v)
{
    int32_t R = r;
    int32_t G = g;
    int32_t B = b;
    int32_t min, max, delta, tmp;
    tmp = R > G ? G : R;
    min = tmp > B ? B : tmp;
    tmp = R > G ? R : G;
    max = tmp > B ? tmp : B;
    *v = 100 * max / 255; /**< v */
    delta = max - min;

    if (max != 0) {
        *s = 100 * delta / max; /**< s */
    } else {
        /**< r = g = b = 0 */
        *s = 0;
        *h = 0;
        return;
    }

    float h_temp = 0;

    if (delta == 0) {
        *h = 0;
        return;
    } else if (R == max) {
        h_temp = ((float)(G - B) / (float)delta);           /**< between yellow & magenta */
    } else if (G == max) {
        h_temp = 2.0f + ((float)(B - R) / (float)delta);    /**< between cyan & yellow */
    } else if (B == max) {
        h_temp = 4.0f + ((float)(R - G) / (float)delta);    /**< between magenta & cyan */
    }

    h_temp *= 60.0f;

    if (h_temp < 0.0f) {
        h_temp += 360;
    }

    *h = (uint32_t)h_temp; /**< degrees */
}

esp_err_t app_pwm_led_init(gpio_num_t gpio_r, gpio_num_t gpio_g, gpio_num_t gpio_b)
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
        .gpio_num       = gpio_r,
        .duty           = 0,
        .hpoint         = 0
    };
    ret_val |= ledc_channel_config(&ledc_channel_red);

    ledc_channel_config_t ledc_channel_green = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_1,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = gpio_g,
        .duty           = 0,
        .hpoint         = 0
    };
    ret_val |= ledc_channel_config(&ledc_channel_green);

    ledc_channel_config_t ledc_channel_blue = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_2,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = gpio_b,
        .duty           = 0,
        .hpoint         = 0
    };
    ret_val |= ledc_channel_config(&ledc_channel_blue);
    g_initialized = 1;
    // Generate gamma correction table
    for (int i = 0; i < (LEDPWM_CNT_TOP + 1); i++) {
        g_gamma_table[i] = (int)roundf(powf((float)i / (float)LEDPWM_CNT_TOP, 1.0 / 0.45) * (float)LEDPWM_CNT_TOP);
    }

    g_customize_color.h = 0;
    g_customize_color.s = 100;
    g_customize_color.v = 100;

    g_last_led_io[0] = gpio_r;
    g_last_led_io[1] = gpio_g;
    g_last_led_io[2] = gpio_b;
    return ESP_OK;
}

esp_err_t app_pwm_led_change_io(gpio_num_t gpio_r, gpio_num_t gpio_g, gpio_num_t gpio_b)
{
    ESP_RETURN_ON_FALSE(g_initialized, ESP_ERR_INVALID_STATE, TAG, "pwm led is not running");
    if (g_initialized) {
        ESP_LOGI(TAG, "io set to %d,%d,%d; before: %d,%d,%d",
                 gpio_r, gpio_g, gpio_b,
                 g_last_led_io[0],
                 g_last_led_io[1],
                 g_last_led_io[2]);
        gpio_set_direction(g_last_led_io[0], GPIO_MODE_INPUT);
        gpio_set_direction(g_last_led_io[1], GPIO_MODE_INPUT);
        gpio_set_direction(g_last_led_io[2], GPIO_MODE_INPUT);
    }
    app_pwm_led_init(gpio_r, gpio_g, gpio_b);
    return ESP_OK;
}

esp_err_t app_pwm_led_deinit(void)
{
    ESP_LOGW(TAG, "app_pwm_led_deinit() ESP_ERR_NOT_SUPPORTED");
    return ESP_ERR_NOT_SUPPORTED;
}

static void update_pwm_led(uint8_t r, uint8_t g, uint8_t b)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, r);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, g);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, b);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
}

esp_err_t app_pwm_led_set_all(uint8_t red, uint8_t green, uint8_t blue)
{
    esp_err_t ret_val = ESP_OK;

    g_led_state.r = red;
    g_led_state.g = green;
    g_led_state.b = blue;
    app_pwm_led_set_power(true);

    return ret_val;
}

esp_err_t app_pwm_led_set_all_hsv(uint16_t h, uint8_t s, uint8_t v)
{
    esp_err_t ret_val = ESP_OK;

    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    led_hsv2rgb(h, s, v, &red, &green, &blue);
    g_led_state.r = red;
    g_led_state.g = green;
    g_led_state.b = blue;
    app_pwm_led_set_power(true);

    return ret_val;
}

esp_err_t app_pwm_led_set_power(bool power)
{
    esp_err_t ret_val = ESP_OK;
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;

    if (power) {
        g_led_state.on = true;
        ui_dev_ctrl_set_state(UI_DEV_LIGHT, 1);
    } else {
        g_led_state.on = false;
        update_pwm_led(0, 0, 0);
        ui_dev_ctrl_set_state(UI_DEV_LIGHT, 0);
        return ret_val;
    }
    red = g_gamma_table[g_led_state.r];
    green = g_gamma_table[g_led_state.g];
    blue = g_gamma_table[g_led_state.b];
    update_pwm_led(red, green, blue);
    return ret_val;
}

bool app_pwm_led_get_state(void)
{
    return g_led_state.on;
}

esp_err_t app_pwm_led_set_customize_color(uint16_t h, uint8_t s, uint8_t v)
{
    ESP_LOGI(TAG, "customize_color: hsv=[%d,%d,%d]", h, s, v);
    g_customize_color.h = h;
    g_customize_color.s = s;
    g_customize_color.v = v;
    return ESP_OK;
}

esp_err_t app_pwm_led_get_customize_color(uint16_t *h, uint8_t *s, uint8_t *v)
{
    *h = g_customize_color.h;
    *s = g_customize_color.s;
    *v = g_customize_color.v;
    return ESP_OK;
}
