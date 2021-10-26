/**
 * @file freetype_example.c
 * @brief Display fonts with FreeType
 * @version 0.1
 * @date 2021-10-21
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

#include <stdbool.h>
#include <dirent.h>
#include "bsp_board.h"
#include "bsp_lcd.h"
#include "bsp_storage.h"
#include "bsp_tp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lv_demo.h"
#include "lv_port.h"
#include "lv_port_fs.h"
#include "lvgl.h"
#include "lv_freetype.h"

static void font_preview(void);

void app_main(void)
{
    ESP_ERROR_CHECK(bsp_board_init());
    ESP_ERROR_CHECK(bsp_spiffs_init_default());

    ESP_ERROR_CHECK(bsp_lcd_init());
    ESP_ERROR_CHECK(bsp_tp_init());
    ESP_ERROR_CHECK(lv_port_init());
    ESP_ERROR_CHECK(lv_port_fs_init());
    lv_freetype_init(32, 1, 0);

    font_preview();

    while (vTaskDelay(1), true) {
        lv_task_handler();
    }
}

static lv_style_t style;
static lv_ft_info_t info = {
    .name = NULL,
    .font = NULL,
    .weight = 28,
    .style = FT_FONT_STYLE_NORMAL,
};

static void btn_event_cb(lv_event_t *event)
{
    lv_obj_t *label = (lv_obj_t *) event->user_data;
    const char *file_name = lv_list_get_btn_text(lv_obj_get_parent(event->target), event->target);
    char *file_name_with_path = (char *) heap_caps_malloc(256, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    if (NULL != file_name_with_path) {
        /* Get full file name with mount point and folder path */
        strcpy(file_name_with_path, "/spiffs/fonts/");
        strcat(file_name_with_path, file_name);

        /* Set font name and size */
        if (NULL != info.font) {
            lv_ft_font_destroy(info.font);
        }
        info.name = file_name_with_path;
        lv_ft_font_init(&info);

        /*Create style with the new font*/
        lv_style_init(&style);
        lv_style_set_text_font(&style, info.font);
        lv_style_set_text_color(&style, lv_color_black());

        /*Create a label with the new style*/
        lv_obj_add_style(label, &style, LV_STATE_DEFAULT);
        lv_label_set_text(label,
            "你好，世界！\n"
            "Hello World!");

        /* Align object */
        lv_obj_align(label, LV_ALIGN_CENTER, lv_obj_get_width(lv_scr_act()) / 5, 0);

        /* Don't forget to free allocated memory */
        free(file_name_with_path);
    }
}

static void font_preview(void)
{
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);

    /* Create a list to show font file(s) */
    lv_obj_t *list = lv_list_create(lv_scr_act());
    lv_obj_set_size(list, 150, 220);
    lv_obj_set_style_border_width(list, 0, LV_STATE_DEFAULT);
    lv_obj_align(list, LV_ALIGN_LEFT_MID, -15, 0);

    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Select font to display");
    lv_obj_align(label, LV_ALIGN_CENTER, lv_obj_get_width(lv_scr_act()) / 5, 0);

    /* Get file name in storage */
    DIR *p_dir_stream = opendir("/spiffs/fonts");
    struct dirent *p_dirent = NULL;

    /* Scan files in storage */
    while (true) {
        p_dirent = readdir(p_dir_stream);
        if (NULL != p_dirent) {
            lv_obj_t *btn = lv_list_add_btn(list, NULL, p_dirent->d_name);
            lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, (void *) label);
        } else {
            closedir(p_dir_stream);
            break;
        }
    }
}
