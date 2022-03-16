/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <dirent.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include "driver/i2s.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "app_player.h"
#include "bsp_i2s.h"
#include "bsp_codec.h"
#include "mp3dec.h"
#include "settings.h"
#include "file_manager.h"

static const char *TAG = "player";

/* **************** AUDIO CALLBACK **************** */
static audio_cb_t s_audio_cb = NULL;
static void *audio_cb_usrt_ctx = NULL;
static QueueHandle_t audio_event_queue = NULL;
player_state_t g_player_state;

static uint16_t g_file_num = 0;
static uint16_t audio_index = 0;
static char **g_file_list = NULL;

esp_err_t app_player_callback_register(audio_cb_t call_back, void *user_ctx)
{
    s_audio_cb = call_back;
    audio_cb_usrt_ctx = user_ctx;

    return ESP_OK;
}


size_t app_player_get_index(void)
{
    return audio_index;
}

const char *app_player_get_name_from_index(size_t index)
{
    ESP_RETURN_ON_FALSE(index < g_file_num, NULL, TAG, "File index out of range");
    ESP_RETURN_ON_FALSE(NULL != g_file_list, NULL, TAG, "Audio file not found");
    ESP_RETURN_ON_FALSE(NULL != g_file_list[index], NULL, TAG, "Audio file not found");

    return g_file_list[index];
}


