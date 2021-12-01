/**
 * @file sr_handler.c
 * @brief 
 * @version 0.1
 * @date 2021-09-19
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

#include "app_audio.h"
#include "app_led.h"
#include "app_sr.h"
#include "app_data_parse.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "ui_main.h"

static const char *TAG = "sr_handler";

void sr_handler_task(void *pvParam)
{
    EventGroupHandle_t event_group_handle = (EventGroupHandle_t) pvParam;

    while (true) {
        /* Wait for event(s) sent from audio_detect_task */
        EventBits_t event_val = xEventGroupWaitBits(event_group_handle, SR_EVENT_ALL, true, false, portMAX_DELAY);

        if (SR_EVENT_TIMEOUT & event_val) {
            sr_anim_set_text(STR_TIMEOUT);
            sr_anim_stop();
            ESP_LOGW(TAG, LOG_BOLD(LOG_COLOR_BROWN) "Timeout");
            continue;
        }

        if (SR_EVENT_WAKE_UP & event_val) {
            sr_anim_start();
            sr_anim_set_text(STR_WAKEWORD);
            audio_play_start();
            ESP_LOGI(TAG, LOG_BOLD(LOG_COLOR_GREEN) "Say command");
            continue;
        }

        if (SR_EVENT_WORD_DETECT & event_val) {
            int32_t cmd_id = app_sr_get_last_cmd_id();
            sr_anim_stop();

            /* **************** REGISTER COMMAND CALLBACK HERE **************** */

            /* Register your callback here. You can get command id from cmd_id  */
            switch (cmd_id) {
            case SR_CMD_LIGHT_ON:
                sr_anim_set_text(get_cmd_string(SR_CMD_LIGHT_ON));
                app_pwm_led_set_all(30, 30, 30);
                break;
            case SR_CMD_LIGHT_OFF:
                sr_anim_set_text(get_cmd_string(SR_CMD_LIGHT_OFF));
                app_pwm_led_set_all(0, 0, 0);
                break;
            case SR_CMD_SET_RED:
                sr_anim_set_text(STR_LIGHT_RED);
                app_pwm_led_set_all(30, 0, 0);
                break;
            case SR_CMD_SET_GREEN:
                sr_anim_set_text(STR_LIGHT_GREEN);
                app_pwm_led_set_all(0, 30, 0);
                break;
            case SR_CMD_SET_BLUE:
                sr_anim_set_text(STR_LIGHT_BLUE);
                app_pwm_led_set_all(0, 0, 30);
                break;
            case SR_CMD_SET_WHITE:
                sr_anim_set_text(STR_LIGHT_WHITE);
                app_pwm_led_set_all(30, 30, 30);
                break;
            case SR_CMD_CUSTOM_COLOR:
                sr_anim_set_text(get_cmd_string(SR_CMD_CUSTOM_COLOR));
                led_state_t *led_state = get_default_led_config();
                lv_color_t color = lv_color_hsv_to_rgb(led_state->h, led_state->s, led_state->v);
                app_pwm_led_set_all(color.ch.red << 3, (color.ch.green_h << 5) + (color.ch.green_l << 2), color.ch.blue << 3);
                break;
            default:
                /* Defalut handler */
                ESP_LOGI(TAG, LOG_BOLD(LOG_COLOR_GREEN) "Detected : %d", cmd_id);
                break;
            }

            ui_dev_ctrl_update_state();
            /* **************** ENDS REGISTER COMMAND CALLBACK **************** */
        }
    }

    vTaskDelete(NULL);
}
