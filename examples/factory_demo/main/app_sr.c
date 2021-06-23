/**
 * @file app_sr.c
 * @brief 
 * @version 0.1
 * @date 2021-08-10
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
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lv_demo.h"

/* AI group codec config. Needs to clean up. */
#include "MediaHal.h"

#define I2S_CHANNEL_NUM     (4)

static const char *TAG = "app_sr";

/* Used for reset command list. Avaliable for Chinese wakeword only */
static const char *old_commands_str =
    /* 00 */ "chong xin qi dong;"
    /* 01 */ "beng kui;"
    /* 02 */ "nei cun xin xi;"
    /* 03 */ "zai jian;";

static const esp_afe_sr_iface_t *afe_handle = NULL;
static model_iface_data_t *model_data = NULL;
static const esp_mn_iface_t *multinet = NULL;

static volatile uint32_t sr_event_id = 0;
static volatile int32_t sr_command_id = -1;

static QueueHandle_t audio_queue_handle = NULL;

static void audio_feed_task(void *pvParam)
{
    size_t bytes_read = 0;
    esp_afe_sr_data_t *afe_data = (esp_afe_sr_data_t *) pvParam;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);

    /* Allocate audio buffer and check for result */
    int16_t *audio_buffer = heap_caps_malloc(audio_chunksize * sizeof(int16_t) * I2S_CHANNEL_NUM, MALLOC_CAP_INTERNAL);
    if (NULL == audio_buffer) {
        ESP_LOGE(TAG, "No mem for audio buffer");
        esp_system_abort("No mem for audio buffer");
    }

    while (true) {
        /* Read audio data from I2S bus */
        i2s_read(I2S_NUM_1, audio_buffer, audio_chunksize * I2S_CHANNEL_NUM * sizeof(int16_t), &bytes_read, portMAX_DELAY);

        /* Adjust channel order and data */
        for (int i = 0; i < audio_chunksize; i++) {
            /* ref channel is used for acquire DAC output value */
            int16_t ref = audio_buffer[4 * i + 2];

            /* Channel order is Left - Right - Ref. Needs adjusted. */
            audio_buffer[3 * i + 0] = audio_buffer[4 * i + 0];
            audio_buffer[3 * i + 1] = audio_buffer[4 * i + 1];
            audio_buffer[3 * i + 2] = ref;
        }

        /* Feed samples of an audio stream to the AFE_SR */
        afe_handle->feed(afe_data, audio_buffer);

        xQueueSend(audio_queue_handle, &afe_data, portMAX_DELAY);
    }

    /* Clean up if audio feed ends */
    afe_handle->destroy(afe_data);

    /* Task never returns */
    vTaskDelete(NULL);
}

