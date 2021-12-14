/**
 * @file app_sr.c
 * @brief 
 * @version 0.1
 * @date 2021-09-19
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
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "app_audio.h"
#include "app_sr.h"
#include "bsp_codec.h"
#include "bsp_i2s.h"
#include "driver/i2s.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_models.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#define SR_CMD_TIME_OUT     (-2)
#define SR_CMD_NOT_DETECTED (-1)
#define SR_CMD_ID_BASE      (0)
#define I2S_CHANNEL_NUM     (2)

static const char *TAG = "app_sr";

static afe_config_t afe_config = {
    .aec_init = false,
    .se_init = true,
    .vad_init = false,
    .wakenet_init = true,
    .vad_mode = 3,
    .wakenet_model = (esp_wn_iface_t *) &WAKENET_MODEL,
    .wakenet_coeff = (model_coeff_getter_t *) &WAKENET_COEFF,
    .wakenet_mode = DET_MODE_2CH_90,
    .afe_mode = SR_MODE_LOW_COST,
    .afe_perferred_core = 0,
    .afe_perferred_priority = 8,
    .afe_ringbuf_size = 50,
    .alloc_from_psram = 1,
    .agc_mode = 2,  /* 1 : -5dB 2: -4dB ... */
};

static model_iface_data_t *model_data = NULL;
static const esp_mn_iface_t *multinet = &MULTINET_MODEL;
static const esp_afe_sr_iface_t *afe_handle = &esp_afe_sr_2mic;

static FILE *fp = NULL;
static bool b_record_en = false;
static volatile int32_t sr_command_id = -1;
static EventGroupHandle_t sr_event_group_handle = NULL;

static void audio_feed_task(void *pvParam)
{
    size_t bytes_read = 0;
    esp_afe_sr_data_t *afe_data = (esp_afe_sr_data_t *) pvParam;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);

    /* Allocate audio buffer and check for result */
    int16_t *audio_buffer = heap_caps_malloc(audio_chunksize * sizeof(int16_t) * 3, MALLOC_CAP_INTERNAL);
    if (NULL == audio_buffer) {
        esp_system_abort("No mem for audio buffer");
    }

    while (true) {
        /* Read audio data from I2S bus */
        i2s_read(I2S_NUM_0, audio_buffer, audio_chunksize * I2S_CHANNEL_NUM * sizeof(int16_t), &bytes_read, portMAX_DELAY);

        /* Save audio data to file if record enabled */
        if (b_record_en && (NULL != fp)) {
            fwrite(audio_buffer, 1, audio_chunksize * I2S_CHANNEL_NUM * sizeof(int16_t), fp);
        }

        /* Channel Adjust */
        for (int  i = audio_chunksize - 1; i >= 0; i--) {
            audio_buffer[i * 3 + 2] = 0;
            audio_buffer[i * 3 + 1] = audio_buffer[i * 2 + 1];
            audio_buffer[i * 3 + 0] = audio_buffer[i * 2 + 0];
        }

        /* Feed samples of an audio stream to the AFE_SR */
        afe_handle->feed(afe_data, audio_buffer);
    }

    /* Clean up if audio feed ends */
    afe_handle->destroy(afe_data);

    /* Task never returns */
    vTaskDelete(NULL);
}

