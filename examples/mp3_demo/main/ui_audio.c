/**
 * @file ui_audio.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2021-11-10
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

#include "lvgl.h"
#include "audio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "es8311.h"

#if CONFIG_ESP32_S3_BOX_LITE_BOARD
static void register_button_callback(lv_obj_t *btn_list[]);
#endif

static void btn_play_pause_cb(lv_event_t *event)
{
    lv_obj_t *btn = (lv_obj_t *) event->target;
    lv_obj_t *lab = (lv_obj_t *) btn->user_data;

    if (lv_obj_has_state(btn, LV_STATE_CHECKED)) {
        lv_label_set_text_static(lab, LV_SYMBOL_PLAY);
        audio_pause();
    } else {
        lv_label_set_text_static(lab, LV_SYMBOL_PAUSE);
        audio_play();
    }
}

static void btn_prev_next_cb(lv_event_t *event)
{
    bool is_next = (bool) event->user_data;

    if (is_next) {
        audio_play_next();
    } else {
        audio_play_prev();
    }
}

static void volume_slider_cb(lv_event_t *event)
{
    lv_obj_t *slider = (lv_obj_t *) event->target;
    int volume = lv_slider_get_value(slider);
    es8311_codec_set_voice_volume(volume);
}

static void audio_callback(audio_cb_ctx_t *ctx)
{
    audio_event_t event = ctx->audio_event;

    lv_obj_t *music_list = (lv_obj_t *) ctx->user_ctx;
    lv_obj_t *label_title = (lv_obj_t *) music_list->user_data;
    lv_obj_t *btn_play_pause = (lv_obj_t *) label_title->user_data;
    lv_obj_t *label_play_pause = (lv_obj_t *) btn_play_pause->user_data;

    if (AUDIO_EVENT_FILE_SCAN_DONE == event) {
        lv_dropdown_clear_options(music_list);

        size_t i = 0;
        while (true) {
            char *file_name = audio_get_name_from_index(i, NULL);
            if (NULL != file_name) {
                lv_dropdown_add_option(music_list, file_name, i);
            } else {
                lv_dropdown_set_selected(music_list, 0);
                lv_label_set_text_static(label_title,
                    audio_get_name_from_index(0, NULL));
                break;
            }
            i++;
        }
    }

    if (AUDIO_EVENT_CHANGE == event) {
        lv_dropdown_set_selected(music_list, audio_get_index());
        lv_label_set_text_static(label_title,
            audio_get_name_from_index(audio_get_index(), NULL));
    
        /* Update play button */
        if (lv_obj_has_state(btn_play_pause, LV_STATE_CHECKED)) {
            lv_obj_clear_state(btn_play_pause, LV_STATE_CHECKED);
            lv_label_set_text_static(label_play_pause, LV_SYMBOL_PAUSE);
            lv_obj_invalidate(btn_play_pause);
        }
    }
}

static void music_list_cb(lv_event_t *event)
{
    lv_obj_t *music_list = (lv_obj_t *) event->target;
    audio_play_index(lv_dropdown_get_selected(music_list));
}

