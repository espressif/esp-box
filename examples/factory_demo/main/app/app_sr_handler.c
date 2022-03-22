/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_check.h"
#include "app_led.h"
#include "app_sr.h"
#include "file_manager.h"
#include "app_player.h"
#include "driver/i2s.h"
#include "bsp_board.h"
#include "bsp_codec.h"
#include "ui_sr.h"
#include "app_sr_handler.h"
#include "settings.h"


static const char *TAG = "sr_handler";

static bool b_audio_playing = false;

typedef enum {
    AUDIO_WAKE,
    AUDIO_OK,
    AUDIO_END,
    AUDIO_MAX,
} audio_segment_t;

typedef struct {
    uint8_t *audio_buffer;
    size_t len;
} audio_data_t;

static audio_data_t g_audio_data[AUDIO_MAX];

static esp_err_t sr_echo_play(audio_segment_t audio)
{
    bsp_board_power_ctrl(POWER_MODULE_AUDIO, false); // turn off the speaker to avoid play some noise

    typedef struct {
        // The "RIFF" chunk descriptor
        uint8_t ChunkID[4];
        int32_t ChunkSize;
        uint8_t Format[4];
        // The "fmt" sub-chunk
        uint8_t Subchunk1ID[4];
        int32_t Subchunk1Size;
        int16_t AudioFormat;
        int16_t NumChannels;
        int32_t SampleRate;
        int32_t ByteRate;
        int16_t BlockAlign;
        int16_t BitsPerSample;
        // The "data" sub-chunk
        uint8_t Subchunk2ID[4];
        int32_t Subchunk2Size;
    } wav_header_t;

    /**
     * read head of WAV file
     */
    uint8_t *p = g_audio_data[audio].audio_buffer;
    wav_header_t *wav_head = (wav_header_t *)p;
    if (NULL == strstr((char *)wav_head->Subchunk1ID, "fmt") &&
            NULL == strstr((char *)wav_head->Subchunk2ID, "data")) {
        ESP_LOGE(TAG, "Header of wav format error");
        return ESP_FAIL;
    }
    p += sizeof(wav_header_t);
    size_t len = g_audio_data[audio].len - sizeof(wav_header_t);
    len = len & 0xfffffffc;
    ESP_LOGD(TAG, "frame_rate=%d, ch=%d, width=%d", wav_head->SampleRate, wav_head->NumChannels, wav_head->BitsPerSample);

    i2s_zero_dma_buffer(I2S_NUM_0);
    vTaskDelay(pdMS_TO_TICKS(50));
    bsp_board_power_ctrl(POWER_MODULE_AUDIO, true);// turn on the speaker
    size_t bytes_written = 0;
    b_audio_playing = true;
    i2s_write(I2S_NUM_0, p, len, &bytes_written, portMAX_DELAY);
    i2s_zero_dma_buffer(I2S_NUM_0);
    vTaskDelay(pdMS_TO_TICKS(20));
    b_audio_playing = false;
    return ESP_OK;
}

bool sr_echo_is_playing(void)
{
    return b_audio_playing;
}

