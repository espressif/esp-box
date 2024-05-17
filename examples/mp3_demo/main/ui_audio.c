/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "lvgl.h"
#include "audio_player.h"
#include "file_iterator.h"
#include "esp_err.h"
#include "esp_log.h"
#include "bsp_board.h"
#include "usb/uac_host.h"
#include "mp3_demo.h"

static const char *TAG = "ui_audio";

typedef struct {
    lv_style_t style_bg;
    lv_style_t style_focus_no_outline;
} button_style_t;

#if CONFIG_BSP_BOARD_ESP32_S3_BOX_Lite
static lv_obj_t *g_group_list[3] = {0};
static lv_obj_t *g_focus_last_obj = NULL;
#endif
static lv_group_t *g_btn_op_group = NULL;
static file_iterator_instance_t *file_iterator;
static uint8_t g_sys_volume;
static button_style_t g_btn_styles;

uint8_t get_sys_volume()
{
    return g_sys_volume;
}

uint8_t get_current_music_index()
{
    return file_iterator_get_index(file_iterator);
}

button_style_t *ui_button_styles(void)
{
    return &g_btn_styles;
}

lv_group_t *ui_get_btn_op_group(void)
{
    return g_btn_op_group;
}

static void ui_button_style_init(void)
{
    /*Init the style for the default state*/
    lv_style_init(&g_btn_styles.style_focus_no_outline);
    lv_style_set_outline_width(&g_btn_styles.style_focus_no_outline, 0);

    lv_style_init(&g_btn_styles.style_bg);
    lv_style_set_bg_opa(&g_btn_styles.style_bg, LV_OPA_100);
    lv_style_set_bg_color(&g_btn_styles.style_bg, lv_color_make(255, 255, 255));
    lv_style_set_shadow_width(&g_btn_styles.style_bg, 0);
}

static void play_index(int index)
{
    ESP_LOGI(TAG, "play_index(%d)", index);

    char filename[128];
    int retval = file_iterator_get_full_path_from_index(file_iterator, index, filename, sizeof(filename));
    if (retval == 0) {
        ESP_LOGE(TAG, "unable to retrieve filename");
        return;
    }

    FILE *fp = fopen(filename, "rb");
    if (fp) {
        ESP_LOGI(TAG, "Playing '%s'", filename);
        audio_player_play(fp);
    } else {
        ESP_LOGE(TAG, "unable to open index %d, filename '%s'", index, filename);
    }
}

#if CONFIG_BSP_BOARD_ESP32_S3_BOX_Lite

static uint32_t music_list_get_num_offset(uint32_t focus, int32_t max, int32_t offset)
{
    if (focus >= max) {
        ESP_LOGI(TAG, "[ERROR] focus should less than max");
        return focus;
    }

    uint32_t i;
    if (offset >= 0) {
        i = (focus + offset) % max;
    } else {
        offset = max + (offset % max);
        i = (focus + offset) % max;
    }
    return i;
}

static int8_t music_list_direct_probe(lv_obj_t *focus_obj)
{
    int8_t direct;
    uint32_t index_max_sz, index_focus, index_prev;

    index_focus = 0;
    index_prev = 0;
    index_max_sz = sizeof(g_group_list) / sizeof(g_group_list[0]);

    for (int i = 0; i < index_max_sz; i++) {
        if (focus_obj == g_group_list[i]) {
            index_focus = i;
        }
        if (g_focus_last_obj == g_group_list[i]) {
            index_prev = i;
        }
    }

    if (NULL == g_focus_last_obj) {
        direct = 0;
    } else if (index_focus == music_list_get_num_offset(index_prev, index_max_sz, 1)) {
        direct = 1;
    } else if (index_focus == music_list_get_num_offset(index_prev, index_max_sz, -1)) {
        direct = -1;
    } else {
        direct = 0;
    }

    g_focus_last_obj = focus_obj;
    return direct;
}

void music_list_new_item_select(lv_obj_t *obj)
{
    int8_t direct = music_list_direct_probe(obj);

    int item_size = file_iterator_get_count(file_iterator);
    int item_index = file_iterator_get_index(file_iterator);
    item_index = music_list_get_num_offset(item_index, item_size, direct);
    ESP_LOGI(TAG, "selected:[%d/%d], direct:%d", item_index, item_size, direct);

    if (1 == direct) {
        file_iterator_next(file_iterator);
    } else if (-1 == direct) {
        file_iterator_prev(file_iterator);
    } else {
        return;
    }

    audio_player_state_t state = audio_player_get_state();
    if (state == AUDIO_PLAYER_STATE_IDLE) {
        // nothing to do, changing songs while not playing
        // doesn't start or stop playback
        ESP_LOGI(TAG, "idle, nothing to do");
    } else if (state == AUDIO_PLAYER_STATE_PLAYING) {
        int index = file_iterator_get_index(file_iterator);
        play_index(index);
    }
}

