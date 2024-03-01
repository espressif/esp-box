/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <dirent.h>

#include "bsp/esp-bsp.h"
#include "esp_log.h"

static const char *TAG = "main";

static lv_group_t *g_btn_op_group = NULL;

static void image_display(void);

void app_main(void)
{
    /* Initialize I2C (for touch and audio) */
    bsp_i2c_init();

    /* Initialize display and LVGL */
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_H_RES * CONFIG_BSP_LCD_DRAW_BUF_HEIGHT,
        .double_buffer = 0,
        .flags = {
            .buff_dma = true,
        }
    };
    bsp_display_start_with_config(&cfg);

    /* Set display brightness to 100% */
    bsp_display_backlight_on();

    /* Mount SPIFFS */
    bsp_spiffs_mount();

    image_display();
}

static void btn_event_cb(lv_event_t *event)
{
    lv_obj_t *img = (lv_obj_t *) event->user_data;
    const char *file_name = lv_list_get_btn_text(lv_obj_get_parent(event->target), event->target);
    char *file_name_with_path = (char *) heap_caps_malloc(256, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    if (NULL != file_name_with_path) {
        /* Get full file name with mount point and folder path */
        strcpy(file_name_with_path, "S:/spiffs/");
        strcat(file_name_with_path, file_name);

        /* Set src of image with file name */
        lv_img_set_src(img, file_name_with_path);

        /* Align object */
        lv_obj_align(img, LV_ALIGN_CENTER, 80, 0);

        /* Only for debug */
        ESP_LOGI(TAG, "Display image file : %s", file_name_with_path);

        /* Don't forget to free allocated memory */
        free(file_name_with_path);
    }
}

static void image_display(void)
{
    lv_indev_t *indev = lv_indev_get_next(NULL);

    if ((lv_indev_get_type(indev) == LV_INDEV_TYPE_KEYPAD) || \
            lv_indev_get_type(indev) == LV_INDEV_TYPE_ENCODER) {
        ESP_LOGI(TAG, "Input device type is keypad");
        g_btn_op_group = lv_group_create();
        lv_indev_set_group(indev, g_btn_op_group);
    }

    lv_obj_t *list = lv_list_create(lv_scr_act());
    lv_obj_set_size(list, 170, 220);
    lv_obj_set_style_border_width(list, 0, LV_STATE_DEFAULT);
    lv_obj_align(list, LV_ALIGN_LEFT_MID, -15, 0);

    lv_obj_t *img = lv_img_create(lv_scr_act());

    /* Get file name in storage */
    struct dirent *p_dirent = NULL;
    DIR *p_dir_stream = opendir("/spiffs");

    /* Scan files in storage */
    while (true) {
        p_dirent = readdir(p_dir_stream);
        if (NULL != p_dirent) {
            lv_obj_t *btn = lv_list_add_btn(list, LV_SYMBOL_IMAGE, p_dirent->d_name);
            lv_group_add_obj(g_btn_op_group, btn);
            lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, (void *) img);
        } else {
            closedir(p_dir_stream);
            break;
        }
    }
}
