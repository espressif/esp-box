/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "esp_log.h"

#include "app_ui_ctrl.h"
#include "app_wifi.h"
#include "bsp/esp-bsp.h"

#include "ui_helpers.h"
#include "ui.h"

#define LABEL_WIFI_TEXT                 "Connecting to Wi-Fi\n"
#define LABEL_NOT_WIFI_TEXT                 "Not Connected to Wi-Fi\n"
#define LABEL_WIFI_DOT_COUNT_MAX        (10)
#define WIFI_CHECK_TIMER_INTERVAL_S     (1)
#define REPLY_SCROLL_TIMER_INTERVAL_MS  (1000)
#define REPLY_SCROLL_SPEED              (1)

static char *TAG = "ui_ctrl";

static ui_ctrl_panel_t current_panel = UI_CTRL_PANEL_SLEEP;
static lv_timer_t *scroll_timer_handle = NULL;
static bool reply_audio_start = false;
static bool reply_audio_end = false;
static bool reply_content_get = false;
static uint16_t content_height = 0;

static void reply_content_scroll_timer_handler();
static void wifi_check_timer_handler(lv_timer_t *timer);

void ui_ctrl_init(void)
{
    bsp_display_lock(0);

    ui_init();

    scroll_timer_handle = lv_timer_create(reply_content_scroll_timer_handler, REPLY_SCROLL_TIMER_INTERVAL_MS / REPLY_SCROLL_SPEED, NULL);
    lv_timer_pause(scroll_timer_handle);

    lv_timer_create(wifi_check_timer_handler, WIFI_CHECK_TIMER_INTERVAL_S * 1000, NULL);

    bsp_display_unlock();
}

static void wifi_check_timer_handler(lv_timer_t *timer)
{
    if (WIFI_STATUS_CONNECTED_OK == wifi_connected_already()) {
        lv_obj_clear_flag(ui_PanelSetupSteps, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_PanelSetupWifi, LV_OBJ_FLAG_HIDDEN);
        lv_timer_del(timer);
        if (ui_get_btn_op_group()) {
            lv_group_remove_all_objs(ui_get_btn_op_group());
            lv_group_add_obj(ui_get_btn_op_group(), ui_ButtonSetup);
        }
    } else if (WIFI_STATUS_CONNECTED_FAILED == wifi_connected_already()) {
        lv_label_set_text(ui_LabelSetupWifi, LABEL_NOT_WIFI_TEXT);
    } else {
        if (strlen(lv_label_get_text(ui_LabelSetupWifi)) >= sizeof(LABEL_WIFI_TEXT) + LABEL_WIFI_DOT_COUNT_MAX + 1) {
            lv_label_set_text(ui_LabelSetupWifi, LABEL_WIFI_TEXT);
        } else {
            lv_label_ins_text(ui_LabelSetupWifi, LV_LABEL_POS_LAST, ".");
        }
    }
}

