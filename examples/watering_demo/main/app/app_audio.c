/*
 * SPDX-FileCopyrightText: 2015-2023 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Unlicense OR CC0-1.0
*/
#include <stdbool.h>
#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_vfs.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "bsp_board.h"
#include "bsp/esp-bsp.h"

typedef struct {
    // The "RIFF" chunk descriptor
    uint8_t ChunkID[4];// Indicates the file as "RIFF" file
    int32_t ChunkSize;// The total size of the entire file, excluding the "RIFF" and the header itself, which is the file size minus 8 bytes.
    uint8_t Format[4];// File format header indicating a "WAVE" file.
    // The "fmt" sub-chunk
    uint8_t Subchunk1ID[4];// Format identifier for the "fmt" sub-chunk.
    int32_t Subchunk1Size;// The length of the fmt sub-chunk (subchunk1) excluding the Subchunk1 ID and Subchunk1 Size fields. It is typically 16, but a value greater than 16 indicates the presence of an extended area. Optional values for the length include 16, 18, 20, 40, etc.
    int16_t AudioFormat;// Audio encoding format, which represents the compression format. A value of 0x01 indicates PCM format, which is uncompressed. Please refer to table 3 for more details.
    int16_t NumChannels;// Number of audio channels
    int32_t SampleRate;// Sample rate, for example, "44100" represents a sampling rate of 44100 Hz.
    int32_t ByteRate;// Bit rate: Sample rate x bit depth x number of channels / 8. For example, the bit rate for a stereo (2 channels) audio with a sample rate of 44.1 kHz and 16-bit depth would be 176400 bits per second.
    int16_t BlockAlign;// Memory size occupied by one sample: Bit depth x number of channels / 8.
    int16_t BitsPerSample;//Sample depth, also known as bit depth.
    // The "data" sub-chunk
    uint8_t Subchunk2ID[4];// Total length of the audio data, which is the file size minus the length of the WAV file header.
    int32_t Subchunk2Size;// Length of the data section, referring to the size of the audio data excluding the header.
} wav_header_t;

static const char *TAG = "app_audio";
static bool b_audio_playing = false;
static SemaphoreHandle_t audio_sem = NULL;

esp_err_t sr_echo_play(void *filepath)
{
    FILE *fp = NULL;
    struct stat file_stat;
    esp_err_t ret = ESP_OK;

    const size_t chunk_size = 4096;
    uint8_t *buffer = malloc(chunk_size);
    ESP_GOTO_ON_FALSE(NULL != buffer, ESP_FAIL, EXIT, TAG, "buffer malloc failed");

    ESP_GOTO_ON_FALSE(-1 != stat(filepath, &file_stat), ESP_FAIL, EXIT, TAG, "Failed to stat file");

    fp = fopen(filepath, "r");
    ESP_GOTO_ON_FALSE(NULL != fp, ESP_FAIL, EXIT, TAG, "Failed create record file");

    wav_header_t wav_head;
    int len = fread(&wav_head, 1, sizeof(wav_header_t), fp);
    ESP_GOTO_ON_FALSE(len > 0, ESP_FAIL, EXIT, TAG, "Read wav header failed");

    if (NULL == strstr((char *)wav_head.Subchunk1ID, "fmt") &&
            NULL == strstr((char *)wav_head.Subchunk2ID, "data")) {
        ESP_LOGI(TAG, "PCM format");
        fseek(fp, 0, SEEK_SET);
        wav_head.SampleRate = 16000;
        wav_head.NumChannels = 2;
        wav_head.BitsPerSample = 16;
    }

    ESP_LOGD(TAG, "frame_rate= %" PRIi32 ", ch=%d, width=%d", wav_head.SampleRate, wav_head.NumChannels, wav_head.BitsPerSample);
    bsp_codec_set_fs(wav_head.SampleRate, wav_head.BitsPerSample, I2S_SLOT_MODE_STEREO);

    bsp_codec_mute_set(true);
    bsp_codec_mute_set(false);
    bsp_codec_volume_set(100, NULL);

    size_t cnt, total_cnt = 0;
    do {
        /* Read file in chunks into the scratch buffer */
        len = fread(buffer, 1, chunk_size, fp);
        if (len <= 0) {
            break;
        } else if (len > 0) {
            bsp_i2s_write(buffer, len, &cnt, portMAX_DELAY);
            total_cnt += cnt;
        }
    } while (1);
    ESP_LOGI(TAG, "play end, %d K", total_cnt / 1024);

EXIT:
    if (fp) {
        fclose(fp);
    }
    if (buffer) {
        free(buffer);
    }
    return ret;
}

static void audio_beep_task(void *pvParam)
{
    while (true) {
        xSemaphoreTake(audio_sem, portMAX_DELAY);
        b_audio_playing = true;
        sr_echo_play("/spiffs/echo_en_wake.wav");
        b_audio_playing = false;

        /* It's useful if wake audio didn't finish playing when next wake word detetced */
        // xSemaphoreTake(audio_sem, 0);
    }
}

esp_err_t app_audio_beep_init(void)
{
    audio_sem = xSemaphoreCreateBinary();
    if (NULL == audio_sem) {
        ESP_LOGE(TAG, "Failed create audio semaphore");
        return ESP_FAIL;
    }

    BaseType_t ret_val = xTaskCreatePinnedToCore(
                             (TaskFunction_t)        audio_beep_task,
                             (const char *const)    "beep Task",
                             (const uint32_t)        4 * 1024,
                             NULL,
                             (UBaseType_t)          5,
                             (TaskHandle_t *const)  NULL,
                             (const BaseType_t)      0);
    if (pdPASS != ret_val) {
        ESP_LOGE(TAG, "Failed create audio task");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t app_audio_beep_play_start(void)
{
    if (NULL == audio_sem) {
        ESP_LOGE(TAG, "audio not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreGive(audio_sem);
    return ESP_OK;
}

bool app_audio_beep_is_playing(void)
{
    return b_audio_playing;
}
