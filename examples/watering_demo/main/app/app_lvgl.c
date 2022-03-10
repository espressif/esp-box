/*
* SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Unlicense OR CC0-1.0
*/

#include <stdbool.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task.h"

#include "bsp_lcd.h"

#include "lv_port.h"
#include "lvgl.h"

#include "gui/ui_main.h"

static void lvgl_task(void *pvParam)
{
    (void) pvParam;

    ui_main();
    bsp_lcd_set_backlight(true);

    for (;;) {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}

esp_err_t app_lvgl_start(void)
{
    ESP_ERROR_CHECK(lv_port_init());

    BaseType_t ret_val = xTaskCreatePinnedToCore(
                             (TaskFunction_t)        lvgl_task,
                             (const char *const)    "lvgl Task",
                             (const uint32_t)        3 * 1024,
                             (void *const)          NULL,
                             (UBaseType_t)           ESP_TASK_PRIO_MIN + 1,
                             (TaskHandle_t *const)  NULL,
                             (const BaseType_t)      0);
    ESP_ERROR_CHECK(ret_val == pdPASS ? ESP_OK : ESP_FAIL);
    return ESP_OK;
}

