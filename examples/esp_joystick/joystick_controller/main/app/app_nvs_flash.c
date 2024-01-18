/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "app_nvs_flash.h"

nvs_handle_t nvs_flash_handle;

void flash_write_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void flash_write_state(char *key, char *value)
{
    nvs_open_from_partition(NVS_PART_NAME, NVS_PART_NAMESPACE, NVS_READWRITE, &nvs_flash_handle);

    ESP_ERROR_CHECK(nvs_set_str(nvs_flash_handle, key, value));
    ESP_ERROR_CHECK(nvs_commit(nvs_flash_handle));
    nvs_close(nvs_flash_handle);
}

uint16_t read_rocker_value_from_flash(char *key)
{
    nvs_open_from_partition(NVS_PART_NAME, NVS_PART_NAMESPACE, NVS_READWRITE, &nvs_flash_handle);
    char test[10] = {0};
    uint16_t num = 0;
    size_t length = sizeof(test);
    esp_err_t err = nvs_get_str(nvs_flash_handle, key, test, &length);
    nvs_close(nvs_flash_handle);
    if (err != ESP_OK || length == 0) {
        return 2;
    } else {
        num = atoi(test);
        return num;
    }
}
