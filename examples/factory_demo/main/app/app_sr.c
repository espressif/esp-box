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
#include "app_sr.h"
#include "dl_lib_coefgetter_if.h"
#include "driver/i2s.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#define I2S_CHANNEL_NUM     (3)

static const char *TAG = "app_sr";

static afe_config_t afe_config = {
    .aec_init = false,
    .se_init = true,
    .vad_init = true,
    .wakenet_init = true,
    .vad_mode = 3,
    .wakenet_model = (esp_wn_iface_t *) &WAKENET_MODEL,
    .wakenet_coeff = (model_coeff_getter_t *) &WAKENET_COEFF,
    .wakenet_mode = DET_MODE_2CH_90,
    .afe_mode = SR_MODE_LOW_COST,
    .afe_perferred_core = 0,
    .afe_perferred_priority = configMAX_PRIORITIES - 2,
    .afe_ringbuf_size = 50,
    .alloc_from_psram = true,
    .agc_mode = 2,
};
static const esp_afe_sr_iface_t *afe_handle = &esp_afe_sr_2mic;

static volatile int32_t sr_command_id = -1;
static EventGroupHandle_t sr_event_group_handle = NULL;

static void audio_feed_task(void *pvParam)
{
    size_t bytes_read = 0;
    esp_afe_sr_data_t *afe_data = (esp_afe_sr_data_t *) pvParam;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);

    /* Allocate audio buffer and check for result */
    int16_t *audio_buffer = heap_caps_malloc(audio_chunksize * sizeof(int16_t) * I2S_CHANNEL_NUM, MALLOC_CAP_INTERNAL);
    if (NULL == audio_buffer) {
        esp_system_abort("No mem for audio buffer");
    }

    while (true) {
        /* Read audio data from I2S bus */
        i2s_read(I2S_NUM_0, audio_buffer, audio_chunksize * I2S_CHANNEL_NUM * sizeof(int16_t), &bytes_read, portMAX_DELAY);

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
    int detect_flag = 0;
    const esp_mn_iface_t *multinet = &MULTINET_MODEL;
    model_iface_data_t *model_data = multinet->create(&MULTINET_COEFF, 6000);
    esp_afe_sr_data_t *afe_data = (esp_afe_sr_data_t *) pvParam;

    /* Allocate buffer for detection */
    size_t afe_chunk_size = afe_handle->get_fetch_chunksize(afe_data);
    int16_t *detect_buff = heap_caps_malloc(afe_chunk_size * sizeof(int16_t), MALLOC_CAP_INTERNAL);
    if (NULL == detect_buff) {
        esp_system_abort("No mem for detect buffer");
    }

    /* Check for chunk size */
    if (afe_chunk_size != multinet->get_samp_chunksize(model_data)) {
        esp_system_abort("Invalid chunk size");
    }

    while (true) {
        /* Wake word detected if `fetch()` returns none zero value */
        if (afe_handle->fetch(afe_data, detect_buff) > 0) {
            ESP_LOGD(TAG, LOG_BOLD(LOG_COLOR_GREEN) "Wakeword detected");
            detect_flag = 1;
            xEventGroupSetBits(sr_event_group_handle, SR_EVENT_WAKE_UP);

            /* Disable wakenet. Detect command(s) with multinet. */
            afe_handle->disable_wakenet(afe_data);
        }

        if (detect_flag == 1) {
            sr_command_id = multinet->detect(model_data, detect_buff);

            if(sr_command_id == -2) {
                ESP_LOGD(TAG, LOG_BOLD(LOG_COLOR_GREEN) "Timeout");

                xEventGroupSetBits(sr_event_group_handle, SR_EVENT_TIMEOUT);

                /* Enable wakenet and AEC again */
                afe_handle->enable_wakenet(afe_data);

                /* Reset detect flag */
                detect_flag = 0;
            }

            if (sr_command_id > -1) {
                ESP_LOGD(TAG, LOG_BOLD(LOG_COLOR_GREEN) "Command word %2d detected", sr_command_id);
                xEventGroupSetBits(sr_event_group_handle, SR_EVENT_WORD_DETECT);

                /* Enable wakenet and AEC again */
                afe_handle->enable_wakenet(afe_data);

                /* Reset detect flag */
                detect_flag = 0;
            }
        }
    }

    /* Clean up if audio feed ends */
    afe_handle->destroy(afe_data);

    /* Task never returns */
    vTaskDelete(NULL);
}

esp_err_t app_sr_start(void)
{
    sr_event_group_handle = xEventGroupCreate();
    if (NULL == sr_event_group_handle) {
        ESP_LOGE(TAG, "Failed create event group");
        return ESP_ERR_NO_MEM;
    }

    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(&afe_config);

    /* Create audio feed task. Acquire data from I2S bus */
    BaseType_t ret_val = xTaskCreatePinnedToCore(
        (TaskFunction_t)        audio_feed_task,
        (const char * const)    "Feed Task",
        (const uint32_t)        4 * 1024,
        (void * const)          afe_data,
        (UBaseType_t)           configMAX_PRIORITIES - 2,
        (TaskHandle_t * const)  NULL,
        (const BaseType_t)      0);
    if (pdPASS != ret_val) {
        ESP_LOGE(TAG, "Failed create audio feed task");
        return ESP_FAIL;
    }
    
    /* Create audio detect task. Detect data fetch from buffer */
    ret_val = xTaskCreatePinnedToCore(
        (TaskFunction_t)        audio_detect_task,
        (const char * const)    "Detect Task",
        (const uint32_t)        6 * 1024,
        (void * const)          afe_data,
        (UBaseType_t)           configMAX_PRIORITIES - 2,
        (TaskHandle_t * const)  NULL,
        (const BaseType_t)      1);
    if (pdPASS != ret_val) {
        ESP_LOGE(TAG, "Failed create audio detect task");
        return ESP_FAIL;
    }

    /* Create audio detect task. Detect data fetch from buffer */
    ret_val = xTaskCreatePinnedToCore(
        (TaskFunction_t)        sr_handler_task,
        (const char * const)    "SR Handler Task",
        (const uint32_t)        4 * 1024,
        (void * const)          sr_event_group_handle,
        (UBaseType_t)           1,
        (TaskHandle_t * const)  NULL,
        (const BaseType_t)      0);
    if (pdPASS != ret_val) {
        ESP_LOGE(TAG, "Failed create audio detect task");
        return ESP_FAIL;
    }

    return ESP_OK;
}

int32_t app_sr_get_last_cmd_id(void)
{
    return sr_command_id;
}
