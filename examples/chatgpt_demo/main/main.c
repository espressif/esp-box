/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "app_ui_ctrl.h"
#include "OpenAI.h"
#include "tts_api.h"
#include "app_sr.h"
#include "bsp/esp-bsp.h"
#include "bsp_board.h"
#include "app_audio.h"
#include "app_wifi.h"
#include "settings.h"

#define SCROLL_START_DELAY_S      (2)
#define OPENAI_API_KEY    CONFIG_OPENAI_API_KEY

static char *TAG = "app_main";
format_t file_type = FORMAT_WAV;

/* program flow. This function is called in app_audio.c */

esp_err_t start_openai(uint8_t *audio, int audio_len)
{
    sys_param_t *sys_set = settings_get_parameter();
    switch (sys_set->current_server) {
        case REGION_Espressif:
            ui_ctrl_show_panel(UI_CTRL_PANEL_GET, 0);
            esp_err_t err = create_whisper_request_from_record(audio, audio_len);
            if ((err == ESP_ERR_INVALID_ARG) || (strcmp(get_message_content_for_chatgpt(), "invalid_request_error") == 0)) {
                    // UI listen fail
                ui_ctrl_label_show_text(UI_CTRL_LABEL_LISTEN_SPEAK, "Sorry, I can't understand.");
                ui_ctrl_show_panel(UI_CTRL_PANEL_SLEEP, 2000);
                return ESP_FAIL;
            } else {
                ui_ctrl_label_show_text(UI_CTRL_LABEL_REPLY_QUESTION, get_message_content_for_whisper());
                ui_ctrl_label_show_text(UI_CTRL_LABEL_REPLY_CONTENT, get_message_content_for_chatgpt());
                ui_ctrl_show_panel(UI_CTRL_PANEL_REPLY, 0);
                ESP_LOGI(TAG, "create_TTS_request");
                esp_err_t status = text_to_speech_request(get_message_content_for_chatgpt(), AUDIO_CODECS_MP3);
                if (status != ESP_OK) {
                    ESP_LOGE(TAG, "Error creating ChatGPT request: %s\n", esp_err_to_name(status));
                    // UI reply audio fail
                    ui_ctrl_show_panel(UI_CTRL_PANEL_SLEEP, 0);
                } else {
                    ESP_LOGI(TAG, "reply audio start");
                    // Wait a moment before starting to scroll the reply content
                    vTaskDelay(pdMS_TO_TICKS(SCROLL_START_DELAY_S * 1000));
                    ui_ctrl_reply_set_audio_start_flag(true);
                }
            }
            break;
        case REGION_OpenAI:
            ui_ctrl_show_panel(UI_CTRL_PANEL_GET, 0);
            esp_err_t err2 = create_whisper_request_from_record(audio, audio_len);
            if ((err2 == ESP_ERR_INVALID_ARG) || strcmp(get_message_content_for_chatgpt(), "invalid_request_error") == 0 || strcmp(get_message_content_for_chatgpt(), "server_error") == 0) {
                // UI listen fail
                ui_ctrl_label_show_text(UI_CTRL_LABEL_LISTEN_SPEAK, "Sorry, I can't understand.");
                ui_ctrl_show_panel(UI_CTRL_PANEL_SLEEP, 2000);
                return ESP_FAIL;
            } else {
                ESP_LOGI(TAG, "create_chatgpt_request: %s", get_message_content_for_chatgpt());

                // UI listen success
                ui_ctrl_label_show_text(UI_CTRL_LABEL_REPLY_QUESTION, get_message_content_for_chatgpt());
                ui_ctrl_label_show_text(UI_CTRL_LABEL_LISTEN_SPEAK, get_message_content_for_chatgpt());

                esp_err_t status = create_chatgpt_request(get_message_content_for_chatgpt());
                if (status != ESP_OK) {
                    ESP_LOGE(TAG, "Error creating ChatGPT request: %s\n", esp_err_to_name(status));
                }
            } if (strcmp(get_message_content_for_chatgpt(), "invalid_request_error") == 0) {

                // UI reply fail
                ui_ctrl_label_show_text(UI_CTRL_LABEL_LISTEN_SPEAK, "Sorry, I can't understand.");
                ui_ctrl_show_panel(UI_CTRL_PANEL_SLEEP, 2000);

                return ESP_FAIL;
            } else {
                // UI reply content
                ui_ctrl_label_show_text(UI_CTRL_LABEL_REPLY_CONTENT, get_message_content_for_chatgpt());
                ui_ctrl_show_panel(UI_CTRL_PANEL_REPLY, 0);
                esp_err_t status = text_to_speech_request(get_message_content_for_chatgpt(), AUDIO_CODECS_MP3);
                if (status != ESP_OK) {
                    ESP_LOGE(TAG, "Error creating ChatGPT request: %s\n", esp_err_to_name(status));
                    // UI reply audio fail
                    ui_ctrl_show_panel(UI_CTRL_PANEL_SLEEP, 0);
                } else {
                    ESP_LOGI(TAG, "replay audio start");
                    // Wait a moment before start to scroll the reply content
                    vTaskDelay(pdMS_TO_TICKS(SCROLL_START_DELAY_S * 1000));
                    ui_ctrl_reply_set_audio_start_flag(true);
                }
            }
            break;
        default:
            // Handle other server regions if necessary
            break;
        }
        return ESP_OK;
}

/* play audio function */

static void audio_play_finish_cb(void)
{
    ESP_LOGI(TAG, "replay audio end");
    if (ui_ctrl_reply_get_audio_start_flag()) {
        ui_ctrl_reply_set_audio_end_flag(true);
    }
}

void app_main()
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(settings_read_parameter_from_nvs());

    bsp_spiffs_mount();

    bsp_i2c_init();
    bsp_display_start();
    bsp_board_init();

    ESP_LOGI(TAG, "Display LVGL demo");
    bsp_display_backlight_on();
    ui_ctrl_init();

    app_network_start();

    sys_param_t *sys_set = settings_get_parameter();
    set_api_key(OPENAI_API_KEY);
    set_server(sys_set->current_server);
    set_audio_type(file_type);
    ESP_LOGI(TAG, "speech recognition start");
    app_sr_start(false);
    audio_register_play_finish_cb(audio_play_finish_cb);

    while (true) {
        ESP_LOGD(TAG, "\tDescription\tInternal\tSPIRAM");
        ESP_LOGD(TAG, "Current Free Memory\t%d\t\t%d",
                 heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
                 heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        ESP_LOGD(TAG, "Min. Ever Free Size\t%d\t\t%d",
                 heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
                 heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM));
        vTaskDelay(pdMS_TO_TICKS(5 * 1000));
    }
}