static void music_list_prev_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_user_data(e);

    if (LV_EVENT_FOCUSED == code) {
        music_list_new_item_select(obj);
    } else if (LV_EVENT_CLICKED == code) {
        lv_event_send(g_group_list[1], LV_EVENT_CLICKED, g_group_list[1]);
    }
}

static void music_list_next_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_user_data(e);

    if (LV_EVENT_FOCUSED == code) {
        music_list_new_item_select(obj);
    } else if (LV_EVENT_CLICKED == code) {
        lv_event_send(g_group_list[1], LV_EVENT_CLICKED, g_group_list[1]);
    }
}

static void music_play_pause_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_user_data(e);

    if (LV_EVENT_FOCUSED == code) {
        ESP_LOGI(TAG, "##play focus, %02X,%02X,%02X", g_group_list[0]->state, g_group_list[1]->state, g_group_list[2]->state);
        music_list_new_item_select(obj);
    } else if (LV_EVENT_CLICKED == code) {
        ESP_LOGI(TAG, "##play click, %02X,%02X,%02X", g_group_list[0]->state, g_group_list[1]->state, g_group_list[2]->state);
        lv_obj_t *lab = lv_obj_get_child(obj, 0);
        ESP_LOGI(TAG, "[play/pause] = %d", file_iterator_get_index(file_iterator));

        audio_player_state_t state = audio_player_get_state();
        if (state == AUDIO_PLAYER_STATE_PAUSE) {
            lv_label_set_text_static(lab, LV_SYMBOL_PAUSE);
            audio_player_resume();
        } else if (state == AUDIO_PLAYER_STATE_PLAYING) {
            lv_label_set_text_static(lab, LV_SYMBOL_PLAY);
            audio_player_pause();
        }
    }
}
#else
static void btn_play_pause_cb(lv_event_t *event)
{
    lv_obj_t *btn = lv_event_get_target(event);
    lv_obj_t *lab = (lv_obj_t *) btn->user_data;

    audio_player_state_t state = audio_player_get_state();
    if (state == AUDIO_PLAYER_STATE_PAUSE) {
        bsp_display_lock(0);
        lv_label_set_text_static(lab, LV_SYMBOL_PAUSE);
        bsp_display_unlock();
        audio_player_resume();
    } else if (state == AUDIO_PLAYER_STATE_PLAYING) {
        bsp_display_lock(0);
        lv_label_set_text_static(lab, LV_SYMBOL_PLAY);
        bsp_display_unlock();
        audio_player_pause();
    }
}

static void btn_prev_next_cb(lv_event_t *event)
{
    bool is_next = (bool) event->user_data;

    if (is_next) {
        ESP_LOGI(TAG, "btn next");
        file_iterator_next(file_iterator);
    } else {
        ESP_LOGI(TAG, "btn prev");
        file_iterator_prev(file_iterator);
    }

    audio_player_state_t state = audio_player_get_state();
    if (state == AUDIO_PLAYER_STATE_IDLE) {
        // nothing to do, changing songs while not playing
        // doesn't start or stop playback
        ESP_LOGI(TAG, "idle, nothing to do");
    } else if (state == AUDIO_PLAYER_STATE_PLAYING) {
        int index = file_iterator_get_index(file_iterator);
        ESP_LOGI(TAG, "playing index '%d'", index);
        play_index(index);
    }
}
#endif

static void volume_slider_cb(lv_event_t *event)
{
    lv_obj_t *slider = lv_event_get_target(event);
    int volume = lv_slider_get_value(slider);
    if (get_audio_player_type() == AUDIO_PLAYER_I2S) {
        bsp_codec_volume_set(volume, NULL);
    } else {
        uac_host_device_set_volume(get_audio_player_handle(), volume);
    }
    g_sys_volume = volume;
    ESP_LOGI(TAG, "volume '%d'", volume);
}

static void build_file_list(lv_obj_t *music_list)
{
    lv_obj_t *label_title = (lv_obj_t *) music_list->user_data;

    bsp_display_lock(0);
    lv_dropdown_clear_options(music_list);
    bsp_display_unlock();

    size_t i = 0;
    while (true) {
        const char *file_name = file_iterator_get_name_from_index(file_iterator, i);
        if (NULL != file_name) {
            bsp_display_lock(0);
            lv_dropdown_add_option(music_list, file_name, i);
            bsp_display_unlock();
        } else {
            bsp_display_lock(0);
            lv_dropdown_set_selected(music_list, 0);
            lv_label_set_text_static(label_title,
                                     file_iterator_get_name_from_index(file_iterator, 0));
            bsp_display_unlock();
            break;
        }
        i++;
    }
}

