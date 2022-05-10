/**
 * @file audio.c
 * @brief 
 * @version 0.1
 * @date 2021-12-06
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

#include "audio.h"
#include "bsp_i2s.h"
#include "bsp_codec.h"
#include "mp3dec.h"

static const char *TAG = "audio";

/* **************** PRIVATE STRUCT **************** */
typedef struct {
    char header[3];     /*!< Always "TAG" */
    char title[30];     /*!< Audio title */
    char artist[30];    /*!< Audio artist */
    char album[30];     /*!< Album name */
    char year[4];       /*!< Char array of year */
    char comment[30];   /*!< Extra comment */
    char genre;         /*!< See "https://en.wikipedia.org/wiki/ID3" */
} __attribute__((packed)) mp3_id3_header_v1_t;

typedef struct {
    char header[3];     /*!< Always "ID3" */
    char ver;           /*!< Version, equals to3 if ID3V2.3 */
    char revision;      /*!< Revision, should be 0 */
    char flag;          /*!< Flag byte, use Bit[7..5] only */
    char size[4];       /*!< TAG size */
} __attribute__((packed)) mp3_id3_header_v2_t;

/* **************** AUDIO CALLBACK **************** */
static audio_cb_t s_audio_cb = NULL;
static void *audio_cb_usrt_ctx = NULL;

esp_err_t audio_callback_register(audio_cb_t call_back, void *user_ctx)
{
    ESP_RETURN_ON_FALSE(esp_ptr_executable(call_back), ESP_ERR_INVALID_ARG,
        TAG, "Not a valid call back");

    s_audio_cb = call_back;
    audio_cb_usrt_ctx = user_ctx;
    
    return ESP_OK;
}

/* **************** AUDIO FILE SCAN **************** */
static size_t audio_count = 0;
static size_t audio_index = 0;
static char **audio_list = NULL;

static esp_err_t audio_file_scan(char *base_path)
{
    audio_count = 0;
    struct dirent *p_dirent = NULL;
    DIR *p_dir_stream = opendir(base_path);
 
    do {    /* Get total file count */
        p_dirent = readdir(p_dir_stream);
        if (NULL != p_dirent) {
            audio_count++;
        } else {
            closedir(p_dir_stream);
            break;
        }
    } while (true);

    audio_list = (char **) malloc(audio_count * sizeof(char *));
    ESP_RETURN_ON_FALSE(NULL != audio_list, ESP_ERR_NO_MEM,
        TAG, "Failed allocate audio list buffer");

    p_dir_stream = opendir(base_path);
    for (size_t i = 0; i < audio_count; i++) {
        p_dirent = readdir(p_dir_stream);
        if (NULL != p_dirent) {
            audio_list[i] = malloc(sizeof(p_dirent->d_name));
            ESP_LOGI(TAG, "File : %s", strcpy(audio_list[i], p_dirent->d_name));
        } else {
            ESP_LOGE(TAG, "The file system may be corrupted");
            closedir(p_dir_stream);
            for (int j = i - 1; j >= 0; j--) {
                free(audio_list[i]);
            }
            free(audio_list);
            return ESP_ERR_INVALID_STATE;
        }
    }

    closedir(p_dir_stream);
    return ESP_OK;
}

size_t audio_get_index(void)
{
    return audio_index;
}

char *audio_get_name_from_index(size_t index, char *base_path)
{
    ESP_RETURN_ON_FALSE(index < audio_count, NULL,
        TAG, "File index out of range");

    ESP_RETURN_ON_FALSE(NULL != audio_list, NULL,
        TAG, "Audio file not found");

    ESP_RETURN_ON_FALSE(NULL != audio_list[index], NULL,
        TAG, "Audio file not found");

    if (NULL != base_path) {
        strcat(strcat(base_path, "/"), audio_list[index]);
    }

    return audio_list[index];
}

/* **************** AUDIO DECODE **************** */
static QueueHandle_t audio_event_queue = NULL;

