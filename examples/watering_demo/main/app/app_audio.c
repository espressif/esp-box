/*
* SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Unlicense OR CC0-1.0
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

static void audio_beep_task(void *pvParam)
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

esp_err_t app_audio_beep_init(void)
{
    FILE *fp = fopen("/spiffs/wake.pcm", "rb");
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
                             (TaskFunction_t)        audio_beep_task,
                             (const char *const)    "beep Task",
                             (const uint32_t)        2 * 1024,
                             (void *const)          audio_buffer,
                             (UBaseType_t)           1,
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
        return;
    }

    xSemaphoreGive(audio_sem);
}

bool app_audio_beep_is_playing(void)
{
    return b_audio_playing;
}