static void audio_callback(audio_player_cb_ctx_t *ctx)
{
    lv_obj_t *music_list = (lv_obj_t *) ctx->user_ctx;
    lv_obj_t *label_title = (lv_obj_t *) music_list->user_data;
    lv_obj_t *btn_play_pause = (lv_obj_t *) label_title->user_data;
    lv_obj_t *label_play_pause = (lv_obj_t *) btn_play_pause->user_data;

    if (ctx->audio_event == AUDIO_PLAYER_CALLBACK_EVENT_IDLE) {
        ESP_LOGI(TAG, "audio_callback IDLE");
        file_iterator_next(file_iterator);
        play_index(file_iterator_get_index(file_iterator));
    }

    size_t index = file_iterator_get_index(file_iterator);

    lv_dropdown_set_selected(music_list, index);

    bsp_display_lock(0);
    lv_label_set_text_static(label_title,
                             file_iterator_get_name_from_index(file_iterator, index));

    if ((ctx->audio_event == AUDIO_PLAYER_CALLBACK_EVENT_PLAYING) ||
            (ctx->audio_event == AUDIO_PLAYER_CALLBACK_EVENT_COMPLETED_PLAYING_NEXT)) {
        lv_obj_clear_state(btn_play_pause, LV_STATE_CHECKED);
        lv_label_set_text_static(label_play_pause, LV_SYMBOL_PAUSE);
    } else {
        lv_obj_add_state(btn_play_pause, LV_STATE_CHECKED);
        lv_label_set_text_static(label_play_pause, LV_SYMBOL_PLAY);
    }

    lv_obj_invalidate(btn_play_pause);
    bsp_display_unlock();
}

static void music_list_cb(lv_event_t *event)
{
    lv_obj_t *music_list = lv_event_get_target(event);
    if (audio_player_get_state() == AUDIO_PLAYER_STATE_PLAYING) {
        uint16_t selected = lv_dropdown_get_selected(music_list);
        ESP_LOGI(TAG, "switching index to '%d'", selected);
        file_iterator_set_index(file_iterator, selected);
        play_index(selected);
    }
}

