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
#include <time.h>
#include <esp_log.h>
#include <nvs.h>
#include "esp_schedule_internal.h"

static const char *TAG = "esp_schedule_nvs";

#define ESP_SCHEDULE_NVS_NAMESPACE "schd"
#define ESP_SCHEDULE_COUNT_KEY "schd_count"

static char *esp_schedule_nvs_partition = NULL;
static bool nvs_enabled = false;

esp_err_t esp_schedule_nvs_add(esp_schedule_t *schedule)
{
    if (!nvs_enabled) {
        ESP_LOGD(TAG, "NVS not enabled. Not adding to NVS.");
        return ESP_ERR_INVALID_STATE;
    }
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open_from_partition(esp_schedule_nvs_partition, ESP_SCHEDULE_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed with error %d", err);
        return err;
    }

    /* Check if this is new schedule or editing an existing schedule */
    size_t buf_size;
    bool editing_schedule = true;
    err = nvs_get_blob(nvs_handle, schedule->name, NULL, &buf_size);
    if (err != ESP_OK) {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            editing_schedule = false;
        } else {
            ESP_LOGE(TAG, "NVS get failed with error %d", err);
            nvs_close(nvs_handle);
            return err;
        }
    } else {
        ESP_LOGI(TAG, "Updating the existing schedule %s", schedule->name);
    }

    err = nvs_set_blob(nvs_handle, schedule->name, schedule, sizeof(esp_schedule_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS set failed with error %d", err);
        nvs_close(nvs_handle);
        return err;
    }
    if (editing_schedule == false) {
        uint8_t schedule_count;
        err = nvs_get_u8(nvs_handle, ESP_SCHEDULE_COUNT_KEY, &schedule_count);
        if (err != ESP_OK) {
            if (err == ESP_ERR_NVS_NOT_FOUND) {
                schedule_count = 0;
            } else {
                ESP_LOGE(TAG, "NVS get failed with error %d", err);
                nvs_close(nvs_handle);
                return err;
            }
        }
        schedule_count++;
        err = nvs_set_u8(nvs_handle, ESP_SCHEDULE_COUNT_KEY, schedule_count);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "NVS set failed for schedule count with error %d", err);
            nvs_close(nvs_handle);
            return err;
        }
    }
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "Schedule %s added in NVS", schedule->name);
    return ESP_OK;
}

esp_err_t esp_schedule_nvs_remove_all(void)
{
    if (!nvs_enabled) {
        ESP_LOGD(TAG, "NVS not enabled. Not removing from NVS.");
        return ESP_ERR_INVALID_STATE;
    }
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open_from_partition(esp_schedule_nvs_partition, ESP_SCHEDULE_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed with error %d", err);
        return err;
    }
    err = nvs_erase_all(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS erase all keys failed with error %d", err);
        nvs_close(nvs_handle);
        return err;
    }
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "All schedules removed from NVS");
    return ESP_OK;
}

esp_err_t esp_schedule_nvs_remove(esp_schedule_t *schedule)
{
    if (!nvs_enabled) {
        ESP_LOGD(TAG, "NVS not enabled. Not removing from NVS.");
        return ESP_ERR_INVALID_STATE;
    }
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open_from_partition(esp_schedule_nvs_partition, ESP_SCHEDULE_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed with error %d", err);
        return err;
    }
    err = nvs_erase_key(nvs_handle, schedule->name);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS erase key failed with error %d", err);
        nvs_close(nvs_handle);
        return err;
    }
    uint8_t schedule_count;
    err = nvs_get_u8(nvs_handle, ESP_SCHEDULE_COUNT_KEY, &schedule_count);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS get failed for schedule count with error %d", err);
        nvs_close(nvs_handle);
        return err;
    }
    schedule_count--;
    err = nvs_set_u8(nvs_handle, ESP_SCHEDULE_COUNT_KEY, schedule_count);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS set failed for schedule count with error %d", err);
        nvs_close(nvs_handle);
        return err;
    }
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "Schedule %s removed from NVS", schedule->name);
    return ESP_OK;
}

static uint8_t esp_schedule_nvs_get_count(void)
{
    if (!nvs_enabled) {
        ESP_LOGD(TAG, "NVS not enabled. Not getting count from NVS.");
        return 0;
    }
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open_from_partition(esp_schedule_nvs_partition, ESP_SCHEDULE_NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed with error %d", err);
        return 0;
    }
    uint8_t schedule_count;
    err = nvs_get_u8(nvs_handle, ESP_SCHEDULE_COUNT_KEY, &schedule_count);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS get failed for schedule count with error %d", err);
        nvs_close(nvs_handle);
        return 0;
    }
    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "Schedules in NVS: %d", schedule_count);
    return schedule_count;
}