static void audio_detect_task(void *pvParam)
{
    bool detect_flag = false;
    esp_afe_sr_data_t *afe_data = (esp_afe_sr_data_t *) pvParam;

    /* Allocate buffer for detection */
    size_t afe_chunk_size = afe_handle->get_fetch_chunksize(afe_data);
    int16_t *detect_buff = heap_caps_malloc(afe_chunk_size * sizeof(int16_t), MALLOC_CAP_INTERNAL);
    if (NULL == detect_buff) {
        ESP_LOGE(TAG, "Expect : %zu, avaliable : %zu",
            afe_chunk_size * sizeof(int16_t),
            heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
        esp_system_abort("No mem for detect buffer");
    }

    /* Check for chunk size */
    if (afe_chunk_size != multinet->get_samp_chunksize(model_data)) {
        esp_system_abort("Invalid chunk size");
    }

    while (true) {
        int ret_val = afe_handle->fetch(afe_data, detect_buff);

        if (AFE_FETCH_WWE_DETECTED == ret_val) {
            ESP_LOGI(TAG, LOG_BOLD(LOG_COLOR_GREEN) "Wakeword detected");
            xEventGroupSetBits(sr_event_group_handle, SR_EVENT_WAKE_UP);
        }

        if (AFE_FETCH_CHANNEL_VERIFIED == ret_val) {
            ESP_LOGI(TAG, LOG_BOLD(LOG_COLOR_GREEN) "Channel verified");
            detect_flag = true;
            afe_handle->disable_wakenet(afe_data);
        }

        if (true == detect_flag) {
            /* Save audio data to file if record enabled */
            if (b_record_en && (NULL != fp)) {
                fwrite(detect_buff, 1, afe_chunk_size * sizeof(int16_t), fp);
            }

            if (false == audio_is_playing()) {
                sr_command_id = multinet->detect(model_data, detect_buff);
            } else {
                continue;
            }

            if (SR_CMD_NOT_DETECTED == sr_command_id) {
                continue;
            }

            if (SR_CMD_TIME_OUT == sr_command_id) {
                ESP_LOGW(TAG, LOG_BOLD(LOG_COLOR_BROWN) "Time out");
                xEventGroupSetBits(sr_event_group_handle, SR_EVENT_TIMEOUT);
                afe_handle->enable_wakenet(afe_data);
                detect_flag = false;
                continue;
            }

            if (SR_CMD_ID_BASE <= sr_command_id) {
                ESP_LOGI(TAG, LOG_BOLD(LOG_COLOR_GREEN) "Deteted command : %d", sr_command_id);
                xEventGroupSetBits(sr_event_group_handle, SR_EVENT_WORD_DETECT);
                afe_handle->enable_wakenet(afe_data);
                detect_flag = 0;

                if (b_record_en && (NULL != fp)) {
                    ESP_LOGI(TAG, "File saved");
                    fclose(fp);
                    fp = NULL;
                }
                continue;
            }

            ESP_LOGE(TAG, "Exception unhandled");
        }
    }

    /* Clean up if audio feed ends */
    afe_handle->destroy(afe_data);

    /* Task never returns */
    vTaskDelete(NULL);
}

static void sr_init_task(void *pvParam)
{
    (void) pvParam;

    ESP_ERROR_CHECK(bsp_i2s_init(I2S_NUM_0, 16000));
    ESP_ERROR_CHECK(bsp_codec_init(AUDIO_HAL_16K_SAMPLES));

    vTaskDelay(pdMS_TO_TICKS(4000));

    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(&afe_config);

    /* Continuously reading flash will call task watchdog  */
    vTaskDelay(pdMS_TO_TICKS(100));
    
    model_data = multinet->create((const model_coeff_getter_t *) &MULTINET_COEFF, 5760);

    /* Create audio feed task. Acquire data from I2S bus */
    BaseType_t ret_val = xTaskCreatePinnedToCore(
        (TaskFunction_t)        audio_feed_task,
        (const char * const)    "Feed Task",
        (const uint32_t)        2 * 1024,
        (void * const)          afe_data,
        (UBaseType_t)           5,
        (TaskHandle_t * const)  NULL,
        (const BaseType_t)      1);
    if (pdPASS != ret_val) {
        ESP_LOGE(TAG, "Failed create audio feed task");
        return;
    }

    /* Create audio detect task. Detect data fetch from buffer */
    ret_val = xTaskCreatePinnedToCore(
        (TaskFunction_t)        audio_detect_task,
        (const char * const)    "Detect Task",
        (const uint32_t)        5 * 1024,
        (void * const)          afe_data,
        (UBaseType_t)           5,
        (TaskHandle_t * const)  NULL,
        (const BaseType_t)      1);
    if (pdPASS != ret_val) {
        ESP_LOGE(TAG, "Failed create audio detect task");
        return;
    }

    /* Create audio detect task. Detect data fetch from buffer */
    extern void sr_handler_task(void *pvParam);
    ret_val = xTaskCreatePinnedToCore(
        (TaskFunction_t)        sr_handler_task,
        (const char * const)    "SR Handler Task",
        (const uint32_t)        5 * 1024,
        (void * const)          sr_event_group_handle,
        (UBaseType_t)           1,
        (TaskHandle_t * const)  NULL,
        (const BaseType_t)      0);
    if (pdPASS != ret_val) {
        ESP_LOGE(TAG, "Failed create audio detect task");
        return;
    }

    vTaskDelete(NULL);
}

esp_err_t app_sr_start(bool record_en)
{
    sr_event_group_handle = xEventGroupCreate();
    if (NULL == sr_event_group_handle) {
        ESP_LOGE(TAG, "Failed create event group");
        return ESP_ERR_NO_MEM;
    }

    /* Create file if record to SD card enabled*/
    if (record_en) {
        char file_name[32];
        b_record_en = true;
        for (size_t i = 0; i < 100; i++) {
            sprintf(file_name, "/sdcard/Record_%02d.pcm", i);
            fp = fopen(file_name, "r");
            fclose(fp);
            if (NULL == fp) {
                break;
            }
        }

        fp = fopen(file_name, "w");
        if (NULL == fp) {
            ESP_LOGE(TAG, "Failed create file");
            return ESP_FAIL;
        } else {
            ESP_LOGI(TAG, "File created : %s", file_name);
        }

        ESP_LOGI(TAG, "File created at %s", file_name);
    }

    /* Create audio detect task. Detect data fetch from buffer */
    BaseType_t ret_val = xTaskCreatePinnedToCore(
        (TaskFunction_t)        sr_init_task,
        (const char * const)    "SR Init Task",
        (const uint32_t)        4 * 1024,
        (void * const)          NULL,
        (UBaseType_t)           1,
        (TaskHandle_t * const)  NULL,
        (const BaseType_t)      1);
    if (pdPASS != ret_val) {
        ESP_LOGE(TAG, "Failed create sr initialize task");
        return ESP_FAIL;
    }

    return ESP_OK;
}

int32_t app_sr_get_last_cmd_id(void)
{
    return sr_command_id;
}

esp_err_t app_sr_reset_command_list(char *command_list)
{
    char *err_id = heap_caps_malloc(1024, MALLOC_CAP_SPIRAM);

    if (NULL == err_id) {
        return ESP_ERR_NO_MEM;
    }

    multinet->reset(model_data, command_list, err_id);

    free(err_id);

    return ESP_OK;
}
