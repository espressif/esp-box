/*
* SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Unlicense OR CC0-1.0
*/


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "app_audio.h"
#include "app_sr.h"
#include "bsp_codec.h"
#include "bsp_i2s.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_models.h"
#include "app_sr_handler.h"


#define I2S_CHANNEL_NUM     (2)

static const char *TAG = "app_sr";

static model_iface_data_t *model_data = NULL;
static const esp_mn_iface_t *multinet = &MULTINET_MODEL;
static const esp_afe_sr_iface_t *afe_handle = &ESP_AFE_HANDLE;
static QueueHandle_t g_result_que = NULL;

static FILE *fp = NULL;
static bool b_record_en = false;

static void audio_feed_task(void *pvParam)
{
    size_t bytes_read = 0;
    esp_afe_sr_data_t *afe_data = (esp_afe_sr_data_t *) pvParam;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    ESP_LOGI(TAG, "audio_chunksize=%d, feed_channel=%d", audio_chunksize, 3);
    /* Allocate audio buffer and check for result */
    int16_t *audio_buffer = heap_caps_malloc(audio_chunksize * sizeof(int16_t) * 3, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
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
    int16_t *detect_buff = heap_caps_malloc(afe_chunk_size * sizeof(int16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
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
        afe_fetch_mode_t ret_val = afe_handle->fetch(afe_data, detect_buff);

        if (AFE_FETCH_WWE_DETECTED == ret_val) {
            ESP_LOGI(TAG, LOG_BOLD(LOG_COLOR_GREEN) "Wakeword detected");
            sr_result_t result = {
                .fetch_mode = ret_val,
                .state = ESP_MN_STATE_DETECTING,
                .command_id = 0,
            };
            xQueueSend(g_result_que, &result, 10);
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

            esp_mn_state_t mn_state = ESP_MN_STATE_DETECTING;
            if (false == app_audio_beep_is_playing()) {
                mn_state = multinet->detect(model_data, detect_buff);
            } else {
                continue;
            }

            if (ESP_MN_STATE_DETECTING == mn_state) {
                continue;
            }

            if (ESP_MN_STATE_TIMEOUT == mn_state) {
                ESP_LOGW(TAG, "Time out");
                sr_result_t result = {
                    .fetch_mode = ret_val,
                    .state = mn_state,
                    .command_id = 0,
                };
                xQueueSend(g_result_que, &result, 10);
                afe_handle->enable_wakenet(afe_data);
                detect_flag = false;
                continue;
            }

            if (ESP_MN_STATE_DETECTED <= mn_state) {
                esp_mn_results_t *mn_result = multinet->get_results(model_data);
                for (int i = 0; i < mn_result->num; i++) {
                    ESP_LOGI(TAG, "TOP %d, command_id: %d, phrase_id: %d, prob: %f",
                             i + 1, mn_result->command_id[i], mn_result->phrase_id[i], mn_result->prob[i]);
                }

                int sr_command_id = mn_result->command_id[0];
                ESP_LOGI(TAG, "Deteted command : %d", sr_command_id);
                sr_result_t result = {
                    .fetch_mode = ret_val,
                    .state = mn_state,
                    .command_id = sr_command_id,
                };
                xQueueSend(g_result_que, &result, 10);
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

esp_err_t app_sr_start(bool record_en)
{
    g_result_que = xQueueCreate(1, sizeof(sr_result_t));
    ESP_RETURN_ON_FALSE(NULL != g_result_que, ESP_ERR_NO_MEM, TAG, "Failed create result queue");

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

    afe_config_t afe_config = AFE_CONFIG_DEFAULT();
    afe_config.aec_init = false;
    // afe_config.vad_init = false;
    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(&afe_config);
    model_data = multinet->create((const model_coeff_getter_t *) &MULTINET_COEFF, 5760);

    BaseType_t ret_val = xTaskCreatePinnedToCore(audio_feed_task, "Feed Task", 4 * 1024, afe_data, 5, NULL, 1);
    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_FAIL, TAG,  "Failed create audio feed task");

    ret_val = xTaskCreatePinnedToCore(audio_detect_task, "Detect Task", 6 * 1024, afe_data, 5, NULL, 1);
    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_FAIL, TAG,  "Failed create audio detect task");

    ret_val = xTaskCreatePinnedToCore(sr_handler_task, "SR Handler Task", 4 * 1024, g_result_que, 1, NULL, 0);
    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_FAIL, TAG,  "Failed create audio handler task");

    return ESP_OK;
}

esp_err_t app_sr_reset_command_list(char *command_list)
{
    char *err_id = heap_caps_malloc(1024, MALLOC_CAP_SPIRAM);
    ESP_RETURN_ON_FALSE(NULL != err_id, ESP_ERR_NO_MEM, TAG,  "memory is not enough");
    multinet->reset(model_data, command_list, err_id);
    free(err_id);
    return ESP_OK;
}