static esp_schedule_handle_t esp_schedule_nvs_get(char *nvs_key)
{
    if (!nvs_enabled) {
        ESP_LOGD(TAG, "NVS not enabled. Not getting from NVS.");
        return NULL;
    }
    size_t buf_size;
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open_from_partition(esp_schedule_nvs_partition, ESP_SCHEDULE_NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed with error %d", err);
        return NULL;
    }
    err = nvs_get_blob(nvs_handle, nvs_key, NULL, &buf_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS get failed with error %d", err);
        nvs_close(nvs_handle);
        return NULL;
    }
    esp_schedule_t *schedule = (esp_schedule_t *)malloc(buf_size);
    if (schedule == NULL) {
        ESP_LOGE(TAG, "Could not allocate handle");
        nvs_close(nvs_handle);
        return NULL;
    }
    err = nvs_get_blob(nvs_handle, nvs_key, schedule, &buf_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS get failed with error %d", err);
        nvs_close(nvs_handle);
        free(schedule);
        return NULL;
    }
    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "Schedule %s found in NVS", schedule->name);
    return (esp_schedule_handle_t) schedule;
}

esp_schedule_handle_t *esp_schedule_nvs_get_all(uint8_t *schedule_count)
{
    if (!nvs_enabled) {
        ESP_LOGD(TAG, "NVS not enabled. Not Initialising NVS.");
        return NULL;
    }

    *schedule_count = esp_schedule_nvs_get_count();
    if (*schedule_count == 0) {
        ESP_LOGI(TAG, "No Entries found in NVS");
        return NULL;
    }
    esp_schedule_handle_t *handle_list = (esp_schedule_handle_t *)malloc(sizeof(esp_schedule_handle_t) * (*schedule_count));
    if (handle_list == NULL) {
        ESP_LOGE(TAG, "Could not allocate schedule list");
        *schedule_count = 0;
        return NULL;
    }
    int handle_count = 0;

    nvs_entry_info_t nvs_entry;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    nvs_iterator_t nvs_iterator = NULL;
    esp_err_t err = nvs_entry_find(esp_schedule_nvs_partition, ESP_SCHEDULE_NVS_NAMESPACE, NVS_TYPE_BLOB, &nvs_iterator);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No entry found in NVS");
        return NULL;;
    }
    while (err == ESP_OK) {
        nvs_entry_info(nvs_iterator, &nvs_entry);
        ESP_LOGI(TAG, "Found schedule in NVS with key: %s", nvs_entry.key);
        handle_list[handle_count] = esp_schedule_nvs_get(nvs_entry.key);
        if (handle_list[handle_count] != NULL) {
            /* Increase count only if nvs_get was successful */
            handle_count++;
        }
        err = nvs_entry_next(&nvs_iterator);
    }
    nvs_release_iterator(nvs_iterator);
#else
    nvs_iterator_t nvs_iterator = nvs_entry_find(esp_schedule_nvs_partition, ESP_SCHEDULE_NVS_NAMESPACE, NVS_TYPE_BLOB);
    if (nvs_iterator == NULL) {
        ESP_LOGE(TAG, "No entry found in NVS");
        return NULL;;
    }
    while (nvs_iterator != NULL) {
        nvs_entry_info(nvs_iterator, &nvs_entry);
        ESP_LOGI(TAG, "Found schedule in NVS with key: %s", nvs_entry.key);
        handle_list[handle_count] = esp_schedule_nvs_get(nvs_entry.key);
        if (handle_list[handle_count] != NULL) {
            /* Increase count only if nvs_get was successful */
            handle_count++;
        }
        nvs_iterator = nvs_entry_next(nvs_iterator);
    }
#endif
    *schedule_count = handle_count;
    ESP_LOGI(TAG, "Found %d schedules in NVS", *schedule_count);
    return handle_list;
}

bool esp_schedule_nvs_is_enabled(void)
{
    return nvs_enabled;
}

esp_err_t esp_schedule_nvs_init(char *nvs_partition)
{
    if (nvs_enabled) {
        ESP_LOGI(TAG, "NVS already enabled");
        return ESP_OK;
    }
    if (nvs_partition) {
        esp_schedule_nvs_partition = strndup(nvs_partition, strlen(nvs_partition));
    } else {
        esp_schedule_nvs_partition = strndup("nvs", strlen("nvs"));
    }
    if (esp_schedule_nvs_partition == NULL) {
        ESP_LOGE(TAG, "Could not allocate nvs_partition");
        return ESP_ERR_NO_MEM;
    }
    nvs_enabled = true;
    return ESP_OK;
}
