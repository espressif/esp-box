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
            sr_anim_set_text("Timeout");
            sr_anim_stop();
            ESP_LOGW(TAG, LOG_BOLD(LOG_COLOR_BROWN) "Timeout");
            continue;
        }

        if (SR_EVENT_WAKE_UP & event_val) {
            sr_anim_start();
            sr_anim_set_text("Hi ESP");
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
                sr_anim_set_text(get_cmd_string(true));
                app_pwm_led_set_all(30, 30, 30);
                break;
            case SR_CMD_LIGHT_OFF:
                sr_anim_set_text(get_cmd_string(false));
                app_pwm_led_set_all(0, 0, 0);
                break;
            case SR_CMD_SET_RED:
                sr_anim_set_text("Turn Red");
                app_pwm_led_set_all(30, 0, 0);
                break;
            case SR_CMD_SET_GREEN:
                sr_anim_set_text("Turn Green");
                app_pwm_led_set_all(0, 30, 0);
                break;
            case SR_CMD_SET_BLUE:
                sr_anim_set_text("Turn Blue");
                app_pwm_led_set_all(0, 0, 30);
                break;
            case SR_CMD_SET_WHITE:
                sr_anim_set_text("Turn White");
                app_pwm_led_set_all(30, 30, 30);
                break;
            default:
                /* Defalut handler */
                ESP_LOGI(TAG, LOG_BOLD(LOG_COLOR_GREEN) "Detected : %d", cmd_id);
                break;
            }
            
            /* **************** ENDS REGISTER COMMAND CALLBACK **************** */
        }
    }

    vTaskDelete(NULL);
}