static esp_err_t aplay_mp3(const char *path)
{
    ESP_LOGI(TAG, "start to decode %s", path);

    FILE *fp = NULL;
    int sample_rate = 0;
    esp_err_t ret = ESP_OK;
    uint8_t *output = NULL;
    uint8_t *read_buf = NULL;
    MP3FrameInfo frame_info;
    HMP3Decoder mp3_decoder = MP3InitDecoder();
    audio_event_t audio_event = AUDIO_EVENT_NONE;

    ESP_RETURN_ON_FALSE(NULL != mp3_decoder, ESP_ERR_NO_MEM,
        TAG, "Failed create MP3 decoder");

    read_buf = malloc(MAINBUF_SIZE);
    ESP_GOTO_ON_FALSE(NULL != read_buf, ESP_ERR_NO_MEM, clean_up,
        TAG, "Failed allocate read buffer");

    output = malloc(1152 * sizeof(int16_t) * 2);
    ESP_GOTO_ON_FALSE(NULL != output, ESP_ERR_NO_MEM, clean_up,
        TAG, "Failed allocate output buffer");
    
    /* Read audio file from given path */
    fp = fopen(path, "rb");
    ESP_GOTO_ON_FALSE(NULL != fp, ESP_ERR_NOT_FOUND, clean_up,
        TAG, "File \"%s\" does not exist", path);

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
                i2s_zero_dma_buffer(I2S_NUM_0);
                xQueuePeek(audio_event_queue, &audio_event, portMAX_DELAY);
                continue;
            }

            if (AUDIO_EVENT_CHANGE == audio_event) {
                i2s_zero_dma_buffer(I2S_NUM_0);
                ret = ESP_FAIL;
                goto clean_up;
            }
        }

        /* Read `mainDataBegin` size to RAM */
        if (bytes_left < MAINBUF_SIZE) {
            memmove(read_buf, read_ptr, bytes_left);
            size_t bytes_read = fread(read_buf + bytes_left, 1, MAINBUF_SIZE - bytes_left, fp);
            ESP_GOTO_ON_FALSE(bytes_read > 0, ESP_OK, clean_up,
                TAG, "No data read from strorage device");
            bytes_left = bytes_left + bytes_read;
            read_ptr = read_buf;
        }

        /* Find MP3 sync word from read buffer */
        int offset = MP3FindSyncWord(read_buf, MAINBUF_SIZE);

        if (offset >= 0) {
            read_ptr += offset;         /*!< Data start point */
            bytes_left -= offset;       /*!< In buffer */
            int mp3_dec_err = MP3Decode(mp3_decoder, &read_ptr, &bytes_left, (int16_t *) output, 0);
            ESP_GOTO_ON_FALSE(ERR_MP3_NONE == mp3_dec_err, ESP_FAIL, clean_up,
                TAG, "Can't decode MP3 frame");

            /* Get MP3 frame info and configure I2S clock */
            MP3GetLastFrameInfo(mp3_decoder, &frame_info);

            // if mono, convert to stereo as es8311 requires stereo input
            // even though it is mono output
            if(frame_info.nChans ==  1) {
                size_t new_output_sample_count = frame_info.outputSamps * 2;

                // convert from back to front to allow conversion in-place
                int16_t *out = (int16_t*)output + new_output_sample_count;
                int16_t *in = (int16_t*)output + frame_info.outputSamps;
                size_t samples = frame_info.outputSamps;
                while(samples) {
                    // write right channel
                    *out = *in;
                    out--;

                    // write left channel
                    *out = *in;
                    out--;

                    // move input buffer back and decrement samples
                    in--;
                    samples--;
                }

                // adjust channels to 2
                frame_info.nChans = 2;

                // recalculate output sample count as its calculated by (channels * samples per frame)
                frame_info.outputSamps = new_output_sample_count;
            }

            /* Configure I2S clock if sample rate changed. Always reconfigure at first frame */
            if (sample_rate != frame_info.samprate) {
                sample_rate = frame_info.samprate;
                uint32_t bits_cfg = frame_info.bitsPerSample;
                i2s_channel_t channel = (frame_info.nChans == 1) ? I2S_CHANNEL_MONO : I2S_CHANNEL_STEREO;
                i2s_set_clk(I2S_NUM_0, sample_rate, bits_cfg, channel);
            }

            /* Write decoded data to audio decoder */
            size_t i2s_bytes_written = 0;
            size_t output_size = frame_info.outputSamps * frame_info.nChans;
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
    if (NULL != mp3_decoder)    MP3FreeDecoder(mp3_decoder);
    if (NULL != fp)             fclose(fp);
    if (NULL != read_buf)       free(read_buf);
    if (NULL != output)         free(output);

    return ret;
}

