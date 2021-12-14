/**
 * @file app_audio.c
 * @brief 
 * @version 0.1
 * @date 2021-09-22
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

#include <stdbool.h>
#include <stdio.h>
#include "driver/i2s.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "app_audio";
static bool b_audio_playing = false;
static SemaphoreHandle_t audio_sem = NULL;

static void audio_task(void *pvParam);

esp_err_t app_audio_start(void)
{
    FILE *fp = fopen("/spiffs/audio/wake.pcm", "rb");
    if (NULL == fp) {
        ESP_LOGE(TAG, "Audio file does't exist");
        return ESP_ERR_NOT_FOUND;
    }

    fseek(fp, 0, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    void *audio_buffer = heap_caps_malloc(file_size, MALLOC_CAP_SPIRAM);
    if (NULL == audio_buffer) {
        ESP_LOGE(TAG, "No mem for audio buffer");
        return ESP_ERR_NO_MEM;
    }

    fread(audio_buffer, 1, file_size, fp);
    fclose(fp);

    audio_sem = xSemaphoreCreateBinary();
    if (NULL == audio_sem) {
        ESP_LOGE(TAG, "Failed create audio semaphore");
        return ESP_FAIL;
    }

    BaseType_t ret_val = xTaskCreatePinnedToCore(
        (TaskFunction_t)        audio_task,
        (const char * const)    "Audio Task",
        (const uint32_t)        2 * 1024,
        (void * const)          audio_buffer,
        (UBaseType_t)           1,
        (TaskHandle_t * const)  NULL,
        (const BaseType_t)      0);
    if (pdPASS != ret_val) {
        ESP_LOGE(TAG, "Failed create audio task");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void audio_play_start(void)
{
    if (NULL == audio_sem) {
        return;
    }

    xSemaphoreGive(audio_sem);
}

static void audio_task(void *pvParam)
{
    void *audio_buffer = pvParam;

    size_t bytes_written = 0;
    size_t bytes_to_write = heap_caps_get_allocated_size(audio_buffer);

    while (true) {
        xSemaphoreTake(audio_sem, portMAX_DELAY);
        b_audio_playing = true;
        i2s_write(I2S_NUM_0, audio_buffer, bytes_to_write, &bytes_written, portMAX_DELAY);
        b_audio_playing = false;

        /* It's useful if wake audio didn't finish playing when next wake word detetced */
        // xSemaphoreTake(audio_sem, 0);
    }
}

bool audio_is_playing(void)
{
    return b_audio_playing;
}

/* **************** AUDIO DEBUG TOOL **************** */
esp_err_t audio_record_to_file(size_t time_ms, const char *file_name)
{
    size_t audio_channel = 4;
    size_t audio_sample_rate = 16000;
    size_t bytes_per_sample = sizeof(int16_t);
    size_t file_size = audio_sample_rate * time_ms / 1000 * bytes_per_sample * audio_channel;

    /* Create file on storage device */
    FILE *fp = fopen(file_name, "w+");
    if (NULL == file_name) {
        ESP_LOGE(TAG, "Failed create file");
        return ESP_FAIL;
    }

    /* Alocate memory for audio data */
    void *audio_buffer = heap_caps_malloc(file_size, MALLOC_CAP_SPIRAM);
    if (NULL == audio_buffer) {
        ESP_LOGE(TAG, "Failed to allocate audio buffer");
        fclose(fp);
        return ESP_ERR_NO_MEM;
    }

    /* Read audio data to RAM from I2S */
    size_t bytes_read;
    ESP_LOGI(TAG, "Record start");
    esp_err_t ret_val = i2s_read(I2S_NUM_0, audio_buffer, file_size, &bytes_read, portMAX_DELAY);
    ESP_LOGI(TAG, "Record stop");

    /* Write audio data to storage device */
    fwrite(audio_buffer, 1, file_size, fp);
    fclose(fp);
    ESP_LOGI(TAG, "File saved to %s", file_name);

    return ret_val;
}
