/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "esp_log.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"

static const char *TAG = "example";

// LVGL image declare
LV_IMG_DECLARE(esp_logo)

static lv_disp_t *display;
static lv_obj_t *lbl_rotation;
static lv_disp_rot_t rotation = LV_DISP_ROT_NONE;


/*******************************************************************************
* Private functions
*******************************************************************************/

static uint16_t app_lvgl_get_rotation_degrees(lv_disp_rot_t rotation)
{
    switch (rotation) {
    case LV_DISP_ROT_NONE:
        return 0;
    case LV_DISP_ROT_90:
        return 90;
    case LV_DISP_ROT_180:
        return 180;
    case LV_DISP_ROT_270:
        return 270;
    }

    return 0;
}

static void app_lvgl_btn_right_cb(lv_event_t *e)
{
    if (rotation == LV_DISP_ROT_270) {
        rotation = LV_DISP_ROT_NONE;
    } else {
        rotation++;
    }

    /* Rotate display */
    bsp_display_rotate(display, rotation);
    lv_label_set_text_fmt(lbl_rotation, "Rotation %d°", app_lvgl_get_rotation_degrees(rotation));

}

static void app_lvgl_btn_left_cb(lv_event_t *e)
{
    if (rotation == LV_DISP_ROT_NONE) {
        rotation = LV_DISP_ROT_270;
    } else {
        rotation--;
    }

    /* Rotate display */
    bsp_display_rotate(display, rotation);
    lv_label_set_text_fmt(lbl_rotation, "Rotation %d°", app_lvgl_get_rotation_degrees(rotation));
}

static void app_lvgl_display(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_t *lbl;
    bsp_display_lock(0);

    // Create image
    lv_obj_t *img_logo = lv_img_create(scr);
    lv_img_set_src(img_logo, &esp_logo);
    lv_obj_align(img_logo, LV_ALIGN_TOP_MID, 0, 20);

    lbl_rotation = lv_label_create(scr);
    lv_label_set_text(lbl_rotation, "Rotation 0°");
    lv_obj_align(lbl_rotation, LV_ALIGN_CENTER, 0, 20);

    lv_obj_t *cont_row = lv_obj_create(scr);
    lv_obj_set_size(cont_row, BSP_LCD_V_RES - 10, 50);
    lv_obj_align(cont_row, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_flex_flow(cont_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_top(cont_row, 5, 0);
    lv_obj_set_style_pad_bottom(cont_row, 5, 0);
    lv_obj_set_style_pad_left(cont_row, 5, 0);
    lv_obj_set_style_pad_right(cont_row, 5, 0);
    lv_obj_set_flex_align(cont_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Button rotate left */
    lv_obj_t *btn_left = lv_btn_create(cont_row);
    lbl = lv_label_create(btn_left);
    lv_label_set_text_static(lbl, LV_SYMBOL_LEFT" Left");
    lv_obj_align(btn_left, LV_ALIGN_BOTTOM_LEFT, 30, -30);
    /* Button event */
    lv_obj_add_event_cb(btn_left, app_lvgl_btn_left_cb, LV_EVENT_CLICKED, scr);

    lbl = lv_label_create(cont_row);
    lv_label_set_text_static(lbl, " rotate ");

    /* Button rotate right */
    lv_obj_t *btn_right = lv_btn_create(cont_row);
    lbl = lv_label_create(btn_right);
    lv_label_set_text_static(lbl, "Right "LV_SYMBOL_RIGHT);
    lv_obj_align(btn_right, LV_ALIGN_BOTTOM_LEFT, 30, -30);
    /* Button event */
    lv_obj_add_event_cb(btn_right, app_lvgl_btn_right_cb, LV_EVENT_CLICKED, scr);

    /* Input device group */
    lv_indev_t *indev = bsp_display_get_input_dev();
    if (indev && indev->driver && indev->driver->type == LV_INDEV_TYPE_ENCODER) {
        lv_group_t *main_group = lv_group_create();
        lv_group_add_obj(main_group, btn_left);
        lv_group_add_obj(main_group, btn_right);
        lv_indev_set_group(indev, main_group);
        ESP_LOGI(TAG, "Input device group was set.");
    }

    bsp_display_unlock();
}

void app_main(void)
{
    /* Initialize I2C (for touch and audio) */
    bsp_i2c_init();

    /* Initialize display and LVGL */
    display = bsp_display_start();

    /* Set display brightness to 100% */
    bsp_display_backlight_on();

    /* Add and show objects on display */
    app_lvgl_display();

    ESP_LOGI(TAG, "Example initialization done.");

}