static esp_err_t play_mp3(const char *path)
{
    ESP_LOGI(TAG, "start to decode %s", path);

    FILE *fp = NULL;
    int sample_rate = 0;
    uint32_t bits_cfg = 0;
    uint32_t nChans = 0;
    esp_err_t ret = ESP_OK;
    uint8_t *output = NULL;
    uint8_t *read_buf = NULL;
    MP3FrameInfo frame_info;
    HMP3Decoder mp3_decoder = MP3InitDecoder();
    player_event_t audio_event = AUDIO_EVENT_NONE;

    ESP_RETURN_ON_FALSE(NULL != mp3_decoder, ESP_ERR_NO_MEM, TAG, "Failed create MP3 decoder");

    read_buf = malloc(MAINBUF_SIZE);
    ESP_GOTO_ON_FALSE(NULL != read_buf, ESP_ERR_NO_MEM, clean_up, TAG, "Failed allocate read buffer");

    output = malloc(1152 * sizeof(int16_t) * 2);
    ESP_GOTO_ON_FALSE(NULL != output, ESP_ERR_NO_MEM, clean_up, TAG, "Failed allocate output buffer");

    /* Read audio file from given path */
    fp = fopen(path, "rb");
    ESP_GOTO_ON_FALSE(NULL != fp, ESP_ERR_NOT_FOUND, clean_up, TAG, "File \"%s\" does not exist", path);

    typedef struct {
        char header[3];     /*!< Always "ID3" */
        char ver;           /*!< Version, equals to3 if ID3V2.3 */
        char revision;      /*!< Revision, should be 0 */
        char flag;          /*!< Flag byte, use Bit[7..5] only */
        char size[4];       /*!< TAG size */
    } __attribute__((packed)) mp3_id3_header_v2_t;
    /* Get ID3 head */
    mp3_id3_header_v2_t tag;
    if (sizeof(mp3_id3_header_v2_t) == fread(&tag, 1, sizeof(mp3_id3_header_v2_t), fp)) {
        if (memcmp("ID3", (const void *) &tag, sizeof(tag.header)) == 0) {
            int tag_len =
                ((tag.size[0] & 0x7F) << 21) +
                ((tag.size[1] & 0x7F) << 14) +
                ((tag.size[2] & 0x7F) << 7) +
                ((tag.size[3] & 0x7F) << 0);
            fseek(fp, tag_len - sizeof(mp3_id3_header_v2_t), SEEK_SET);
        } else {
            /* Not ID3 header */
            fseek(fp, 0, SEEK_SET);
        }
    }

    /* Start MP3 decoding */
    int bytes_left = 0;
    unsigned char *read_ptr = read_buf;
    do {
        /* Process audio event sent from other task */
        if (pdPASS == xQueueReceive(audio_event_queue, &audio_event, 0)) {
            if (AUDIO_EVENT_PAUSE == audio_event) {
                g_player_state = PLAYER_STATE_PAUSE;
                i2s_zero_dma_buffer(I2S_NUM_0);
                xQueuePeek(audio_event_queue, &audio_event, portMAX_DELAY);
                continue;
            }

            if (AUDIO_EVENT_CHANGE == audio_event ||
                    AUDIO_EVENT_NEXT == audio_event ||
                    AUDIO_EVENT_PREV == audio_event) {
                i2s_zero_dma_buffer(I2S_NUM_0);
                ret = ESP_FAIL;
                goto clean_up;
            }
        }
        g_player_state = PLAYER_STATE_PLAYING;

        /* Read `mainDataBegin` size to RAM */
        if (bytes_left < MAINBUF_SIZE) {
            memmove(read_buf, read_ptr, bytes_left);
            size_t bytes_read = fread(read_buf + bytes_left, 1, MAINBUF_SIZE - bytes_left, fp);
            ESP_GOTO_ON_FALSE(bytes_read > 0, ESP_OK, clean_up, TAG, "No data read from strorage device");
            bytes_left = bytes_left + bytes_read;
            read_ptr = read_buf;
        }

        /* Find MP3 sync word from read buffer */
        int offset = MP3FindSyncWord(read_buf, MAINBUF_SIZE);

        if (offset >= 0) {
            read_ptr += offset;         /*!< Data start point */
            bytes_left -= offset;       /*!< In buffer */
            int mp3_dec_err = MP3Decode(mp3_decoder, &read_ptr, &bytes_left, (int16_t *) output, 0);
            ESP_GOTO_ON_FALSE(ERR_MP3_NONE == mp3_dec_err, ESP_FAIL, clean_up, TAG, "Can't decode MP3 frame");

            /* Get MP3 frame info and configure I2S clock */
            MP3GetLastFrameInfo(mp3_decoder, &frame_info);

            /* Configure I2S clock if sample rate changed. Always reconfigure at first frame */
            if (sample_rate != frame_info.samprate ||
                    nChans != frame_info.nChans ||
                    bits_cfg != frame_info.bitsPerSample) {
                sample_rate = frame_info.samprate;
                bits_cfg = frame_info.bitsPerSample;
                nChans = frame_info.nChans;
                ESP_LOGI(TAG, "audio info: sr=%d, bit=%d, ch=%d", sample_rate, bits_cfg, nChans);
                i2s_channel_t channel = (nChans == 1) ? I2S_CHANNEL_MONO : I2S_CHANNEL_STEREO;
                i2s_set_clk(I2S_NUM_0, sample_rate, bits_cfg, channel);
                // bsp_codec_set_fmt((nChans == 1) ? AUDIO_HAL_I2S_LEFT : AUDIO_HAL_I2S_NORMAL);
            }

            /* Write decoded data to audio decoder */
            size_t i2s_bytes_written = 0;
            size_t output_size = frame_info.outputSamps * nChans;
            i2s_write(I2S_NUM_0, output, output_size, &i2s_bytes_written, portMAX_DELAY);
        } else {
            /* Sync word not found in frame. Try to read next frame */
            ESP_LOGE(TAG, "MP3 sync word not found");
            bytes_left = 0;
            continue;
        }
    } while (true);

clean_up:
    /* This will prevent from sending dumy data to audio decoder */
    i2s_zero_dma_buffer(I2S_NUM_0);

    /* Clean up resources */
    if (NULL != mp3_decoder) {
        MP3FreeDecoder(mp3_decoder);
    }
    if (NULL != fp) {
        fclose(fp);
    }
    if (NULL != read_buf) {
        free(read_buf);
    }
    if (NULL != output) {
        free(output);
    }

    return ret;
}

static void audio_task(void *pvParam)
{
    char *base_path = (char *) pvParam;
    fm_init(base_path);
    /* Scan audio file */
    fm_print_dir(base_path, 2);
    fm_file_table_create(&g_file_list, &g_file_num, ".mp3");
    for (size_t i = 0; i < g_file_num; i++) {
        ESP_LOGI(TAG, "have file [%d:%s]", i, g_file_list[i]);
    }
    if (0 == g_file_num) {
        ESP_LOGW(TAG, "Can't found any mp3 file!");
        vTaskDelete(NULL);
    }

    if (esp_ptr_executable(s_audio_cb)) {
        player_cb_ctx_t ctx = {
            .audio_event = AUDIO_EVENT_FILE_SCAN_DONE,
            .user_ctx = audio_cb_usrt_ctx,
        };
        s_audio_cb(&ctx);
    }

    /* Audio control event queue */
    audio_event_queue = xQueueCreate(4, sizeof(player_event_t));
    if (NULL == audio_event_queue) {
        vTaskDelete(NULL);
    }

    /* Get name of first song to play */
    char full_name[256];
    memset(full_name, '\0', sizeof(full_name));
    strcpy(full_name, base_path);
    strcat(full_name, "/");
    strcat(full_name, app_player_get_name_from_index(audio_index));

    /* Default state is pause */
    app_player_pause();

    /* Start play music */
    while (1) {
        esp_err_t ret_val = play_mp3(full_name);

        /* Get next audio's name if audio played without error */
        if (ESP_OK == ret_val) {
            audio_index ++;
            if (audio_index >= g_file_num) {
                audio_index = 0;
            }
        }

        /* Callback for audio index change */
        if (esp_ptr_executable(s_audio_cb)) {
            player_cb_ctx_t ctx = {
                .audio_event = AUDIO_EVENT_CHANGE,
                .user_ctx = audio_cb_usrt_ctx,
            };
            s_audio_cb(&ctx);
        }

        /* Get next song's file name */
        memset(full_name, '\0', sizeof(full_name));
        strcpy(full_name, base_path);
        strcat(full_name, "/");
        strcat(full_name, app_player_get_name_from_index(audio_index));
    }

    /* Task never returns */
    vTaskDelete(NULL);
}

