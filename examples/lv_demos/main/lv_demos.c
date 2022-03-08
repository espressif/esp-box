/**
 * @file lv_demos.c
 * @brief Evaluate demos provided by LVGL
 * @version 0.1
 * @date 2021-10-19
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

#include "bsp_board.h"
#include "bsp_lcd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lv_demo.h"
#include "lv_port.h"
#include "lvgl.h"

void app_main(void)
{
    ESP_ERROR_CHECK(bsp_board_init());

    ESP_ERROR_CHECK(lv_port_init());
    bsp_lcd_set_backlight(true);

    /**
     * @brief Demos provided by LVGL.
     * 
     * @note Only enable one demo every time.
     * 
     */
    lv_demo_widgets();      /* A widgets example. This is what you get out of the box */
    // lv_demo_music();        /* A modern, smartphone-like music player demo. */
    // lv_demo_stress();       /* A stress test for LVGL. */
    // lv_demo_benchmark();    /* A demo to measure the performance of LVGL or to compare different settings. */

    do {
        lv_task_handler();
    } while (vTaskDelay(1), true);
}