static void audio_task(void *pvParam)
{
    /* Scan audio file */
    char *base_path = (char *) pvParam;
    if (NULL != base_path) {
        audio_file_scan(base_path);
        if (esp_ptr_executable(s_audio_cb)) {
            audio_cb_ctx_t ctx = {
                .audio_event = AUDIO_EVENT_FILE_SCAN_DONE,
                .user_ctx = audio_cb_usrt_ctx,
            };
            s_audio_cb(&ctx);
        }
    } else {
        ESP_LOGE(TAG, "Invalid base path");
        vTaskDelete(NULL);
    }

    /**
     * @brief Initialize I2S and audio codec
     * 
     * @note `MP3GetLastFrameInfo` is used to fill the `MP3FrameInfo`, which includes `samprate`,
     *       and the sampling rate is updated during playback using this value.
     */
    bsp_codec_init(AUDIO_HAL_44K_SAMPLES);
    bsp_i2s_init(I2S_NUM_0, 44100);
    
    /* Audio control event queue */
    audio_event_queue = xQueueCreate(4, sizeof(audio_event_t));
    if (NULL == audio_event_queue) {
        vTaskDelete(NULL);
    }

    /* Get name of first song to play */
    char full_name[256] = { [0 ... sizeof(full_name) - 1] = '\0' };
    strcpy(full_name, base_path);
    char *file_name = audio_get_name_from_index(audio_index, full_name);

    /* Start play music */
    while (vTaskDelay(1), NULL != file_name) {
        esp_err_t ret_val = aplay_mp3(full_name);

        /* Get next audio's name if audio played without error */
        if (ESP_OK == ret_val) {
            audio_index ++;
            if (audio_index >= audio_count) {
                audio_index = 0;
            }
        }

        /* Callback for audio index change */
        if (esp_ptr_executable(s_audio_cb)) {
            audio_cb_ctx_t ctx = {
                .audio_event = AUDIO_EVENT_CHANGE,
                .user_ctx = audio_cb_usrt_ctx,
            };
            s_audio_cb(&ctx);
        }
        
        /* Get next song's file name */
        file_name = audio_get_name_from_index(audio_index, strcpy(full_name, base_path));
    }

    /* Task never returns */
    vTaskDelete(NULL);
}

/* **************** AUDIO PLAY CONTROL **************** */
esp_err_t audio_play(void)
{
    ESP_RETURN_ON_FALSE(NULL != audio_event_queue, ESP_ERR_INVALID_STATE,
        TAG, "Audio task not started yet");

    audio_event_t event = AUDIO_EVENT_PLAY;
    BaseType_t ret_val = xQueueSend(audio_event_queue, &event, 0);

    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_ERR_INVALID_STATE,
        TAG, "The last event has not been processed yet");

    return ESP_OK;
}

esp_err_t audio_pause(void)
{
    ESP_RETURN_ON_FALSE(NULL != audio_event_queue, ESP_ERR_INVALID_STATE,
        TAG, "Audio task not started yet");

    audio_event_t event = AUDIO_EVENT_PAUSE;
    BaseType_t ret_val = xQueueSend(audio_event_queue, &event, 0);

    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_ERR_INVALID_STATE,
        TAG, "The last event has not been processed yet");

    return ESP_OK;
}

esp_err_t audio_play_next(void)
{
    ESP_RETURN_ON_FALSE(NULL != audio_event_queue, ESP_ERR_INVALID_STATE,
        TAG, "Audio task not started yet");

    audio_index ++;
    if (audio_index >= audio_count) {
        audio_index = 0;
    }
    audio_event_t event = AUDIO_EVENT_CHANGE;
    BaseType_t ret_val = xQueueSend(audio_event_queue, &event, 0);

    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_ERR_INVALID_STATE,
        TAG, "The last event has not been processed yet");

    return ESP_OK;
}

esp_err_t audio_play_prev(void)
{
    ESP_RETURN_ON_FALSE(NULL != audio_event_queue, ESP_ERR_INVALID_STATE,
        TAG, "Audio task not started yet");

    if (audio_index == 0) {
        audio_index = audio_count;
    }
    audio_index--;
    audio_event_t event = AUDIO_EVENT_CHANGE;
    BaseType_t ret_val = xQueueSend(audio_event_queue, &event, 0);

    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_ERR_INVALID_STATE,
        TAG, "The last event has not been processed yet");

    return ESP_OK;
}

esp_err_t audio_play_index(size_t index)
{
    ESP_RETURN_ON_FALSE(NULL != audio_event_queue, ESP_ERR_INVALID_STATE,
        TAG, "Audio task not started yet");

    ESP_RETURN_ON_FALSE(index < audio_count, ESP_ERR_INVALID_ARG,
        TAG, "File index out of range");

    audio_index = index;
    audio_event_t event = AUDIO_EVENT_CHANGE;
    BaseType_t ret_val = xQueueSend(audio_event_queue, &event, 0);

    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_ERR_INVALID_STATE,
        TAG, "The last event has not been processed yet");

    return ESP_OK;
}

/* **************** START AUDIO PLAYER **************** */
esp_err_t mp3_player_start(char *file_path)
{
    BaseType_t ret_val = xTaskCreatePinnedToCore(
        (TaskFunction_t)        audio_task,
        (const char * const)    "Audio Task",
        (const uint32_t)        4 * 1024,
        (void * const)          file_path,
        (UBaseType_t)           configMAX_PRIORITIES - 1,
        (TaskHandle_t * const)  NULL,
        (const BaseType_t)      0);

    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_ERR_NO_MEM,
        TAG, "Failed create audio task");

    return ESP_OK;
}
