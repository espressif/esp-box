/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

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
esp_err_t fm_file_table_free(char ***list, uint16_t files_number);
int fm_mkdir(const char *path);

#ifdef __cplusplus
}
#endif

#endif
