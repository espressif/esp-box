/**
 * @file mp3_demo.c
 * @brief 
 * @version 0.1
 * @date 2021-11-11
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

#include "audio.h"
#include "bsp_board.h"
#include "bsp_lcd.h"
#include "bsp_storage.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lv_port.h"
#include "lvgl.h"
#include "ui_audio.h"

void app_main(void)
{
    ESP_ERROR_CHECK(bsp_board_init());
    ESP_ERROR_CHECK(bsp_board_power_ctrl(POWER_MODULE_AUDIO, true));

    ESP_ERROR_CHECK(bsp_spiffs_init("storage", "/spiffs", 2));
    ESP_ERROR_CHECK(lv_port_init());
    bsp_lcd_set_backlight(true);

    ui_audio_start();
    ESP_ERROR_CHECK(mp3_player_start("/spiffs"));

    do {
        lv_task_handler();
    } while (vTaskDelay(1), true);
}
