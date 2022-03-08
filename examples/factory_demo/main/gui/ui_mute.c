/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdbool.h>
#include <stdint.h>
#include "bsp_board.h"
#include "bsp_codec.h"
#include "driver/gpio.h"
#include "ui_main.h"

LV_IMG_DECLARE(mute_on)
LV_IMG_DECLARE(mute_off)

LV_FONT_DECLARE(font_en_16)

static int32_t mute_disp_count = 15;
static const int32_t disp_time = 15;
static bool mute_state = false;

static void mute_timer_cb(lv_timer_t *timer)
{
    lv_obj_t *obj = (lv_obj_t *) timer->user_data;
    lv_obj_t *img = (lv_obj_t *) obj->user_data;

    if (mute_disp_count <= disp_time) {
        if (mute_disp_count == 0) {
            if (mute_state) {
                lv_img_set_src(img, &mute_on);
            } else {
                lv_img_set_src(img, &mute_off);
            }
        } else if (disp_time != mute_disp_count) {
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(obj);
            if (mute_disp_count == 2) {
                bsp_codec_init(AUDIO_HAL_16K_SAMPLES);
            }
        } else {
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        mute_disp_count++;
    }
}

void ui_mute_init(void)
{
    static lv_obj_t *obj = NULL;
    static lv_obj_t *img = NULL;
    if (NULL == obj) {
        obj = lv_obj_create(lv_scr_act());
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(obj, 180, 45);
        lv_obj_set_style_bg_color(obj, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(obj, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_radius(obj, 10, LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(obj, 20, LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(obj, LV_OPA_50, LV_STATE_DEFAULT);
        lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 10);

        lv_obj_t *lab = lv_label_create(obj);
        lv_label_set_text_static(lab, "Voice Command");
        lv_obj_set_style_text_font(lab, &font_en_16, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(lab, lv_color_black(), LV_STATE_DEFAULT);
        lv_obj_align(lab, LV_ALIGN_CENTER, 10, 0);

        img = lv_img_create(obj);
        lv_img_set_src(img, &mute_on);
        lv_obj_align(img, LV_ALIGN_CENTER, -70, 0);
        lv_obj_set_user_data(obj, (void *) img);
    }

    lv_timer_create(mute_timer_cb, 100, (void *) obj);

}

static void ui_mute_set_state(bool mute)
{
    mute_state = mute;
    mute_disp_count = 0;
}

/**
 * @brief Mute button handler.
 *
 * @note Due to other examples might have no handler of mute button. So the
 *       default handler just simply print the mute button state and marked
 *       as `WEAK` so that you can rewrite the isr handler.
 *
 * @param arg Unused
 */
void mute_btn_handler(void *arg)
{
    (void) arg;
    const board_res_desc_t *brd = bsp_board_get_description();
    if (brd->GPIO_MUTE_LEVEL == gpio_get_level(brd->GPIO_MUTE_NUM)) {
        ets_printf(DRAM_STR("Mute Off"));
        ui_mute_set_state(0);
    } else {
        ets_printf(DRAM_STR("Mute On"));
        ui_mute_set_state(1);
    }
}