void ui_audio_start(file_iterator_instance_t *i)
{
    file_iterator = i;
    g_sys_volume = 60;

    ui_button_style_init();

    lv_indev_t *indev = lv_indev_get_next(NULL);

    if (lv_indev_get_type(indev) == LV_INDEV_TYPE_ENCODER) {
        ESP_LOGI(TAG, "Input device type is encoder");
        g_btn_op_group = lv_group_create();
        lv_indev_set_group(indev, g_btn_op_group);
    }

    /* Create audio control button */
    lv_obj_t *btn_play_pause = lv_btn_create(lv_scr_act());
    lv_obj_align(btn_play_pause, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_size(btn_play_pause, 50, 50);
    lv_obj_set_style_radius(btn_play_pause, 25, LV_STATE_DEFAULT);
    lv_obj_add_flag(btn_play_pause, LV_OBJ_FLAG_CHECKABLE);

    lv_obj_add_style(btn_play_pause, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_play_pause, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);

    lv_obj_t *label_play_pause = lv_label_create(btn_play_pause);
    lv_label_set_text_static(label_play_pause, LV_SYMBOL_PAUSE);
    lv_obj_center(label_play_pause);
    lv_obj_set_user_data(btn_play_pause, (void *) label_play_pause);
#if CONFIG_BSP_BOARD_ESP32_S3_BOX_Lite
    lv_obj_add_event_cb(btn_play_pause, music_play_pause_cb, LV_EVENT_ALL, (void *) btn_play_pause);
#else
    lv_obj_add_event_cb(btn_play_pause, btn_play_pause_cb, LV_EVENT_VALUE_CHANGED, NULL);
#endif

    lv_obj_t *btn_play_prev = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn_play_prev, 50, 50);
    lv_obj_set_style_radius(btn_play_prev, 25, LV_STATE_DEFAULT);
    lv_obj_clear_flag(btn_play_prev, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_align_to(btn_play_prev, btn_play_pause, LV_ALIGN_OUT_LEFT_MID, -40, 0);

    lv_obj_add_style(btn_play_prev, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_play_prev, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);
    lv_obj_add_style(btn_play_prev, &ui_button_styles()->style_bg, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_play_prev, &ui_button_styles()->style_bg, LV_STATE_FOCUSED);
    lv_obj_add_style(btn_play_prev, &ui_button_styles()->style_bg, LV_STATE_DEFAULT);

    lv_obj_t *label_prev = lv_label_create(btn_play_prev);
    lv_label_set_text_static(label_prev, LV_SYMBOL_PREV);
    lv_obj_set_style_text_font(label_prev, &lv_font_montserrat_24, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_prev, lv_color_make(0, 0, 0), LV_STATE_DEFAULT);
    lv_obj_center(label_prev);
    lv_obj_set_user_data(btn_play_prev, (void *) label_prev);

#if CONFIG_BSP_BOARD_ESP32_S3_BOX_Lite
    g_group_list[0] = btn_play_prev;
    lv_obj_add_event_cb(btn_play_prev, music_list_prev_cb, LV_EVENT_ALL, (void *) btn_play_prev);
    if (ui_get_btn_op_group()) {
        lv_group_add_obj(ui_get_btn_op_group(), btn_play_prev);
    }
#else
    lv_obj_add_event_cb(btn_play_prev, btn_prev_next_cb, LV_EVENT_CLICKED, (void *) false);
#endif

#if CONFIG_BSP_BOARD_ESP32_S3_BOX_Lite
    g_group_list[1] = btn_play_pause;
    if (ui_get_btn_op_group()) {
        lv_group_add_obj(ui_get_btn_op_group(), btn_play_pause);
    }
#endif

    lv_obj_t *btn_play_next = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn_play_next, 50, 50);
    lv_obj_set_style_radius(btn_play_next, 25, LV_STATE_DEFAULT);
    lv_obj_clear_flag(btn_play_next, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_align_to(btn_play_next, btn_play_pause, LV_ALIGN_OUT_RIGHT_MID, 40, 0);

    lv_obj_add_style(btn_play_next, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_play_next, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);
    lv_obj_add_style(btn_play_next, &ui_button_styles()->style_bg, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_play_next, &ui_button_styles()->style_bg, LV_STATE_FOCUSED);
    lv_obj_add_style(btn_play_next, &ui_button_styles()->style_bg, LV_STATE_DEFAULT);

    lv_obj_t *label_next = lv_label_create(btn_play_next);
    lv_label_set_text_static(label_next, LV_SYMBOL_NEXT);
    lv_obj_set_style_text_font(label_next, &lv_font_montserrat_24, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_next, lv_color_make(0, 0, 0), LV_STATE_DEFAULT);
    lv_obj_center(label_next);
    lv_obj_set_user_data(btn_play_next, (void *) label_next);

#if CONFIG_BSP_BOARD_ESP32_S3_BOX_Lite
    g_group_list[2] = btn_play_next;
    lv_obj_add_event_cb(btn_play_next, music_list_next_cb, LV_EVENT_ALL, (void *) btn_play_next);
    if (ui_get_btn_op_group()) {
        lv_group_add_obj(ui_get_btn_op_group(), btn_play_next);
    }
#else
    lv_obj_add_event_cb(btn_play_next, btn_prev_next_cb, LV_EVENT_CLICKED, (void *) true);
#endif

    /* Create volume slider */
    lv_obj_t *volume_slider = lv_slider_create(lv_scr_act());
    lv_obj_set_size(volume_slider, 200, 6);
    lv_obj_set_ext_click_area(volume_slider, 15);
    lv_obj_align(volume_slider, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_slider_set_range(volume_slider, 0, 100);
    lv_slider_set_value(volume_slider, g_sys_volume, LV_ANIM_ON);
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
    lv_label_set_text_static(lab_title, "Scanning Files...");
    lv_obj_set_style_text_font(lab_title, &lv_font_montserrat_32, LV_STATE_DEFAULT);
    lv_obj_align(lab_title, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *music_list = lv_dropdown_create(lv_scr_act());
    lv_dropdown_clear_options(music_list);
    lv_dropdown_set_options_static(music_list, "Scanning...");
    lv_obj_set_width(music_list, 200);
    lv_obj_align_to(music_list, lab_title, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_set_user_data(music_list, (void *) lab_title);
    lv_obj_add_event_cb(music_list, music_list_cb, LV_EVENT_VALUE_CHANGED, NULL);

    build_file_list(music_list);
    audio_player_callback_register(audio_callback, (void *) music_list);

    // initiate playback
    play_index(file_iterator_get_index(file_iterator));
}
