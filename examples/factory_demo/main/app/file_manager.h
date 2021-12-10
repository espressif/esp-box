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

#ifndef _IOT_FILE_MANAGER_H_
#define _IOT_FILE_MANAGER_H_

#include "esp_err.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t fm_init(const char *root_path);
void fm_print_dir(const char *direntName, int level);
const char *fm_get_rootpath(void);
const char *fm_get_filename(const char *file);
size_t fm_get_file_size(const char *filepath);
esp_err_t fm_file_table_create(char ***list_out, uint16_t *files_number, const char *filter_suffix);
esp_err_t fm_file_table_free(char ***list,uint16_t files_number);
int fm_mkdir(const char *path);

#ifdef __cplusplus
}
#endif

#endif

