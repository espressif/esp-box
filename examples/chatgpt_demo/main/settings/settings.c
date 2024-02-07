/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_check.h"
#include "bsp/esp-bsp.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "settings.h"
#include "esp_ota_ops.h"

static const char *TAG = "settings";
const char *uf2_nvs_partition = "nvs";
const char *uf2_nvs_namespace = "configuration";
static nvs_handle_t my_handle;
static sys_param_t g_sys_param = {0};

esp_err_t settings_factory_reset(void)
{
    const esp_partition_t *update_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    ESP_LOGI(TAG, "Switch to partition UF2");
    esp_ota_set_boot_partition(update_partition);
    esp_restart();
    return ESP_OK;
}

esp_err_t settings_read_parameter_from_nvs(void)
{
    esp_err_t ret = nvs_open_from_partition(uf2_nvs_partition, uf2_nvs_namespace, NVS_READONLY, &my_handle);
    if (ESP_ERR_NVS_NOT_FOUND == ret) {
        ESP_LOGI(TAG, "Credentials not found");
        goto err;
    }

    ESP_GOTO_ON_FALSE(ESP_OK == ret, ret, err, TAG, "nvs open failed (0x%x)", ret);
    size_t len = 0;

    // Read SSID
    len = sizeof(g_sys_param.ssid);
    ret = nvs_get_str(my_handle, "ssid", g_sys_param.ssid, &len);
    if (ret != ESP_OK || len == 0) {
        ESP_LOGI(TAG, "No SSID found");
        goto err;
    }

    // Read password
    len = sizeof(g_sys_param.password);
    ret = nvs_get_str(my_handle, "password", g_sys_param.password, &len);
    if (ret != ESP_OK || len == 0) {
        ESP_LOGI(TAG, "No Password found");
        goto err;
    }

    // Read key
    len = sizeof(g_sys_param.key);
    ret = nvs_get_str(my_handle, "ChatGPT_key", g_sys_param.key, &len);
    if (ret != ESP_OK || len == 0) {
        ESP_LOGI(TAG, "No OpenAI key found");
        goto err;
    }

    // Read url
    len = sizeof(g_sys_param.url);
    ret = nvs_get_str(my_handle, "Base_url", g_sys_param.url, &len);
    if (ret != ESP_OK || len == 0) {
        ESP_LOGI(TAG, "No OpenAI Base url found");
        goto err;
    }

    nvs_close(my_handle);

    ESP_LOGI(TAG, "stored ssid:%s", g_sys_param.ssid);
    ESP_LOGI(TAG, "stored password:%s", g_sys_param.password);
    ESP_LOGI(TAG, "stored OpenAI:%s", g_sys_param.key);
    ESP_LOGI(TAG, "stored Base URL:%s", g_sys_param.url);
    return ESP_OK;

err:
    if (my_handle) {
        nvs_close(my_handle);
    }
    settings_factory_reset();
    return ret;
}

sys_param_t *settings_get_parameter(void)
{
    return &g_sys_param;
}
