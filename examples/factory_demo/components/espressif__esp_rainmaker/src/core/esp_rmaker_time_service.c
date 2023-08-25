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
#include <string.h>
#include <esp_log.h>
#include <nvs.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_services.h>
#include <esp_rmaker_utils.h>

static const char *TAG = "esp_rmaker_time_service";

#define ESP_RMAKER_TIME_SERV_NAME           "Time"

static esp_err_t esp_rmaker_time_service_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
        const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    esp_err_t err = ESP_FAIL;
    if (strcmp(esp_rmaker_param_get_type(param), ESP_RMAKER_PARAM_TIMEZONE) == 0) {
        ESP_LOGI(TAG, "Received value = %s for %s - %s",
                val.val.s, esp_rmaker_device_get_name(device), esp_rmaker_param_get_name(param));
        err = esp_rmaker_time_set_timezone(val.val.s);
        if (err == ESP_OK) {
            char *tz_posix = esp_rmaker_time_get_timezone_posix();
            if (tz_posix) {
                esp_rmaker_param_t *tz_posix_param = esp_rmaker_device_get_param_by_type(
                        device, ESP_RMAKER_PARAM_TIMEZONE_POSIX);
                esp_rmaker_param_update_and_report(tz_posix_param, esp_rmaker_str(tz_posix));
                free(tz_posix);
            }
        }
    } else if (strcmp(esp_rmaker_param_get_type(param), ESP_RMAKER_PARAM_TIMEZONE_POSIX) == 0) {
        ESP_LOGI(TAG, "Received value = %s for %s - %s",
                val.val.s, esp_rmaker_device_get_name(device), esp_rmaker_param_get_name(param));
        err = esp_rmaker_time_set_timezone_posix(val.val.s);
    }
    if (err == ESP_OK) {
        esp_rmaker_param_update_and_report(param, val);
    }
    return err;
}

static esp_err_t esp_rmaker_time_add_service(const char *tz, const char *tz_posix)
{
    esp_rmaker_device_t *service = esp_rmaker_time_service_create(ESP_RMAKER_TIME_SERV_NAME, tz, tz_posix, NULL);
    if (!service) {
        ESP_LOGE(TAG, "Failed to create Time Service");
        return ESP_FAIL;
    }
    esp_rmaker_device_add_cb(service, esp_rmaker_time_service_cb, NULL);
    esp_err_t err = esp_rmaker_node_add_device(esp_rmaker_get_node(), service);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Time service enabled");
    } else {
        esp_rmaker_device_delete(service);
    }
    return err;
}

esp_err_t esp_rmaker_timezone_service_enable(void)
{
    char *tz_posix = esp_rmaker_time_get_timezone_posix();
    char *tz = esp_rmaker_time_get_timezone();
    esp_err_t err = esp_rmaker_time_add_service(tz, tz_posix);
    if (tz_posix) {
        free(tz_posix);
    }
    if (tz) {
        free(tz);
    }
    return err;
}
