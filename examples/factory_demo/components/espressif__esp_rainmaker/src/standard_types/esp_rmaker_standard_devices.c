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

esp_rmaker_device_t *esp_rmaker_switch_device_create(const char *dev_name,
        void *priv_data, bool power)
{
    esp_rmaker_device_t *device = esp_rmaker_device_create(dev_name, ESP_RMAKER_DEVICE_SWITCH, priv_data);
    if (device) {
        esp_rmaker_device_add_param(device, esp_rmaker_name_param_create(ESP_RMAKER_DEF_NAME_PARAM, dev_name));
        esp_rmaker_param_t *primary = esp_rmaker_power_param_create(ESP_RMAKER_DEF_POWER_NAME, power);
        esp_rmaker_device_add_param(device, primary);
        esp_rmaker_device_assign_primary_param(device, primary);
	}
    return device;
}

esp_rmaker_device_t *esp_rmaker_lightbulb_device_create(const char *dev_name,
        void *priv_data, bool power)
{
    esp_rmaker_device_t *device = esp_rmaker_device_create(dev_name, ESP_RMAKER_DEVICE_LIGHTBULB, priv_data);
    if (device) {
        esp_rmaker_device_add_param(device, esp_rmaker_name_param_create(ESP_RMAKER_DEF_NAME_PARAM, dev_name));
        esp_rmaker_param_t *primary = esp_rmaker_power_param_create(ESP_RMAKER_DEF_POWER_NAME, power);
        esp_rmaker_device_add_param(device, primary);
        esp_rmaker_device_assign_primary_param(device, primary);
    }
    return device;
}

esp_rmaker_device_t *esp_rmaker_fan_device_create(const char *dev_name,
        void *priv_data, bool power)
{
    esp_rmaker_device_t *device = esp_rmaker_device_create(dev_name, ESP_RMAKER_DEVICE_FAN, priv_data);
    if (device) {
        esp_rmaker_device_add_param(device, esp_rmaker_name_param_create(ESP_RMAKER_DEF_NAME_PARAM, dev_name));
        esp_rmaker_param_t *primary = esp_rmaker_power_param_create(ESP_RMAKER_DEF_POWER_NAME, power);
        esp_rmaker_device_add_param(device, primary);
        esp_rmaker_device_assign_primary_param(device, primary);
    }
    return device;
}

esp_rmaker_device_t *esp_rmaker_temp_sensor_device_create(const char *dev_name,
        void *priv_data, float temperature)
{
    esp_rmaker_device_t *device = esp_rmaker_device_create(dev_name, ESP_RMAKER_DEVICE_TEMP_SENSOR, priv_data);
    if (device) {
        esp_rmaker_device_add_param(device, esp_rmaker_name_param_create(ESP_RMAKER_DEF_NAME_PARAM, dev_name));
        esp_rmaker_param_t *primary = esp_rmaker_temperature_param_create(ESP_RMAKER_DEF_TEMPERATURE_NAME, temperature);
        esp_rmaker_device_add_param(device, primary);
        esp_rmaker_device_assign_primary_param(device, primary);
	}
    return device;
}
