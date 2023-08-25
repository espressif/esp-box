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


#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>

esp_rmaker_device_t *esp_rmaker_ota_service_create(const char *serv_name, void *priv_data)
{
    esp_rmaker_device_t *service = esp_rmaker_service_create(serv_name, ESP_RMAKER_SERVICE_OTA, priv_data);
    if (service) {
        esp_rmaker_device_add_param(service, esp_rmaker_ota_status_param_create(ESP_RMAKER_DEF_OTA_STATUS_NAME));
        esp_rmaker_device_add_param(service, esp_rmaker_ota_info_param_create(ESP_RMAKER_DEF_OTA_INFO_NAME));
        esp_rmaker_device_add_param(service, esp_rmaker_ota_url_param_create(ESP_RMAKER_DEF_OTA_URL_NAME));
    }
    return service;
}

esp_rmaker_device_t *esp_rmaker_time_service_create(const char *serv_name, const char *timezone,
        const char *timezone_posix, void *priv_data)
{
    esp_rmaker_device_t *service = esp_rmaker_service_create(serv_name, ESP_RMAKER_SERVICE_TIME, priv_data);
    if (service) {
        esp_rmaker_device_add_param(service, esp_rmaker_timezone_param_create(
                ESP_RMAKER_DEF_TIMEZONE_NAME, timezone));
        esp_rmaker_device_add_param(service, esp_rmaker_timezone_posix_param_create(
                ESP_RMAKER_DEF_TIMEZONE_POSIX_NAME, timezone_posix));
    }
    return service;
}

esp_rmaker_device_t *esp_rmaker_create_schedule_service(const char *serv_name, esp_rmaker_device_write_cb_t write_cb,
        esp_rmaker_device_read_cb_t read_cb, int max_schedules, void *priv_data)
{
    esp_rmaker_device_t *service = esp_rmaker_service_create(serv_name, ESP_RMAKER_SERVICE_SCHEDULE, priv_data);
    if (service) {
        esp_rmaker_device_add_cb(service, write_cb, read_cb);
        esp_rmaker_device_add_param(service, esp_rmaker_schedules_param_create(ESP_RMAKER_DEF_SCHEDULE_NAME, max_schedules));
    }
    return service;
}

esp_rmaker_device_t *esp_rmaker_create_scenes_service(const char *serv_name, esp_rmaker_device_write_cb_t write_cb,
        esp_rmaker_device_read_cb_t read_cb, int max_scenes, bool deactivation_support, void *priv_data)
{
    esp_rmaker_device_t *service = esp_rmaker_service_create(serv_name, ESP_RMAKER_SERVICE_SCENES, priv_data);
    if (service) {
        esp_rmaker_device_add_cb(service, write_cb, read_cb);
        esp_rmaker_device_add_param(service, esp_rmaker_scenes_param_create(ESP_RMAKER_DEF_SCENES_NAME, max_scenes));
        esp_rmaker_device_add_attribute(service, "deactivation_support", deactivation_support ? "yes" : "no");
    }
    return service;
}

esp_rmaker_device_t *esp_rmaker_create_system_service(const char *serv_name, void *priv_data)
{
    return esp_rmaker_service_create(serv_name, ESP_RMAKER_SERVICE_SYSTEM, priv_data);
}

esp_rmaker_device_t *esp_rmaker_create_local_control_service(const char *serv_name, const char *pop, int sec_type, void *priv_data)
{
    esp_rmaker_device_t *service = esp_rmaker_service_create(serv_name, ESP_RMAKER_SERVICE_LOCAL_CONTROL, priv_data);
    if (service) {
        esp_rmaker_device_add_param(service, esp_rmaker_local_control_pop_param_create(ESP_RMAKER_DEF_LOCAL_CONTROL_POP, pop));
        esp_rmaker_device_add_param(service, esp_rmaker_local_control_type_param_create(ESP_RMAKER_DEF_LOCAL_CONTROL_TYPE, sec_type));
    }
    return service;
}