void sr_handler_task(void *pvParam)
{
    esp_err_t ret;
    FILE *fp;
    const sys_param_t *param = settings_get_parameter();
    const char *files[2][3] = {
        {"/spiffs/echo_en_wake.wav", "/spiffs/echo_en_ok.wav", "/spiffs/echo_en_end.wav"},
        {"/spiffs/echo_cn_wake.wav", "/spiffs/echo_cn_ok.wav", "/spiffs/echo_cn_end.wav"},
    };
    char audio_file[48] = {0};
    for (size_t i = 0; i < AUDIO_MAX; i++) {
        strncpy(audio_file, files[param->sr_lang][i], sizeof(audio_file));
        fp = fopen(audio_file, "rb");
        ESP_GOTO_ON_FALSE(NULL != fp, ESP_ERR_NOT_FOUND, err, TAG, "Open file %s failed", audio_file);
        size_t file_size = fm_get_file_size(audio_file);
        g_audio_data[i].len = file_size;
        g_audio_data[i].audio_buffer = heap_caps_calloc(1, file_size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
        ESP_GOTO_ON_FALSE(NULL != g_audio_data[i].audio_buffer, ESP_ERR_NO_MEM, err, TAG,  "No mem for sr echo buffer");
        fread(g_audio_data[i].audio_buffer, 1, file_size, fp);
        fclose(fp);
    }
    (void)ret;

    player_state_t last_player_state = PLAYER_STATE_IDLE;
    while (true) {
        sr_result_t result;
        app_sr_get_result(&result, portMAX_DELAY);
        char audio_file[48] = {0};

        if (ESP_MN_STATE_TIMEOUT == result.state) {
            if (SR_LANG_EN == param->sr_lang) {
                sr_anim_set_text("Timeout");
            } else {
                sr_anim_set_text("超时");
            }
#if !SR_RUN_TEST
            sr_echo_play(AUDIO_END);
#endif
            sr_anim_stop();
            if (PLAYER_STATE_PLAYING == last_player_state) {
                app_player_play();
            }
            continue;
        }

        if (AFE_FETCH_WWE_DETECTED == result.fetch_mode) {
            sr_anim_start();
            last_player_state = app_player_get_state();
            app_player_pause();

            if (SR_LANG_EN == param->sr_lang) {
                sr_anim_set_text("Say command");
            } else {
                sr_anim_set_text("请说");
            }
#if !SR_RUN_TEST
            sr_echo_play(AUDIO_WAKE);
#endif
            continue;
        }

        if (ESP_MN_STATE_DETECTED & result.state) {
            const sr_cmd_t *cmd = app_sr_get_cmd_from_id(result.command_id);
            ESP_LOGI(TAG, "command:%s, act:%d", cmd->str, cmd->cmd);
            sr_anim_set_text((char *) cmd->str);
#if !SR_CONTINUE_DET
            sr_anim_stop();
            if (PLAYER_STATE_PLAYING == last_player_state) {
                app_player_play();
            }
#endif

            switch (cmd->cmd) {
            case SR_CMD_SET_RED:
                app_pwm_led_set_all(128, 0, 0);
                break;
            case SR_CMD_SET_GREEN:
                app_pwm_led_set_all(0, 128, 0);
                break;
            case SR_CMD_SET_BLUE:
                app_pwm_led_set_all(0, 0, 128);
                break;
            case SR_CMD_LIGHT_ON:
                app_pwm_led_set_power(1);
                break;
            case SR_CMD_LIGHT_OFF:
                app_pwm_led_set_power(0);
                break;
            case SR_CMD_CUSTOMIZE_COLOR: {
                uint16_t h;
                uint8_t s, v;
                app_pwm_led_get_customize_color(&h, &s, &v);
                app_pwm_led_set_all_hsv(h, s, v);
            } break;
            case SR_CMD_NEXT:
                app_player_play_next();
                break;
            case SR_CMD_PLAY:
                app_player_play();
                last_player_state = PLAYER_STATE_PLAYING;
                break;
            case SR_CMD_PAUSE:
                app_player_pause();
                last_player_state = PLAYER_STATE_PAUSE;
                break;
            default:
                ESP_LOGE(TAG, "Unknow cmd");
                break;
            }
#if !SR_RUN_TEST
            if (SR_LANG_EN == param->sr_lang) {
                strncpy(audio_file, "/spiffs/echo_en_ok.wav", sizeof(audio_file));
            } else {
                strncpy(audio_file, "/spiffs/echo_cn_ok.wav", sizeof(audio_file));
            }
            sr_echo_play(AUDIO_OK);
#endif
        }
    }

err:
    if (fp) {
        fclose(fp);
    }
    vTaskDelete(NULL);
}
