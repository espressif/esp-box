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
#include <esp_err.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>

esp_rmaker_param_t *esp_rmaker_name_param_create(const char *param_name, const char *val)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_NAME,
            esp_rmaker_str(val), PROP_FLAG_READ | PROP_FLAG_WRITE | PROP_FLAG_PERSIST);
    return param;
}

esp_rmaker_param_t *esp_rmaker_power_param_create(const char *param_name, bool val)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_POWER,
            esp_rmaker_bool(val), PROP_FLAG_READ | PROP_FLAG_WRITE);
    if (param) {
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_TOGGLE);
    }
    return param;
}

esp_rmaker_param_t *esp_rmaker_brightness_param_create(const char *param_name, int val)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_BRIGHTNESS,
            esp_rmaker_int(val), PROP_FLAG_READ | PROP_FLAG_WRITE);
    if (param) {
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_SLIDER);
        esp_rmaker_param_add_bounds(param, esp_rmaker_int(0), esp_rmaker_int(100), esp_rmaker_int(1));
    }
    return param;
}

esp_rmaker_param_t *esp_rmaker_hue_param_create(const char *param_name, int val)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_HUE,
            esp_rmaker_int(val), PROP_FLAG_READ | PROP_FLAG_WRITE);
    if (param) {
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_HUE_SLIDER);
        esp_rmaker_param_add_bounds(param, esp_rmaker_int(0), esp_rmaker_int(360), esp_rmaker_int(1));
    }
    return param;
}

esp_rmaker_param_t *esp_rmaker_saturation_param_create(const char *param_name, int val)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_SATURATION,
            esp_rmaker_int(val), PROP_FLAG_READ | PROP_FLAG_WRITE);
    if (param) {
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_SLIDER);
        esp_rmaker_param_add_bounds(param, esp_rmaker_int(0), esp_rmaker_int(100), esp_rmaker_int(1));
    }
    return param;
}

esp_rmaker_param_t *esp_rmaker_intensity_param_create(const char *param_name, int val)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_INTENSITY,
            esp_rmaker_int(val), PROP_FLAG_READ | PROP_FLAG_WRITE);
    if (param) {
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_SLIDER);
        esp_rmaker_param_add_bounds(param, esp_rmaker_int(0), esp_rmaker_int(100), esp_rmaker_int(1));
    }
    return param;
}

esp_rmaker_param_t *esp_rmaker_cct_param_create(const char *param_name, int val)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_CCT,
            esp_rmaker_int(val), PROP_FLAG_READ | PROP_FLAG_WRITE);
    if (param) {
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_SLIDER);
        esp_rmaker_param_add_bounds(param, esp_rmaker_int(2700), esp_rmaker_int(6500), esp_rmaker_int(100));
    }
    return param;
}

esp_rmaker_param_t *esp_rmaker_direction_param_create(const char *param_name, int val)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_DIRECTION,
            esp_rmaker_int(val), PROP_FLAG_READ | PROP_FLAG_WRITE);
    if (param) {
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_DROPDOWN);
        esp_rmaker_param_add_bounds(param, esp_rmaker_int(0), esp_rmaker_int(1), esp_rmaker_int(1));
    }
    return param;
}

esp_rmaker_param_t *esp_rmaker_speed_param_create(const char *param_name, int val)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_SPEED,
            esp_rmaker_int(val), PROP_FLAG_READ | PROP_FLAG_WRITE);
    if (param) {
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_SLIDER);
        esp_rmaker_param_add_bounds(param, esp_rmaker_int(0), esp_rmaker_int(5), esp_rmaker_int(1));
    }
    return param;
}

esp_rmaker_param_t *esp_rmaker_temperature_param_create(const char *param_name, float val)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_TEMPERATURE,
            esp_rmaker_float(val), PROP_FLAG_READ | PROP_FLAG_TIME_SERIES);
    if (param) {
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_TEXT);
    }
    return param;
}

esp_rmaker_param_t *esp_rmaker_ota_status_param_create(const char *param_name)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_OTA_STATUS,
            esp_rmaker_str(""), PROP_FLAG_READ);
    return param;
}

esp_rmaker_param_t *esp_rmaker_ota_info_param_create(const char *param_name)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_OTA_INFO,
            esp_rmaker_str(""), PROP_FLAG_READ);
    return param;
}

esp_rmaker_param_t *esp_rmaker_ota_url_param_create(const char *param_name)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_OTA_URL,
            esp_rmaker_str(""), PROP_FLAG_WRITE);
    return param;
}

esp_rmaker_param_t *esp_rmaker_timezone_param_create(const char *param_name, const char *val)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_TIMEZONE,
            esp_rmaker_str(val), PROP_FLAG_READ | PROP_FLAG_WRITE);
    return param;
}

esp_rmaker_param_t *esp_rmaker_timezone_posix_param_create(const char *param_name, const char *val)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_TIMEZONE_POSIX,
            esp_rmaker_str(val), PROP_FLAG_READ | PROP_FLAG_WRITE);
    return param;
}

esp_rmaker_param_t *esp_rmaker_schedules_param_create(const char *param_name, int max_schedules)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_SCHEDULES,
            esp_rmaker_array("[]"), PROP_FLAG_READ | PROP_FLAG_WRITE | PROP_FLAG_PERSIST);
    esp_rmaker_param_add_array_max_count(param, max_schedules);
    return param;
}

esp_rmaker_param_t *esp_rmaker_scenes_param_create(const char *param_name, int max_scenes)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_SCENES,
            esp_rmaker_array("[]"), PROP_FLAG_READ | PROP_FLAG_WRITE | PROP_FLAG_PERSIST);
    esp_rmaker_param_add_array_max_count(param, max_scenes);
    return param;
}

esp_rmaker_param_t *esp_rmaker_reboot_param_create(const char *param_name)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_REBOOT,
            esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    return param;
}

esp_rmaker_param_t *esp_rmaker_factory_reset_param_create(const char *param_name)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_FACTORY_RESET,
            esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    return param;
}

esp_rmaker_param_t *esp_rmaker_wifi_reset_param_create(const char *param_name)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_WIFI_RESET,
            esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    return param;
}

esp_rmaker_param_t *esp_rmaker_local_control_pop_param_create(const char *param_name, const char *val)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_LOCAL_CONTROL_POP,
            esp_rmaker_str(val), PROP_FLAG_READ);
    return param;
}

esp_rmaker_param_t *esp_rmaker_local_control_type_param_create(const char *param_name, int val)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_LOCAL_CONTROL_TYPE,
            esp_rmaker_int(val), PROP_FLAG_READ);
    return param;
}
