// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdint.h>
#include <string.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_rmaker_work_queue.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_services.h>
#include <esp_rmaker_ota.h>

#include "esp_rmaker_ota_internal.h"
#include "esp_rmaker_mqtt.h"

static const char *TAG = "esp_rmaker_ota_using_params";

#define ESP_RMAKER_OTA_SERV_NAME    "OTA"

void esp_rmaker_ota_finish_using_params(esp_rmaker_ota_t *ota)
{
    if (ota->url) {
        free(ota->url);
        ota->url = NULL;
    }
    ota->filesize = 0;
    if (ota->transient_priv) {
        ota->transient_priv = NULL;
    }
    if (ota->metadata) {
        free(ota->metadata);
        ota->metadata = NULL;
    }
    ota->ota_in_progress = false;
}
static esp_err_t esp_rmaker_ota_service_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
        const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    esp_rmaker_ota_t *ota = (esp_rmaker_ota_t *)priv_data;
    if (!ota) {
        ESP_LOGE(TAG, "No OTA specific data received in callback");
        return ESP_FAIL;
    }
    if (ota->ota_in_progress) {
        ESP_LOGE(TAG, "OTA already in progress. Please try later.");
        return ESP_FAIL;
    }
    if (strcmp(esp_rmaker_param_get_type(param), ESP_RMAKER_PARAM_OTA_URL) == 0) {
        ESP_LOGI(TAG, "Received value = %s for %s - %s",
                val.val.s, esp_rmaker_device_get_name(device), esp_rmaker_param_get_name(param));
        if (ota->url) {
            free(ota->url);
            ota->url = NULL;
        }
        ota->url = strdup(val.val.s);
        if (ota->url) {
            ota->filesize = 0;
            ota->ota_in_progress = true;
            ota->transient_priv = (void *)device;
            ota->metadata = NULL;
            if (esp_rmaker_work_queue_add_task(esp_rmaker_ota_common_cb, ota) != ESP_OK) {
                esp_rmaker_ota_finish_using_params(ota);
            } else {
                return ESP_OK;
            }
        }

    }
    return ESP_FAIL;
}

esp_err_t esp_rmaker_ota_report_status_using_params(esp_rmaker_ota_handle_t ota_handle, ota_status_t status, char *additional_info)
{
    const esp_rmaker_device_t *device = esp_rmaker_node_get_device_by_name(esp_rmaker_get_node(), ESP_RMAKER_OTA_SERV_NAME);
    if (!device) {
        return ESP_FAIL;
    }
    esp_rmaker_param_t *info_param = esp_rmaker_device_get_param_by_type(device, ESP_RMAKER_PARAM_OTA_INFO);
    esp_rmaker_param_t *status_param = esp_rmaker_device_get_param_by_type(device, ESP_RMAKER_PARAM_OTA_STATUS);

    esp_rmaker_param_update_and_report(info_param, esp_rmaker_str(additional_info));
    esp_rmaker_param_update_and_report(status_param, esp_rmaker_str(esp_rmaker_ota_status_to_string(status)));
    
    return ESP_OK;
}

/* Enable the ESP RainMaker specific OTA */
esp_err_t esp_rmaker_ota_enable_using_params(esp_rmaker_ota_t *ota)
{
    esp_rmaker_device_t *service = esp_rmaker_ota_service_create(ESP_RMAKER_OTA_SERV_NAME, ota);
    if (!service) {
        ESP_LOGE(TAG, "Failed to create OTA Service");
        return ESP_FAIL;
    }
    esp_rmaker_device_add_cb(service, esp_rmaker_ota_service_cb, NULL);
    esp_err_t err = esp_rmaker_node_add_device(esp_rmaker_get_node(), service);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "OTA enabled with Params");
    }
    return err;
}
