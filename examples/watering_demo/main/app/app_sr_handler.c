/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */


#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "app_audio.h"
#include "app_sr.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_check.h"
#include "ui_sr.h"
#include "app_pump.h"
#include "app_sr_handler.h"
#include "esp_afe_sr_iface.h"

static const char *TAG = "sr_handler";


void sr_handler_task(void *pvParam)
{
    QueueHandle_t xQueue = (QueueHandle_t) pvParam;

    while (true) {
        sr_result_t result;
        xQueueReceive(xQueue, &result, portMAX_DELAY);

        ESP_LOGI(TAG, "cmd:%d, wakemode:%d,state:%d", result.command_id, result.wakenet_mode, result.state);

        if (ESP_MN_STATE_TIMEOUT == result.state) {
            sr_anim_set_text("Timeout");
            sr_anim_stop();
            continue;
        }

        if (WAKENET_DETECTED == result.wakenet_mode) {
            sr_anim_start();
            sr_anim_set_text("Say command");
            app_audio_beep_play_start();
            continue;
        }

        if (ESP_MN_STATE_DETECTED & result.state) {
            sr_anim_stop();

            switch (result.command_id) {
            case 0://stop
            case 1://stop watering
                app_pump_watering_stop();
                break;
            case 2://please watering
            case 3:
                app_pump_watering_start();
                break;
            default:
                break;
            }
            /* **************** REGISTER COMMAND CALLBACK HERE **************** */
        }
    }

    vTaskDelete(NULL);
}
