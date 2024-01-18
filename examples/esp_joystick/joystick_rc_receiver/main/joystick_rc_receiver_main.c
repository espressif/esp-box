/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "espnow.h"
#include "espnow_ctrl.h"
#include "espnow_utils.h"
#include "led_strip.h"

#define LED_STRIP_GPIO         GPIO_NUM_42

#define PWM_TIMER              LEDC_TIMER_0
#define PWM_MODE               LEDC_LOW_SPEED_MODE
#define PWM_INTR_TYPE          LEDC_INTR_DISABLE
#define PWM_CLK_TYPE           LEDC_AUTO_CLK
#define PWM_DUTY_RES           LEDC_TIMER_10_BIT // Set duty resolution to 13 bits
#define PWM_FREQUENCY          (50) // Frequency in Hertz. Set frequency at 5 kHz

/* Lights control pin */
#define LEFT_LED_PIN      8     //Left turn signal control pin
#define RIGHT_LED_PIN     9     //Right turn signal control pin
#define BRAKE_LED_PIN     5     //Brake light control pin
#define REVERSE_LED_PIN   40    //Reverse light control pin

static const char *TAG = "joystick_rc_receiver";

TaskHandle_t light_control_task_handle = NULL;

static int g_left_led_state = 0;
static int g_right_led_state = 0;

typedef enum {
    APP_ESPNOW_CTRL_INIT,
    APP_ESPNOW_CTRL_BOUND,
    APP_ESPNOW_CTRL_MAX
} app_espnow_ctrl_status_t;

static app_espnow_ctrl_status_t s_espnow_ctrl_status = APP_ESPNOW_CTRL_INIT;

static led_strip_handle_t g_strip_handle = NULL;

static void example_pwm_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t pwm_timer = {
        .speed_mode       = PWM_MODE,
        .timer_num        = PWM_TIMER,
        .duty_resolution  = PWM_DUTY_RES,
        .freq_hz          = PWM_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = PWM_CLK_TYPE
    };
    ESP_ERROR_CHECK(ledc_timer_config(&pwm_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t pwm_channel_1 = {
        .speed_mode     = PWM_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = PWM_TIMER,
        .intr_type      = PWM_INTR_TYPE,
        .gpio_num       = 2,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };

    ledc_channel_config_t pwm_channel_2 = {
        .speed_mode     = PWM_MODE,
        .channel        = LEDC_CHANNEL_1,
        .timer_sel      = PWM_TIMER,
        .intr_type      = PWM_INTR_TYPE,
        .gpio_num       = 3,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };

    ledc_channel_config_t pwm_channel_3 = {
        .speed_mode     = PWM_MODE,
        .channel        = LEDC_CHANNEL_2,
        .timer_sel      = PWM_TIMER,
        .intr_type      = PWM_INTR_TYPE,
        .gpio_num       = 4,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };

    ESP_ERROR_CHECK(ledc_channel_config(&pwm_channel_1));
    ESP_ERROR_CHECK(ledc_channel_config(&pwm_channel_2));
    ESP_ERROR_CHECK(ledc_channel_config(&pwm_channel_3));
}

static void light_gpio_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LEFT_LED_PIN) | (1ULL << RIGHT_LED_PIN) | (1ULL << BRAKE_LED_PIN) | (1ULL << REVERSE_LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);
    gpio_set_level(LEFT_LED_PIN, 0);
    gpio_set_level(RIGHT_LED_PIN, 0);
    gpio_set_level(BRAKE_LED_PIN, 0);
    gpio_set_level(REVERSE_LED_PIN, 0);
}

static void channel_pwm_output(ledc_channel_t channel, uint32_t duty)
{
    ESP_ERROR_CHECK(ledc_set_duty(PWM_MODE, channel, duty));
    ESP_ERROR_CHECK(ledc_update_duty(PWM_MODE, channel));
}

