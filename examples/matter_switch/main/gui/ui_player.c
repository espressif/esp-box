/*
 * SPDX-FileCopyrightText: 2015-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "esp_log.h"
#include "bsp_board.h"
#include "audio_player.h"
#include "file_iterator.h"
#include "lvgl.h"
#include "ui_main.h"
#include "settings.h"

static const char *TAG = "ui_player";

static bool g_media_is_playing = false;
lv_obj_t *g_lab_file = NULL;
static void (*g_player_end_cb)(void) = NULL;
lv_obj_t *lab_play_pause = NULL;

extern file_iterator_instance_t *file_iterator;
lv_obj_t *player_page = NULL;

lv_obj_t *get_player_page()
{
    return player_page;
}

static void ui_player_page_vol_inc_click_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_user_data(e);
    sys_param_t *param = settings_get_parameter();
    uint8_t v = param->volume / 10;
    if (v < 10) {
        v++;
    }
    param->volume = v * 10;
    bsp_codec_volume_set(param->volume, NULL);
    lv_bar_set_value(obj, v, LV_ANIM_ON);
}

static void ui_player_page_vol_dec_click_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_user_data(e);
    sys_param_t *param = settings_get_parameter();
    uint8_t v = param->volume / 10;
    if (v > 0) {
        v -= 1;
    }
    param->volume = v * 10;
    bsp_codec_volume_set(param->volume, NULL);
    lv_bar_set_value(obj, v, LV_ANIM_ON);
}

static void play_present()
{
    char filename[128];
    file_iterator_get_full_path_from_index(file_iterator, file_iterator_get_index(file_iterator), filename, sizeof(filename));
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        ESP_LOGE(TAG, "unable to open '%s'", filename);
        return;
    }

    audio_player_play(fp);
}

static void ui_player_page_pause_click_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_user_data(e);

    if (g_media_is_playing) {
        audio_player_pause();
        g_media_is_playing = false;
        lv_label_set_text_static(obj, LV_SYMBOL_PLAY);
    } else {
        play_present();
        g_media_is_playing = true;
        lv_label_set_text_static(obj, LV_SYMBOL_PAUSE);
    }
}

static void ui_player_page_prev_click_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_user_data(e);
    file_iterator_prev(file_iterator);
    play_present();
    lv_label_set_text_static(g_lab_file, file_iterator_get_name_from_index(file_iterator, file_iterator_get_index(file_iterator)));
    lv_event_t event = {
        .user_data = obj,
    };
    g_media_is_playing = 0;
    ui_player_page_pause_click_cb(&event);
}

static void ui_player_page_next_click_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_user_data(e);
    file_iterator_next(file_iterator);
    lv_label_set_text_static(g_lab_file, file_iterator_get_name_from_index(file_iterator, file_iterator_get_index(file_iterator)));
    lv_event_t event = {
        .user_data = obj,
    };
    g_media_is_playing = 0;
    ui_player_page_pause_click_cb(&event);
}

static void ui_player_page_return_click_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_user_data(e);
    if (ui_get_btn_op_group()) {
        lv_group_remove_all_objs(ui_get_btn_op_group());
    }
#if !CONFIG_BSP_BOARD_ESP32_S3_BOX_Lite
    bsp_btn_rm_all_callback(BSP_BUTTON_MAIN);
#endif
    lv_obj_del(obj);
    player_page = NULL;
    audio_player_callback_register(NULL, NULL);
    settings_write_parameter_to_nvs(); // save volume to nvs
    if (g_player_end_cb) {
        g_player_end_cb();
    }
}

#if !CONFIG_BSP_BOARD_ESP32_S3_BOX_Lite
static void btn_return_down_cb(void *handle, void *arg)
{
    lv_obj_t *obj = (lv_obj_t *) arg;
    ui_acquire();
    lv_event_send(obj, LV_EVENT_CLICKED, NULL);
    ui_release();
}
#endif

static void audio_cb(audio_player_cb_ctx_t *ctx)
{
    if (player_page == NULL) {
        ESP_LOGE(TAG, "player_page has exited!");
        return;
    }

    if (AUDIO_PLAYER_CALLBACK_EVENT_IDLE == ctx->audio_event) {
        g_media_is_playing = false;
        ui_acquire();
        lv_label_set_text_static(g_lab_file, file_iterator_get_name_from_index(file_iterator, file_iterator_get_index(file_iterator)));
        if (lab_play_pause) {
            lv_label_set_text_static(lab_play_pause, LV_SYMBOL_PLAY);
        }
        ui_release();
    }

    if ((AUDIO_PLAYER_CALLBACK_EVENT_PLAYING == ctx->audio_event) ||
            (AUDIO_PLAYER_CALLBACK_EVENT_COMPLETED_PLAYING_NEXT == ctx->audio_event)) {
        g_media_is_playing = true;
        ui_acquire();
        lv_label_set_text_static(g_lab_file, file_iterator_get_name_from_index(file_iterator, file_iterator_get_index(file_iterator)));
        if (lab_play_pause) {
            lv_label_set_text_static(lab_play_pause, LV_SYMBOL_PAUSE);
        }
        ui_release();
    }

    if (AUDIO_PLAYER_CALLBACK_EVENT_PAUSE == ctx->audio_event) {
        g_media_is_playing = false;
        ui_acquire();
        lv_label_set_text_static(g_lab_file, file_iterator_get_name_from_index(file_iterator, file_iterator_get_index(file_iterator)));
        if (lab_play_pause) {
            lv_label_set_text_static(lab_play_pause, LV_SYMBOL_PLAY);
        }
        ui_release();
    }
}

void ui_media_player(void (*fn)(void))
{
    g_player_end_cb = fn;
    lv_obj_t *page = lv_obj_create(lv_scr_act());
    player_page = page;
    lv_obj_set_size(page, lv_obj_get_width(lv_obj_get_parent(page)), lv_obj_get_height(lv_obj_get_parent(page)) - lv_obj_get_height(ui_main_get_status_bar()));
    lv_obj_set_style_border_width(page, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(page, lv_obj_get_style_bg_color(lv_scr_act(), LV_STATE_DEFAULT), LV_PART_MAIN);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align_to(page, ui_main_get_status_bar(), LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

    lv_obj_t *btn_return = lv_btn_create(page);
    lv_obj_set_size(btn_return, 24, 24);
    lv_obj_add_style(btn_return, &ui_button_styles()->style, 0);
    lv_obj_add_style(btn_return, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(btn_return, &ui_button_styles()->style_focus, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_return, &ui_button_styles()->style_focus, LV_STATE_FOCUSED);
    lv_obj_align(btn_return, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t *lab_btn_text = lv_label_create(btn_return);
    lv_label_set_text_static(lab_btn_text, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(lab_btn_text, lv_color_make(158, 158, 158), LV_STATE_DEFAULT);
    lv_obj_center(lab_btn_text);
    lv_obj_add_event_cb(btn_return, ui_player_page_return_click_cb, LV_EVENT_CLICKED, page);
#if !CONFIG_BSP_BOARD_ESP32_S3_BOX_Lite
    bsp_btn_register_callback(BSP_BUTTON_MAIN, BUTTON_PRESS_UP, btn_return_down_cb, (void *)btn_return);
#endif

    LV_IMG_DECLARE(img_music)
    lv_obj_t *img = lv_img_create(page);
    lv_img_set_src(img, &img_music);
    lv_obj_align(img, LV_ALIGN_TOP_RIGHT, -10, 35);

    g_lab_file = lv_label_create(page);
    lv_label_set_text_static(g_lab_file, file_iterator_get_name_from_index(file_iterator, file_iterator_get_index(file_iterator)));
    lv_obj_set_size(g_lab_file, 250, 32);
    lv_obj_set_style_text_font(g_lab_file, &lv_font_montserrat_24, LV_STATE_DEFAULT);
    lv_label_set_long_mode(g_lab_file, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_align(g_lab_file, LV_ALIGN_TOP_MID, 10, 0);

    lv_obj_t *ctrl_panel = lv_obj_create(page);
    lv_obj_set_size(ctrl_panel, 160, 52);
    lv_obj_clear_flag(ctrl_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(ctrl_panel, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ctrl_panel, LV_OPA_30, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ctrl_panel, 20, LV_STATE_DEFAULT);

    lv_obj_t *btn_play_pause = lv_btn_create(ctrl_panel);
    lv_obj_set_size(btn_play_pause, 36, 36);
    lv_obj_add_style(btn_play_pause, &ui_button_styles()->style, 0);
    lv_obj_add_style(btn_play_pause, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(btn_play_pause, &ui_button_styles()->style_focus, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_play_pause, &ui_button_styles()->style_focus, LV_STATE_FOCUSED);
    lv_obj_set_style_radius(btn_play_pause, 18, LV_STATE_DEFAULT);
    lv_obj_align(btn_play_pause, LV_ALIGN_CENTER, 0, 0);
    lab_play_pause = lv_label_create(btn_play_pause);
    g_media_is_playing = (audio_player_get_state() == AUDIO_PLAYER_STATE_PLAYING);
    if (g_media_is_playing) {
        lv_label_set_text_static(lab_play_pause, LV_SYMBOL_PAUSE);
    } else {
        lv_label_set_text_static(lab_play_pause, LV_SYMBOL_PLAY);
    }
    lv_obj_set_style_text_color(lab_play_pause, lv_color_make(158, 158, 158), LV_STATE_DEFAULT);
    lv_obj_center(lab_play_pause);
    lv_obj_add_event_cb(btn_play_pause, ui_player_page_pause_click_cb, LV_EVENT_CLICKED, lab_play_pause);

    lv_obj_t *btn_prev = lv_btn_create(ctrl_panel);
    lv_obj_set_size(btn_prev, 36, 36);
    lv_obj_add_style(btn_prev, &ui_button_styles()->style, 0);
    lv_obj_add_style(btn_prev, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(btn_prev, &ui_button_styles()->style_focus, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_prev, &ui_button_styles()->style_focus, LV_STATE_FOCUSED);
    lv_obj_set_style_radius(btn_prev, 18, LV_STATE_DEFAULT);
    lv_obj_align(btn_prev, LV_ALIGN_CENTER, -54, 0);
    lab_btn_text = lv_label_create(btn_prev);
    lv_label_set_text_static(lab_btn_text, LV_SYMBOL_PREV);
    lv_obj_set_style_text_color(lab_btn_text, lv_color_make(158, 158, 158), LV_STATE_DEFAULT);
    lv_obj_center(lab_btn_text);
    lv_obj_add_event_cb(btn_prev, ui_player_page_prev_click_cb, LV_EVENT_CLICKED, lab_play_pause);

    lv_obj_t *btn_next = lv_btn_create(ctrl_panel);
    lv_obj_set_size(btn_next, 36, 36);
    lv_obj_add_style(btn_next, &ui_button_styles()->style, 0);
    lv_obj_add_style(btn_next, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(btn_next, &ui_button_styles()->style_focus, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_next, &ui_button_styles()->style_focus, LV_STATE_FOCUSED);
    lv_obj_set_style_radius(btn_next, 18, LV_STATE_DEFAULT);
    lv_obj_align(btn_next, LV_ALIGN_CENTER, 54, 0);
    lab_btn_text = lv_label_create(btn_next);
    lv_label_set_text_static(lab_btn_text, LV_SYMBOL_NEXT);
    lv_obj_set_style_text_color(lab_btn_text, lv_color_make(158, 158, 158), LV_STATE_DEFAULT);
    lv_obj_center(lab_btn_text);
    lv_obj_add_event_cb(btn_next, ui_player_page_next_click_cb, LV_EVENT_CLICKED, lab_play_pause);

    lv_obj_t *vol_panel = lv_obj_create(page);
    lv_obj_set_size(vol_panel, 280, 52);
    lv_obj_clear_flag(vol_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(vol_panel, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(vol_panel, LV_OPA_30, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(vol_panel, 20, LV_STATE_DEFAULT);
    lv_obj_align(vol_panel, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_align_to(ctrl_panel, vol_panel, LV_ALIGN_OUT_TOP_LEFT, 0, -20);

    lab_btn_text = lv_label_create(vol_panel);
    lv_label_set_text_static(lab_btn_text, LV_SYMBOL_VOLUME_MAX);
    lv_obj_set_style_text_color(lab_btn_text, lv_color_make(158, 158, 158), LV_STATE_DEFAULT);
    lv_obj_align(lab_btn_text, LV_ALIGN_LEFT_MID, 10, 0);

    sys_param_t *param = settings_get_parameter();
    bsp_codec_volume_set(param->volume, NULL);
    lv_obj_t *vol_bar = lv_bar_create(vol_panel);
    lv_obj_set_size(vol_bar, 100, 4);
    lv_bar_set_range(vol_bar, 0, 10);
    lv_bar_set_value(vol_bar, param->volume / 10, LV_ANIM_OFF);
    lv_obj_align(vol_bar, LV_ALIGN_CENTER, 20, 0);

    lv_obj_t *btn_vol_inc = lv_btn_create(vol_panel);
    lv_obj_set_size(btn_vol_inc, 36, 36);
    lv_obj_add_style(btn_vol_inc, &ui_button_styles()->style, 0);
    lv_obj_add_style(btn_vol_inc, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(btn_vol_inc, &ui_button_styles()->style_focus, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_vol_inc, &ui_button_styles()->style_focus, LV_STATE_FOCUSED);
    lv_obj_set_style_radius(btn_vol_inc, 18, LV_STATE_DEFAULT);
    lv_obj_align(btn_vol_inc, LV_ALIGN_RIGHT_MID, 0, 0);
    lab_btn_text = lv_label_create(btn_vol_inc);
    lv_label_set_text_static(lab_btn_text, LV_SYMBOL_PLUS);
    lv_obj_set_style_text_color(lab_btn_text, lv_color_make(158, 158, 158), LV_STATE_DEFAULT);
    lv_obj_center(lab_btn_text);
    lv_obj_add_event_cb(btn_vol_inc, ui_player_page_vol_inc_click_cb, LV_EVENT_CLICKED, vol_bar);

    lv_obj_t *btn_vol_dec = lv_btn_create(vol_panel);
    lv_obj_set_size(btn_vol_dec, 36, 36);
    lv_obj_add_style(btn_vol_dec, &ui_button_styles()->style, 0);
    lv_obj_add_style(btn_vol_dec, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(btn_vol_dec, &ui_button_styles()->style_focus, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_vol_dec, &ui_button_styles()->style_focus, LV_STATE_FOCUSED);
    lv_obj_set_style_radius(btn_vol_dec, 18, LV_STATE_DEFAULT);
    lv_obj_align(btn_vol_dec, LV_ALIGN_LEFT_MID, 40, 0);
    lab_btn_text = lv_label_create(btn_vol_dec);
    lv_label_set_text_static(lab_btn_text, LV_SYMBOL_MINUS);
    lv_obj_set_style_text_color(lab_btn_text, lv_color_make(158, 158, 158), LV_STATE_DEFAULT);
    lv_obj_center(lab_btn_text);
    lv_obj_add_event_cb(btn_vol_dec, ui_player_page_vol_dec_click_cb, LV_EVENT_CLICKED, vol_bar);

    if (ui_get_btn_op_group()) {
        lv_group_add_obj(ui_get_btn_op_group(), btn_prev);
        lv_group_add_obj(ui_get_btn_op_group(), btn_play_pause);
        lv_group_add_obj(ui_get_btn_op_group(), btn_next);
        lv_group_add_obj(ui_get_btn_op_group(), btn_vol_dec);
        lv_group_add_obj(ui_get_btn_op_group(), btn_vol_inc);
        lv_group_add_obj(ui_get_btn_op_group(), btn_return);
    }
    audio_player_callback_register(audio_cb, NULL);
}
