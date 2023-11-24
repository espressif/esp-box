/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_check.h"

#include "app_nvs.h"
#include "app_pump.h"

static const char *TAG = "app_nvs";

/*nvs read write utils*/
esp_err_t app_nvs_set_watering_time(int time)
{
    esp_err_t ret;
    nvs_handle handle;

    ret = nvs_open(NVS_NAMESPACE_APP_WATERING_CFG, NVS_READWRITE, &handle);
    ESP_RETURN_ON_ERROR(ret, TAG, "nvs open failed");
    ret = nvs_set_i32(handle, "watering_time", time);
    ESP_RETURN_ON_ERROR(ret, TAG, "nvs set failed");
    nvs_commit(handle);
    nvs_close(handle);
    return ret;
}

esp_err_t app_nvs_get_watering_time(int *time)
{
    esp_err_t ret;
    nvs_handle handle;
    if (!time) {
        return ESP_ERR_INVALID_ARG;
    }
    ret = nvs_open(NVS_NAMESPACE_APP_WATERING_CFG, NVS_READWRITE, &handle);
    ESP_RETURN_ON_ERROR(ret, TAG, "nvs open failed");
    ret = nvs_get_i32(handle, "watering_time", (int32_t *) time);
    nvs_close(handle);

    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "watering set to default %d", APP_PUMP_WATERING_DEFAULT_TIME);
        ret = app_nvs_set_watering_time(APP_PUMP_WATERING_DEFAULT_TIME);
        ESP_RETURN_ON_ERROR(ret, TAG, "nvs set failed");
        *time = APP_PUMP_WATERING_DEFAULT_TIME;
    }
    return ret;
}

esp_err_t app_nvs_set_lower_humidity(int humidity)
{
    esp_err_t ret;
    nvs_handle handle;

    ret = nvs_open(NVS_NAMESPACE_APP_WATERING_CFG, NVS_READWRITE, &handle);
    ESP_RETURN_ON_ERROR(ret, TAG, "nvs open failed");
    ret = nvs_set_i32(handle, "lower_humidity", humidity);
    ESP_RETURN_ON_ERROR(ret, TAG, "nvs set failed");
    nvs_commit(handle);
    nvs_close(handle);
    return ret;
}

esp_err_t app_nvs_get_lower_humidity(int *humidity)
{
    esp_err_t ret;
    nvs_handle handle;
    if (!humidity) {
        return ESP_ERR_INVALID_ARG;
    }
    ret = nvs_open(NVS_NAMESPACE_APP_WATERING_CFG, NVS_READWRITE, &handle);
    ESP_RETURN_ON_ERROR(ret, TAG, "nvs open failed");
    ret = nvs_get_i32(handle, "lower_humidity", (int32_t *) humidity);
    nvs_close(handle);

    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "min humidity set to default %d", APP_PUMP_WATERING_LOWER_HUMIDITY);
        ret = app_nvs_set_lower_humidity(APP_PUMP_WATERING_LOWER_HUMIDITY);
        ESP_RETURN_ON_ERROR(ret, TAG, "nvs set failed");
        *humidity = APP_PUMP_WATERING_LOWER_HUMIDITY;
    }
    return ret;
}

esp_err_t app_nvs_set_auto_watering_enable(bool on)
{
    esp_err_t ret;
    nvs_handle handle;

    ret = nvs_open(NVS_NAMESPACE_APP_WATERING_CFG, NVS_READWRITE, &handle);
    ESP_RETURN_ON_ERROR(ret, TAG, "nvs open failed");
    ret = nvs_set_i8(handle, "auto_watering", (int8_t)on);
    ESP_RETURN_ON_ERROR(ret, TAG, "nvs set failed");
    nvs_commit(handle);
    nvs_close(handle);
    return ret;
}

esp_err_t app_nvs_get_auto_watering_enable(bool *on)
{
    esp_err_t ret;
    nvs_handle handle;
    if (!on) {
        return ESP_ERR_INVALID_ARG;
    }

    ret = nvs_open(NVS_NAMESPACE_APP_WATERING_CFG, NVS_READWRITE, &handle);
    ESP_RETURN_ON_ERROR(ret, TAG, "nvs open failed");
    ret = nvs_get_i8(handle, "auto_watering", (int8_t *)on);
    nvs_close(handle);

    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "min humidity set to default %d", APP_PUMP_WATERING_AUTO_ENABLE);
        ret = app_nvs_set_auto_watering_enable(APP_PUMP_WATERING_AUTO_ENABLE);
        ESP_RETURN_ON_ERROR(ret, TAG, "nvs set failed");
        *on = APP_PUMP_WATERING_AUTO_ENABLE;
    }
    return ret;
}
