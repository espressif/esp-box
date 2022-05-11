/**
 * @file
 * @version 0.1
 * @date 2022-11-10
 *
 * @copyright Copyright 2021 Espressif Systems (Shanghai) Co. Ltd.
 * @copyright Copyright 2022 Chris Morgan <chmorgan@gmail.com>
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include <dirent.h>
#include <string.h>
#include "esp_check.h"
#include "esp_log.h"
#include "playlist.h"

static const char *TAG = "playlist";

static size_t audio_count;
static size_t audio_index;
static char **audio_list;
static const char *directory_path;

static esp_err_t playlist_file_scan(const char *base_path)
{
    audio_count = 0;
    struct dirent *p_dirent = NULL;
    DIR *p_dir_stream = opendir(base_path);

    do {    /* Get total file count */
        p_dirent = readdir(p_dir_stream);
        if (NULL != p_dirent) {
            audio_count++;
        } else {
            closedir(p_dir_stream);
            break;
        }
    } while (1);

    audio_list = (char **) malloc(audio_count * sizeof(char *));
    ESP_RETURN_ON_FALSE(NULL != audio_list, ESP_ERR_NO_MEM,
        TAG, "Failed allocate audio list buffer");

    p_dir_stream = opendir(base_path);
    for (size_t i = 0; i < audio_count; i++) {
        p_dirent = readdir(p_dir_stream);
        if (NULL != p_dirent) {
            audio_list[i] = malloc(sizeof(p_dirent->d_name));
            ESP_LOGI(TAG, "File : %s", strcpy(audio_list[i], p_dirent->d_name));
        } else {
            ESP_LOGE(TAG, "The file system may be corrupted");
            closedir(p_dir_stream);
            for (int j = i - 1; j >= 0; j--) {
                free(audio_list[i]);
            }
            free(audio_list);
            return ESP_ERR_INVALID_STATE;
        }
    }

    closedir(p_dir_stream);
    return ESP_OK;
}

size_t playlist_get_count() {
    return audio_count;
}

size_t playlist_get_index(void)
{
    ESP_LOGI(TAG, "playlist_get_index() %d", audio_index);
    return audio_index;
}

void playlist_set_index(size_t index)
{
    ESP_LOGI(TAG, "playlist_set_index() %d", index);

    if(index >= audio_count) {
        index = 0;
    }

    audio_index = index;
}

const char* playlist_get_name_from_index(size_t index)
{
    ESP_RETURN_ON_FALSE(index < audio_count, NULL,
        TAG, "File index out of range");

    ESP_RETURN_ON_FALSE(NULL != audio_list, NULL,
        TAG, "Audio file not found");

    ESP_RETURN_ON_FALSE(NULL != audio_list[index], NULL,
        TAG, "Audio file not found");

    return audio_list[index];
}

int playlist_get_full_path_from_index(size_t index, char* path, size_t path_len)
{
    const char* name = playlist_get_name_from_index(index);
    if(!name) {
        return 0;
    }

    int retval = snprintf(path, path_len, "%s/%s", directory_path, name);

    return retval;
}

esp_err_t playlist_next(void)
{
    audio_index++;
    if (audio_index >= audio_count) {
        audio_index = 0;
    }
    return ESP_OK;
}

esp_err_t playlist_prev(void)
{
    if (audio_index == 0) {
        audio_index = audio_count;
    }
    audio_index--;
    return ESP_OK;
}

esp_err_t playlist_init(const char *base_path)
{
    /* Scan audio file */
    if (NULL != base_path) {
        playlist_file_scan(base_path);
    } else {
        ESP_LOGE(TAG, "Invalid base path");
    }

    directory_path = strdup(base_path);

    return ESP_OK;
}
