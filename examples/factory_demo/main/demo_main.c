/**
 * @file demo_main.c
 * @brief 
 * @version 0.1
 * @date 2021-06-23
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
#include "app_sr.h"
#include "bsp_board.h"
#include "bsp_codec.h"
#include "bsp_i2s.h"
#include "bsp_lcd.h"
#include "bsp_storage.h"
#include "bsp_tp.h"
#include "esp_err.h"
#include "freertos/task.h"
#include "lv_demo.h"
#include "lv_port.h"
#include "lvgl.h"
#include "ui_main.h"

void app_main(void)
{
    ESP_ERROR_CHECK(bsp_board_init());
    ESP_ERROR_CHECK(bsp_board_power_ctrl(POWER_MODULE_AUDIO, true));
    ESP_ERROR_CHECK(bsp_storage_init(BSP_STORAGE_SPIFFS));
    // ESP_ERROR_CHECK(bsp_storage_init(BSP_STORAGE_SD_CARD));

    ESP_ERROR_CHECK(bsp_lcd_init());
    ESP_ERROR_CHECK(bsp_tp_init());
    ESP_ERROR_CHECK(lv_port_init());
    ui_main_start();
    lv_task_handler();

    ESP_ERROR_CHECK(bsp_i2s_init(I2S_NUM_0, 16000));
    ESP_ERROR_CHECK(bsp_codec_init(AUDIO_HAL_16K_SAMPLES));
    ESP_ERROR_CHECK(app_audio_start());
    ESP_ERROR_CHECK(app_sr_start());

    while (vTaskDelay(1), true) {
        lv_task_handler();
    }
}