/* **************** AUDIO PLAY CONTROL **************** */
esp_err_t app_player_play(void)
{
    ESP_RETURN_ON_FALSE(NULL != audio_event_queue, ESP_ERR_INVALID_STATE, TAG, "Audio task not started yet");

    player_event_t event = AUDIO_EVENT_PLAY;
    BaseType_t ret_val = xQueueSend(audio_event_queue, &event, 0);
    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_ERR_INVALID_STATE, TAG, "The last event has not been processed yet");

    return ESP_OK;
}

esp_err_t app_player_pause(void)
{
    ESP_RETURN_ON_FALSE(NULL != audio_event_queue, ESP_ERR_INVALID_STATE, TAG, "Audio task not started yet");

    player_event_t event = AUDIO_EVENT_PAUSE;
    BaseType_t ret_val = xQueueSend(audio_event_queue, &event, 0);
    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_ERR_INVALID_STATE, TAG, "The last event has not been processed yet");

    return ESP_OK;
}

esp_err_t app_player_play_next(void)
{
    ESP_RETURN_ON_FALSE(NULL != audio_event_queue, ESP_ERR_INVALID_STATE, TAG, "Audio task not started yet");

    audio_index ++;
    if (audio_index >= g_file_num) {
        audio_index = 0;
    }
    player_event_t event = AUDIO_EVENT_NEXT;
    BaseType_t ret_val = xQueueSend(audio_event_queue, &event, 0);
    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_ERR_INVALID_STATE, TAG, "The last event has not been processed yet");

    return ESP_OK;
}

esp_err_t app_player_play_prev(void)
{
    ESP_RETURN_ON_FALSE(NULL != audio_event_queue, ESP_ERR_INVALID_STATE, TAG, "Audio task not started yet");

    if (audio_index == 0) {
        audio_index = g_file_num;
    }
    audio_index--;
    player_event_t event = AUDIO_EVENT_PREV;
    BaseType_t ret_val = xQueueSend(audio_event_queue, &event, 0);
    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_ERR_INVALID_STATE, TAG, "The last event has not been processed yet");

    return ESP_OK;
}

esp_err_t app_player_play_index(size_t index)
{
    ESP_RETURN_ON_FALSE(NULL != audio_event_queue, ESP_ERR_INVALID_STATE, TAG, "Audio task not started yet");
    ESP_RETURN_ON_FALSE(index < g_file_num, ESP_ERR_INVALID_ARG, TAG, "File index out of range");

    audio_index = index;
    player_event_t event = AUDIO_EVENT_CHANGE;
    BaseType_t ret_val = xQueueSend(audio_event_queue, &event, 0);
    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_ERR_INVALID_STATE, TAG, "The last event has not been processed yet");

    return ESP_OK;
}

/* **************** START AUDIO PLAYER **************** */
esp_err_t app_player_start(char *file_path)
{
    sys_param_t *param = settings_get_parameter();
    bsp_codec_set_voice_volume(param->volume);

    ESP_RETURN_ON_FALSE(NULL != file_path, ESP_ERR_INVALID_ARG, TAG,  "Invalid base path");
    BaseType_t ret_val = xTaskCreatePinnedToCore(audio_task, "Audio Task", 4 * 1024, file_path, configMAX_PRIORITIES - 5, NULL, 1);
    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_FAIL, TAG,  "Failed create audio task");
    return ESP_OK;
}

player_state_t app_player_get_state(void)
{
    return g_player_state;
}