static void show_panel_timer_handler(struct _lv_timer_t *t)
{
    ui_ctrl_panel_t panel = (ui_ctrl_panel_t)t->user_data;
    lv_obj_t *show_panel = NULL;
    lv_obj_t *hide_panel[3] = { NULL };

    switch (panel) {
    case UI_CTRL_PANEL_SLEEP:
        show_panel = ui_PanelSleep;
        hide_panel[0] = ui_PanelListen;
        hide_panel[1] = ui_PanelGet;
        hide_panel[2] = ui_PanelReply;
        lv_obj_clear_flag(ui_ImageListenSettings, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_LabelListenSpeak, " ");
        break;
    case UI_CTRL_PANEL_LISTEN:
        show_panel = ui_PanelListen;
        hide_panel[0] = ui_PanelSleep;
        hide_panel[1] = ui_PanelGet;
        hide_panel[2] = ui_PanelReply;
        lv_obj_clear_flag(ui_LabelListenSpeak, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_LabelListenSpeak, "Listening ...");
        // Reset flags and timer of reply
        reply_content_get = false;
        reply_audio_start = false;
        reply_audio_end = false;
        lv_timer_pause(scroll_timer_handle);
        break;
    case UI_CTRL_PANEL_GET:
        show_panel = ui_PanelGet;
        hide_panel[0] = ui_PanelSleep;
        hide_panel[1] = ui_PanelListen;
        hide_panel[2] = ui_PanelReply;
        lv_obj_clear_flag(ui_LabelListenSpeak, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_LabelListenSpeak, "Thinking ...");
        break;
    case UI_CTRL_PANEL_REPLY:
        show_panel = ui_PanelReply;
        hide_panel[0] = ui_PanelSleep;
        hide_panel[1] = ui_PanelListen;
        hide_panel[2] = ui_PanelGet;
        lv_obj_add_flag(ui_ImageListenSettings, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_ImageListenSettings, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_LabelListenSpeak, LV_OBJ_FLAG_HIDDEN);
        break;
    default:
        break;
    }

    if (panel != UI_CTRL_PANEL_REPLY) {
        lv_obj_clear_flag(ui_ImageListenSettings, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_clear_flag(show_panel, LV_OBJ_FLAG_HIDDEN);
    for (int i = 0; i < sizeof(hide_panel) / sizeof(lv_obj_t *); i++) {
        lv_obj_add_flag(hide_panel[i], LV_OBJ_FLAG_HIDDEN);
    }

    current_panel = panel;

    ESP_LOGI(TAG, "Swich to panel[%d]", panel);
}

void ui_ctrl_show_panel(ui_ctrl_panel_t panel, uint16_t timeout)
{
    bsp_display_lock(0);

    if (timeout) {
        lv_timer_t *timer = lv_timer_create(show_panel_timer_handler, timeout, NULL);
        timer->user_data = (void *)panel;
        lv_timer_set_repeat_count(timer, 1);
        ESP_LOGW(TAG, "Switch panel to [%d] in %dms", panel, timeout);
    } else {
        lv_timer_t timer;
        timer.user_data = (void *)panel;
        show_panel_timer_handler(&timer);
    }

    bsp_display_unlock();
}

static void reply_content_show_text(const char *text)
{
    if (NULL == text) {
        return;
    }

    char *decode = heap_caps_malloc((strlen(text) + 1), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    assert(decode);

    int j = 0;
    for (int i = 0; i < strlen(text);) {
        if ((*(text + i) == '\\') && ((i + 1) < strlen(text)) && (*(text + i + 1) == 'n')) {
            *(decode + j++) = '\n';
            i += 2;
        } else {
            *(decode + j++) = *(text + i);
            i += 1;
        }
    }
    *(decode + j) = '\0';

    ESP_LOGI(TAG, "decode:[%d, %d] %s\r\n", j, strlen(decode), decode);

    lv_label_set_text(ui_LabelReplyContent, decode);
    content_height = lv_obj_get_self_height(ui_LabelReplyContent);
    lv_obj_scroll_to_y(ui_ContainerReplyContent, 0, LV_ANIM_OFF);
    reply_content_get = true;
    lv_timer_resume(scroll_timer_handle);
    ESP_LOGI(TAG, "reply scroll timer start");

    if (decode) {
        free(decode);
    }
}

void ui_ctrl_label_show_text(ui_ctrl_label_t label, const char *text)
{
    bsp_display_lock(0);

    if (text != NULL) {
        switch (label) {
        case UI_CTRL_LABEL_LISTEN_SPEAK:
            ESP_LOGI(TAG, "update listen speak");
            lv_label_set_text(ui_LabelListenSpeak, text);
            break;
        case UI_CTRL_LABEL_REPLY_QUESTION:
            ESP_LOGI(TAG, "update reply question");
            lv_label_set_text(ui_LabelReplyQuestion, text);
            break;
        case UI_CTRL_LABEL_REPLY_CONTENT:
            ESP_LOGI(TAG, "update reply content");
            reply_content_show_text(text);
            break;
        default:
            break;
        }
    }

    bsp_display_unlock();
}

static void anim_callback_set_bg_img_opacity(lv_anim_t *a, int32_t v)
{
    ui_anim_user_data_t *usr = (ui_anim_user_data_t *)a->user_data;
    lv_obj_set_style_bg_img_opa(usr->target, v, 0);
}

static int32_t anim_callback_get_opacity(lv_anim_t *a)
{
    ui_anim_user_data_t *usr = (ui_anim_user_data_t *)a->user_data;
    return lv_obj_get_style_bg_img_opa(usr->target, 0);
}

void ui_sleep_show_animation(void)
{
    bsp_display_lock(0);

    ui_anim_user_data_t *PropertyAnimation_0_user_data = lv_mem_alloc(sizeof(ui_anim_user_data_t));
    PropertyAnimation_0_user_data->target = ui_ContainerBigZ;
    PropertyAnimation_0_user_data->val = -1;
    lv_anim_t PropertyAnimation_0;
    lv_anim_init(&PropertyAnimation_0);
    lv_anim_set_time(&PropertyAnimation_0, 1000);
    lv_anim_set_user_data(&PropertyAnimation_0, PropertyAnimation_0_user_data);
    lv_anim_set_custom_exec_cb(&PropertyAnimation_0, anim_callback_set_bg_img_opacity );
    lv_anim_set_values(&PropertyAnimation_0, 0, 255 );
    lv_anim_set_path_cb( &PropertyAnimation_0, lv_anim_path_linear);
    lv_anim_set_delay( &PropertyAnimation_0, 0 );
    // lv_anim_set_deleted_cb( &PropertyAnimation_0, _ui_anim_callback_free_user_data );
    lv_anim_set_playback_time(&PropertyAnimation_0, 1000);
    lv_anim_set_playback_delay(&PropertyAnimation_0, 0);
    lv_anim_set_repeat_count(&PropertyAnimation_0, LV_ANIM_REPEAT_INFINITE );
    lv_anim_set_repeat_delay(&PropertyAnimation_0, 1000);
    lv_anim_set_early_apply( &PropertyAnimation_0, false );
    lv_anim_set_get_value_cb(&PropertyAnimation_0, &anim_callback_get_opacity );
    lv_anim_start(&PropertyAnimation_0);

    ui_anim_user_data_t *PropertyAnimation_1_user_data = lv_mem_alloc(sizeof(ui_anim_user_data_t));
    PropertyAnimation_1_user_data->target = ui_ContainerSmallZ;
    PropertyAnimation_1_user_data->val = -1;
    lv_anim_t PropertyAnimation_1;
    lv_anim_init(&PropertyAnimation_1);
    lv_anim_set_time(&PropertyAnimation_1, 1000);
    lv_anim_set_user_data(&PropertyAnimation_1, PropertyAnimation_1_user_data);
    lv_anim_set_custom_exec_cb(&PropertyAnimation_1, anim_callback_set_bg_img_opacity );
    lv_anim_set_values(&PropertyAnimation_1, 0, 255 );
    lv_anim_set_path_cb( &PropertyAnimation_1, lv_anim_path_linear);
    lv_anim_set_delay( &PropertyAnimation_1, 1000 );
    // lv_anim_set_deleted_cb( &PropertyAnimation_1, _ui_anim_callback_free_user_data );
    lv_anim_set_playback_time(&PropertyAnimation_1, 1000);
    lv_anim_set_playback_delay(&PropertyAnimation_1, 0);
    lv_anim_set_repeat_count(&PropertyAnimation_1, LV_ANIM_REPEAT_INFINITE );
    lv_anim_set_repeat_delay(&PropertyAnimation_1, 1000);
    lv_anim_set_early_apply( &PropertyAnimation_1, false );
    lv_anim_set_get_value_cb(&PropertyAnimation_1, &anim_callback_get_opacity );
    lv_anim_start(&PropertyAnimation_1);

    bsp_display_unlock();
}

void ui_ctrl_reply_set_audio_start_flag(bool result)
{
    reply_audio_start = result;
}

bool ui_ctrl_reply_get_audio_start_flag(void)
{
    return reply_audio_start;
}

void ui_ctrl_reply_set_audio_end_flag(bool result)
{
    reply_audio_end = result;
}

static void reply_content_scroll_timer_handler()
{
    lv_coord_t offset = 0;
    const lv_font_t *font = NULL;

    if (reply_content_get && reply_audio_start) {
        font = lv_obj_get_style_text_font(ui_LabelReplyContent, 0);
        offset = lv_obj_get_scroll_y(ui_ContainerReplyContent);
        // ESP_LOGI(TAG, "offset: %d, content_height: %d, font_height: %d", offset, content_height, font->line_height);
        if ((content_height > lv_obj_get_height(ui_ContainerReplyContent)) &&
                (offset < (content_height - lv_obj_get_height(ui_ContainerReplyContent)))) {
            offset += font->line_height / 2;
            lv_obj_scroll_to_y(ui_ContainerReplyContent, offset, LV_ANIM_OFF);
        } else if (reply_audio_end) {
            ESP_LOGI(TAG, "reply scroll timer stop");
            reply_content_get = false;
            reply_audio_start = false;
            reply_audio_end = false;
            lv_timer_pause(scroll_timer_handle);
            ui_ctrl_show_panel(UI_CTRL_PANEL_SLEEP, 1000);
        }
    }
}

void ui_ctrl_guide_jump( void )
{
    lv_obj_t *act_scr = lv_scr_act();
    if (act_scr == ui_ScreenSetup) {
        ESP_LOGI(TAG, "act_scr:%p, ui_ScreenSetup:%p", act_scr, ui_ScreenSetup);
        lv_event_send(ui_ButtonSetup, LV_EVENT_CLICKED, 0);
    }
}