void ui_audio_start(void)
{
    /* Create audio control button */
    lv_obj_t *btn_play_pause = lv_btn_create(lv_scr_act());
    lv_obj_align(btn_play_pause, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_size(btn_play_pause, 50, 50);
    lv_obj_set_style_radius(btn_play_pause, 25, LV_STATE_DEFAULT);
    lv_obj_add_flag(btn_play_pause, LV_OBJ_FLAG_CHECKABLE);

    lv_obj_t *label_play_pause = lv_label_create(btn_play_pause);
    lv_label_set_text_static(label_play_pause, LV_SYMBOL_PAUSE);
    lv_obj_center(label_play_pause);
    lv_obj_set_user_data(btn_play_pause, (void *) label_play_pause);
    lv_obj_add_event_cb(btn_play_pause, btn_play_pause_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *label_prev = lv_label_create(lv_scr_act());
    lv_obj_set_user_data(label_prev, btn_play_pause);
    lv_obj_add_flag(label_prev, LV_OBJ_FLAG_CLICKABLE);
    lv_label_set_text_static(label_prev, LV_SYMBOL_PREV);
    lv_obj_set_style_text_font(label_prev, &lv_font_montserrat_24, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_prev, lv_color_make(255, 0, 0) , LV_STATE_PRESSED);
    lv_obj_align_to(label_prev, btn_play_pause, LV_ALIGN_OUT_LEFT_MID, -40, 0);
    lv_obj_add_event_cb(label_prev, btn_prev_next_cb, LV_EVENT_CLICKED, (void *) false);

    lv_obj_t *label_next = lv_label_create(lv_scr_act());
    lv_obj_set_user_data(label_next, btn_play_pause);
    lv_obj_add_flag(label_next, LV_OBJ_FLAG_CLICKABLE);
    lv_label_set_text_static(label_next, LV_SYMBOL_NEXT);
    lv_obj_set_style_text_font(label_next, &lv_font_montserrat_24, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_next, lv_color_make(255, 0, 0) , LV_STATE_PRESSED);
    lv_obj_align_to(label_next, btn_play_pause, LV_ALIGN_OUT_RIGHT_MID, 40, 0);
    lv_obj_add_event_cb(label_next, btn_prev_next_cb, LV_EVENT_CLICKED, (void *) true);

    /* Create volume slider */
    lv_obj_t *volume_slider = lv_slider_create(lv_scr_act());
    lv_obj_set_size(volume_slider, 200, 6);
    lv_obj_set_ext_click_area(volume_slider, 15);
    lv_obj_align(volume_slider, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_slider_set_range(volume_slider, 0, 75);
    lv_slider_set_value(volume_slider, 50, LV_ANIM_ON);
    lv_obj_add_event_cb(volume_slider, volume_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *lab_vol_min = lv_label_create(lv_scr_act());
    lv_label_set_text_static(lab_vol_min, LV_SYMBOL_VOLUME_MID);
    lv_obj_set_style_text_font(lab_vol_min, &lv_font_montserrat_20, LV_STATE_DEFAULT);
    lv_obj_align_to(lab_vol_min, volume_slider, LV_ALIGN_OUT_LEFT_MID, -10, 0);

    lv_obj_t *lab_vol_max = lv_label_create(lv_scr_act());
    lv_label_set_text_static(lab_vol_max, LV_SYMBOL_VOLUME_MAX);
    lv_obj_set_style_text_font(lab_vol_max, &lv_font_montserrat_20, LV_STATE_DEFAULT);
    lv_obj_align_to(lab_vol_max, volume_slider, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    lv_obj_t *lab_title = lv_label_create(lv_scr_act());
    lv_obj_set_user_data(lab_title, (void *) btn_play_pause);
    lv_label_set_text_static(lab_title, "Scaning Files...");
    lv_obj_set_style_text_font(lab_title, &lv_font_montserrat_32, LV_STATE_DEFAULT);
    lv_obj_align(lab_title, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *music_list = lv_dropdown_create(lv_scr_act());
    lv_dropdown_clear_options(music_list);
    lv_dropdown_set_options_static(music_list, "Scaning...");
    lv_obj_set_width(music_list, 200);
    lv_obj_align_to(music_list, lab_title, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_set_user_data(music_list, (void *) lab_title);
    lv_obj_add_event_cb(music_list, music_list_cb, LV_EVENT_VALUE_CHANGED, NULL);

    audio_callback_register(audio_callback, (void *) music_list);

#if CONFIG_ESP32_S3_BOX_LITE_BOARD
    register_button_callback((lv_obj_t *[]) { label_prev, label_next, btn_play_pause });
#endif
}

#if CONFIG_ESP32_S3_BOX_LITE_BOARD
#include "bsp_btn.h"

static void prev_click_cb(void *arg)
{
    button_dev_t *event = (button_dev_t *) arg;
    lv_obj_t *obj = (lv_obj_t *) event->cb_user_data;

    lv_event_send(obj, LV_EVENT_CLICKED, NULL);
}

static void next_click_cb(void *arg)
{
    button_dev_t *event = (button_dev_t *) arg;
    lv_obj_t *obj = (lv_obj_t *) event->cb_user_data;

    lv_event_send(obj, LV_EVENT_CLICKED, NULL);
}

static void play_pause_click_cb(void *arg)
{
    button_dev_t *event = (button_dev_t *) arg;
    lv_obj_t *obj = (lv_obj_t *) event->cb_user_data;

    if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
        lv_obj_clear_state(obj, LV_STATE_CHECKED);
    } else {
        lv_obj_add_state(obj, LV_STATE_CHECKED);
    }

    lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
}

static void register_button_callback(lv_obj_t *btn_list[])
{
    lv_obj_t *btn_prev = btn_list[0];
    lv_obj_t *btn_next = btn_list[1];
    lv_obj_t *btn_play_pause = btn_list[2];

    bsp_btn_set_user_data(0, (void *) btn_prev);
    bsp_btn_set_user_data(1, (void *) btn_play_pause);
    bsp_btn_set_user_data(2, (void *) btn_next);

    bsp_btn_register_callback(0, BUTTON_SINGLE_CLICK, prev_click_cb);
    bsp_btn_register_callback(1, BUTTON_SINGLE_CLICK, play_pause_click_cb);
    bsp_btn_register_callback(2, BUTTON_SINGLE_CLICK, next_click_cb);
}
#endif