static void audio_detect_task(void *pvParam)
{
    int detect_flag = 0;
    multinet = &MULTINET_MODEL;
    model_data = multinet->create(&MULTINET_COEFF, 6000);
    esp_afe_sr_data_t *afe_data = (esp_afe_sr_data_t *) pvParam;

    /* Allocate buffer for detection */
    size_t afe_chunk_size = afe_handle->get_fetch_chunksize(afe_data);
    int16_t *detect_buff = heap_caps_malloc(afe_chunk_size * sizeof(int16_t), MALLOC_CAP_INTERNAL);
    if (NULL == detect_buff) {
        ESP_LOGE(TAG, "No mem for detect buffer");
        esp_system_abort("No mem for detect buffer");
    }

    /* Check for chunk size */
    if (afe_chunk_size != multinet->get_samp_chunksize(model_data)) {
        ESP_LOGE(TAG, "Invalid chunk size");

        abort();
    }

    void *queue_data = NULL;

    /* Start audio detection */
    ESP_LOGI(TAG, "Audio detect start");

    while (true) {
        xQueueReceive(audio_queue_handle, &queue_data, portMAX_DELAY);

        /* Wake word detected if `fetch()` returns none zero value */
        if (afe_handle->fetch(afe_data, detect_buff) > 0) {
            ESP_LOGI(TAG, LOG_BOLD(LOG_COLOR_GREEN) "Wakeword detected");
            detect_flag = 1;
            app_sr_set_event_flag(SR_EVENT_WAKE);

            /* Disable wakenet and aec. Detect command(s) with multinet. */
            afe_handle->disable_wakenet(afe_data);
            afe_handle->disable_aec(afe_data);
        }

        if (detect_flag == 1) {
            sr_command_id = multinet->detect(model_data, detect_buff);

            if (sr_command_id > -1) {
                ESP_LOGI(TAG, LOG_BOLD(LOG_COLOR_GREEN) "Command word %2d detected", sr_command_id);
                app_sr_set_event_flag(SR_EVENT_WORD_DETECT);

                /* Enable wakenet and AEC again */
                afe_handle->enable_wakenet(afe_data);
                afe_handle->enable_aec(afe_data);

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
    /**
     * @brief Init codec and I2C, I2S bus.
     * 
     * @note  Provided by AI group. Needs to clean up.
     * 
     */
    codec_init();

    /* AFE config */
    int afe_perferred_core = 0;
    afe_handle = &esp_afe_sr_2mic;
    int afe_mode = SR_MODE_STEREO_HIGH_PERF;
    esp_afe_sr_data_t *afe_data = afe_handle->create(afe_mode, afe_perferred_core);
    afe_handle->set_wakenet(afe_data, (esp_wn_iface_t *) &WAKENET_MODEL, &WAKENET_COEFF);

    /* Create audio queue to control audio feed and fetch */
    audio_queue_handle = xQueueCreate(4, sizeof(void *));
    if (NULL == audio_queue_handle) {
        ESP_LOGE(TAG, "No mem for audio queue");
        esp_system_abort("No mem for audio queue");
    }

    /* Create audio feed task. Acquire data from I2S bus */
    BaseType_t ret_val = xTaskCreatePinnedToCore(
        (TaskFunction_t)        audio_feed_task,
        (const char * const)    "Feed Task",
        (const uint32_t)        4 * 1024,
        (void * const)          afe_data,
        (UBaseType_t)           configMAX_PRIORITIES - 2,
        (TaskHandle_t * const)  NULL,
        (const BaseType_t)      1);
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

    return ESP_OK;
}

esp_err_t app_sr_reset_multinet(const char *cmd_on, const char *cmd_off)
{
    if (NULL == multinet) {
        return ESP_FAIL;
    }

    if (NULL == cmd_on || NULL == cmd_off) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t len_prev_word = strlen(old_commands_str);
    size_t len_new_word = strlen(cmd_on) + strlen(cmd_off);
    char *new_word_total = heap_caps_malloc(len_prev_word + len_new_word + 4, MALLOC_CAP_INTERNAL);
    strcpy(new_word_total, old_commands_str);
    strcat(new_word_total, cmd_off);
    strcat(new_word_total, ";");
    strcat(new_word_total, cmd_on);
    strcat(new_word_total, ";");
    char *error_phrase_id = heap_caps_calloc(32, 1, MALLOC_CAP_INTERNAL);

    multinet->reset(model_data, new_word_total, error_phrase_id);
    ESP_LOGI(TAG, "Error phrase :\n%s", error_phrase_id);
    memset(error_phrase_id, 0, 32);
    free(error_phrase_id);
    return ESP_OK;
}

esp_err_t app_sr_set_event_flag(sr_event_flag_t event_flag)
{
    sr_event_id |= (1 << event_flag);
    return ESP_OK;
}

esp_err_t app_sr_clear_event_flag(sr_event_flag_t event_flag)
{
    sr_event_id &= ~(1 << event_flag);
    return ESP_OK;
}

bool app_sr_get_event_flag(sr_event_flag_t event_flag)
{
    if (sr_event_id & (1 << event_flag)) {
        return true;
    }

    return false;
}

int32_t app_sr_get_command_id(void)
{
    return sr_command_id;
}
