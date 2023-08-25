// Copyright 2021 Espressif Systems (Shanghai) PTE LTD
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
#include <esp_rmaker_core.h>
#include <esp_rmaker_utils.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_services.h>

static const char *TAG = "esp_rmaker_system_service";

#define ESP_RMAKER_SYSTEM_SERV_NAME     "System"

static esp_err_t esp_rmaker_system_serv_write_cb(const esp_rmaker_device_t *device,
        const esp_rmaker_param_t *param, const esp_rmaker_param_val_t val,
        void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    esp_err_t err = ESP_OK;
    esp_rmaker_system_serv_config_t *config = (esp_rmaker_system_serv_config_t *)priv_data;
    if (strcmp(esp_rmaker_param_get_type(param), ESP_RMAKER_PARAM_REBOOT) == 0) {
        if (val.val.b == true) {
            err = esp_rmaker_reboot(config->reboot_seconds);
        }
    } else if (strcmp(esp_rmaker_param_get_type(param), ESP_RMAKER_PARAM_FACTORY_RESET) == 0) {
        if (val.val.b == true) {
            err = esp_rmaker_factory_reset(config->reset_seconds, config->reset_reboot_seconds);
        }
    } else if (strcmp(esp_rmaker_param_get_type(param), ESP_RMAKER_PARAM_WIFI_RESET) == 0) {
        if (val.val.b == true) {
            err = esp_rmaker_wifi_reset(config->reset_seconds, config->reset_reboot_seconds);
        }
    } else {
        return ESP_FAIL;
    }

    if (err == ESP_OK) {
        esp_rmaker_param_update_and_report(param, val);
    }
    return err;
}

esp_err_t esp_rmaker_system_service_enable(esp_rmaker_system_serv_config_t *config)
{
    if ((config->flags & SYSTEM_SERV_FLAGS_ALL) == 0) {
        ESP_LOGE(TAG, "Atleast one flag should be set for system service.");
        return ESP_ERR_INVALID_ARG;
    }
    esp_rmaker_system_serv_config_t *priv_config = MEM_CALLOC_EXTRAM(1, sizeof(esp_rmaker_system_serv_config_t));
    if (!priv_config) {
        ESP_LOGE(TAG, "Failed to allocate data for system service config.");
        return ESP_ERR_NO_MEM;
    }
    *priv_config = *config;
    esp_rmaker_device_t *service = esp_rmaker_create_system_service(ESP_RMAKER_SYSTEM_SERV_NAME, (void *)priv_config);
    if (service) {
        esp_rmaker_device_add_cb(service, esp_rmaker_system_serv_write_cb, NULL);
        if (priv_config->flags & SYSTEM_SERV_FLAG_REBOOT) {
            esp_rmaker_device_add_param(service, esp_rmaker_reboot_param_create(ESP_RMAKER_DEF_REBOOT_NAME));
        }
        if (priv_config->flags & SYSTEM_SERV_FLAG_FACTORY_RESET) {
            esp_rmaker_device_add_param(service, esp_rmaker_factory_reset_param_create(ESP_RMAKER_DEF_FACTORY_RESET_NAME));
        }
        if (priv_config->flags & SYSTEM_SERV_FLAG_WIFI_RESET) {
            esp_rmaker_device_add_param(service, esp_rmaker_wifi_reset_param_create(ESP_RMAKER_DEF_WIFI_RESET_NAME));
        }
        esp_err_t err = esp_rmaker_node_add_device(esp_rmaker_get_node(), service);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "System service enabled.");
        } else {
            esp_rmaker_device_delete(service);
        }
        return err;
    } else {
        free(priv_config);
        ESP_LOGE(TAG, "Failed to create System service.");
    }
    return ESP_ERR_NO_MEM;
}
