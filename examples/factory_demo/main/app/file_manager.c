// Copyright 2015-2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "file_manager.h"

static const char *TAG = "file manager";

#define FLN_MAX 127

static char g_root_path[FLN_MAX];

static void TraverseDir(const char *direntName, int level, int indent)
{
    DIR *p_dir = NULL;
    struct dirent *p_dirent = NULL;

    p_dir = opendir(direntName);

    if (p_dir == NULL) {
        printf("opendir error\n");
        return;
    }

    while ((p_dirent = readdir(p_dir)) != NULL) {
        char *backupDirName = NULL;

        if (p_dirent->d_name[0] == '.') {
            continue;
        }

        int i;

        for (i = 0; i < indent; i++) {
            // printf("|");
            printf("     ");
        }

        printf("|--- %s", p_dirent->d_name);

        /* Itme is a file */
        if (p_dirent->d_type == DT_REG) {
            int curDirentNameLen = strlen(direntName) + strlen(p_dirent->d_name) + 2;

            //prepare next path
            backupDirName = (char *)malloc(curDirentNameLen);
            struct stat *st = NULL;
            st = malloc(sizeof(struct stat));

            if (NULL == backupDirName || NULL == st) {
                goto _err;
            }

            memset(backupDirName, 0, curDirentNameLen);
            memcpy(backupDirName, direntName, strlen(direntName));

            strcat(backupDirName, "/");
            strcat(backupDirName, p_dirent->d_name);

            int statok = stat(backupDirName, st);

            if (0 == statok) {
                printf("[%dB]\n", (int)st->st_size);
            } else {
                printf("\n");
            }

            free(backupDirName);
            backupDirName = NULL;
        } else {
            printf("\n");
        }

        /* Itme is a directory */
        if (p_dirent->d_type == DT_DIR) {
            int curDirentNameLen = strlen(direntName) + strlen(p_dirent->d_name) + 2;

            //prepare next path
            backupDirName = (char *)malloc(curDirentNameLen);

            if (NULL == backupDirName) {
                goto _err;
            }

            memset(backupDirName, 0, curDirentNameLen);
            memcpy(backupDirName, direntName, curDirentNameLen);

            strcat(backupDirName, "/");
            strcat(backupDirName, p_dirent->d_name);

            if (level > 0) {
                TraverseDir(backupDirName, level - 1, indent + 1);
            }

            free(backupDirName);
            backupDirName = NULL;
        }
    }

_err:
    closedir(p_dir);
}

void fm_print_dir(const char *direntName, int level)
{
    printf("Traverse directory %s\n", direntName);
    TraverseDir(direntName, level, 0);
    printf("\r\n");
}

esp_err_t fm_init(const char *root_path)
{
    strcpy(g_root_path, root_path);
    return ESP_OK;
}

const char *fm_get_rootpath(void)
{
    return g_root_path;
}

esp_err_t fm_file_table_create(char ***list_out, uint16_t *files_number, const char *filter_suffix)
{
    DIR *p_dir = NULL;
    struct dirent *p_dirent = NULL;

    p_dir = opendir(g_root_path);

    if (p_dir == NULL) {
        ESP_LOGE(TAG, "opendir error");
        return ESP_FAIL;
    }

    uint16_t f_num = 0;
    while ((p_dirent = readdir(p_dir)) != NULL) {
        if (p_dirent->d_type == DT_REG) {
            f_num++;
        }
    }

    rewinddir(p_dir);

    *list_out = calloc(f_num, sizeof(char *));
    if (NULL == (*list_out)) {
        goto _err;
    }
    for (size_t i = 0; i < f_num; i++) {
        (*list_out)[i] = malloc(FLN_MAX);
        if (NULL == (*list_out)[i]) {
            ESP_LOGE(TAG, "malloc failed at %d", i);
            fm_file_table_free(list_out, f_num);
            goto _err;
        }
    }

    uint16_t index = 0;
    while ((p_dirent = readdir(p_dir)) != NULL) {
        if (p_dirent->d_type == DT_REG) {
            if (NULL != filter_suffix) {
                if (strstr(p_dirent->d_name, filter_suffix)) {
                    strncpy((*list_out)[index], p_dirent->d_name, FLN_MAX - 1);
                    (*list_out)[index][FLN_MAX - 1] = '\0';
                    index++;
                }
            } else {
                strncpy((*list_out)[index], p_dirent->d_name, FLN_MAX - 1);
                (*list_out)[index][FLN_MAX - 1] = '\0';
                index++;
            }
        }
    }
    (*files_number) = index;

    closedir(p_dir);
    return ESP_OK;
_err:
    closedir(p_dir);

    return ESP_FAIL;
}

esp_err_t fm_file_table_free(char ***list, uint16_t files_number)
{
    for (size_t i = 0; i < files_number; i++) {
        free((*list)[i]);
    }
    free((*list));
    return ESP_OK;
}

const char *fm_get_filename(const char *file)
{
    const char *p = file + strlen(file);
    while (p > file) {
        if ('/' == *p) {
            return (p + 1);
        }
        p--;
    }
    return file;
}

size_t fm_get_file_size(const char *filepath)
{
    struct stat siz =  { 0 };
    stat(filepath, &siz);
    return siz.st_size;
}

int fm_mkdir(const char *path)
{
  struct stat st;
  int status = 0;

  if (stat(path, &st) != 0) {
    /* Directory does not exist. EEXIST for race condition */
    ESP_LOGI(TAG, "Create dir [%s]", path);
    if (mkdir(path, 0755) != 0 && errno != EEXIST) {
      status = -1;
      ESP_LOGE(TAG, "Create dir [%s] failed", path);
    }
  } else if (!S_ISDIR(st.st_mode)) {
    errno = ENOTDIR;
    status = -1;
    ESP_LOGE(TAG, "Exist [%s] but not dir", path);
  }

  return status;
}