static void app_wifi_init()
{
    esp_event_loop_create_default();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void app_led_init(void)
{
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO,
        .max_leds = 1, // at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &g_strip_handle));
    /* Set all LED off to clear all pixels */
    led_strip_clear(g_strip_handle);
}

void app_led_set_color(uint8_t red, uint8_t green, uint8_t blue)
{
    led_strip_set_pixel(g_strip_handle, 0, red, green, blue);
    led_strip_refresh(g_strip_handle);
}

static void remote_control(int ch1_value, int ch2_value, int ch3_value, int ch4_value, int ch5_value, int ch6_value, int ch7_value, int ch8_value)
{
    static int pre_ch1_value = 0;
    static int pre_ch2_value = 0;
    static int pre_ch3_value = 0;
    static int pre_ch4_value = 0;
    static int pre_ch5_value = 0;
    static int pre_ch7_value = 0;
    static int pre_ch8_value = 0;
    if (pre_ch1_value != ch1_value) {
        pre_ch1_value = ch1_value;
    }

    if (pre_ch2_value != ch2_value) {
        float throttle_value = ch2_value - 90.0;
        float ch2_duty = 0.0;
        if (throttle_value >= 0) {
            if (throttle_value > 5) {
                ESP_LOGI(TAG, "Forward.");
            }
            gpio_set_level(REVERSE_LED_PIN, 0);
            ch2_duty = throttle_value * 10.0;
            channel_pwm_output(LEDC_CHANNEL_0, (int)ch2_duty);
            channel_pwm_output(LEDC_CHANNEL_1, 0);
        } else {
            if (throttle_value < -5) {
                gpio_set_level(REVERSE_LED_PIN, 1);
                ESP_LOGI(TAG, "Backward.");
            }
            ch2_duty = -throttle_value * 6.0;
            channel_pwm_output(LEDC_CHANNEL_1, (int)ch2_duty);
            channel_pwm_output(LEDC_CHANNEL_0, 0);
        }

        pre_ch2_value = ch2_value;
    }

    if (pre_ch3_value != ch3_value) {
        float steer_value = (ch3_value + 8) / 2.8;
        float ch3_duty = ((steer_value / 90.0 + 0.5) / 20.0) * 1024;
        channel_pwm_output(LEDC_CHANNEL_2, (int)ch3_duty);
        if (ch3_value > 100) {
            g_right_led_state = 0;
            ESP_LOGI(TAG, "Turn right.");
        } else if (ch3_value < 80) {
            g_left_led_state = 0;
            ESP_LOGI(TAG, "Turn left.");
        }
        pre_ch3_value = ch3_value;
    }

    if (pre_ch4_value != ch4_value) {
        pre_ch4_value = ch4_value;
    }

    if (pre_ch5_value != ch5_value) {
        pre_ch5_value = ch5_value;
    }

    if (ch6_value == 1) {
        ESP_LOGI(TAG, "Braking...");
        gpio_set_level(BRAKE_LED_PIN, 1);
        channel_pwm_output(LEDC_CHANNEL_0, 0);
        channel_pwm_output(LEDC_CHANNEL_1, 0);
    } else {
        gpio_set_level(BRAKE_LED_PIN, 0);
    }

    if (pre_ch7_value != ch7_value) {
        g_left_led_state = !g_left_led_state;
        g_right_led_state = 0;
        pre_ch7_value = ch7_value;
    }

    if (pre_ch8_value != ch8_value) {
        g_left_led_state = 0;
        g_right_led_state = !g_right_led_state;
        pre_ch8_value = ch8_value;
    }
}

static void app_responder_ctrl_data_cb(espnow_attribute_t initiator_attribute,
                                       espnow_attribute_t responder_attribute,
                                       uint32_t status1,
                                       int status2,
                                       int lx_value,
                                       int ly_value,
                                       int rx_value,
                                       int ry_value,
                                       int channel_one_value,
                                       int channel_two_value)
{
    remote_control(lx_value + 90, ly_value + 90, rx_value + 90, ry_value + 90, status1, status2, channel_one_value, channel_two_value);
}

static void app_responder_init(void)
{
    ESP_ERROR_CHECK(espnow_ctrl_responder_bind(60 * 1000, -55, NULL));
    espnow_ctrl_responder_data(app_responder_ctrl_data_cb);
}

static void app_espnow_event_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    if (base != ESP_EVENT_ESPNOW) {
        return;
    }

    switch (id) {
    case ESP_EVENT_ESPNOW_CTRL_BIND: {
        espnow_ctrl_bind_info_t *info = (espnow_ctrl_bind_info_t *)event_data;
        ESP_LOGI(TAG, "bind, uuid: " MACSTR ", initiator_type: %d", MAC2STR(info->mac), info->initiator_attribute);
        s_espnow_ctrl_status = APP_ESPNOW_CTRL_BOUND;
        app_led_set_color(0, 255, 0);
        break;
    }

    case ESP_EVENT_ESPNOW_CTRL_UNBIND: {
        espnow_ctrl_bind_info_t *info = (espnow_ctrl_bind_info_t *)event_data;
        ESP_LOGI(TAG, "unbind, uuid: " MACSTR ", initiator_type: %d", MAC2STR(info->mac), info->initiator_attribute);
        s_espnow_ctrl_status = APP_ESPNOW_CTRL_INIT;
        app_led_set_color(255, 0, 0);
        break;
    }

    default:
        break;
    }
}

static void light_control_task(void *pvParameters)
{
    light_gpio_init();
    while (1) {
        if (g_left_led_state == 1) {
            ESP_LOGI(TAG, "About to turn left.");
            gpio_set_level(LEFT_LED_PIN, 1);
            gpio_set_level(RIGHT_LED_PIN, 0);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            gpio_set_level(LEFT_LED_PIN, 0);
            gpio_set_level(RIGHT_LED_PIN, 0);
            vTaskDelay(400 / portTICK_PERIOD_MS);
        } else if (g_right_led_state == 1) {
            ESP_LOGI(TAG, "About to turn right.");
            gpio_set_level(LEFT_LED_PIN, 0);
            gpio_set_level(RIGHT_LED_PIN, 1);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            gpio_set_level(LEFT_LED_PIN, 0);
            gpio_set_level(RIGHT_LED_PIN, 0);
            vTaskDelay(400 / portTICK_PERIOD_MS);
        } else {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
}

void app_main(void)
{
    example_pwm_init();

    app_led_init();

    espnow_storage_init();

    app_wifi_init();

    esp_wifi_set_channel(13, WIFI_SECOND_CHAN_NONE);

    espnow_config_t espnow_config = ESPNOW_INIT_CONFIG_DEFAULT();
    espnow_init(&espnow_config);

    esp_event_handler_register(ESP_EVENT_ESPNOW, ESP_EVENT_ANY_ID, app_espnow_event_handler, NULL);

    app_responder_init();

    xTaskCreate(light_control_task, "light_control_task", 1024 * 4, NULL, 10, &light_control_task_handle);
}
