/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Init SD crad
 *
 * @param mount_point Path where partition should be registered (e.g. "/sdcard")
 * @param max_files Maximum number of files which can be open at the same time
 * @return
 *    - ESP_OK                  Success
 *    - ESP_ERR_INVALID_STATE   If esp_vfs_fat_register was already called
 *    - ESP_ERR_NOT_SUPPORTED   If dev board not has SDMMC/SDSPI
 *    - ESP_ERR_NO_MEM          If not enough memory or too many VFSes already registered
 *    - Others                  Fail
 */
esp_err_t bsp_sdcard_init(char *mount_point, size_t max_files);

/**
 * @brief Init SD crad with default config
 *
 * @return
 *    - ESP_OK                  Success
 *    - ESP_ERR_INVALID_STATE   If esp_vfs_fat_register was already called
 *    - ESP_ERR_NOT_SUPPORTED   If dev board not has SDMMC/SDSPI
 *    - ESP_ERR_NO_MEM          If not enough memory or too many VFSes already registered
 *    - Others                  Fail
 */
esp_err_t bsp_sdcard_init_default(void);

/**
 * @brief Deinit SD card
 *
 * @param mount_point Path where partition was registered (e.g. "/sdcard")
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_sdcard_deinit(char *mount_point);

/**
 * @brief Deinit SD card initialized by `bsp_sdcard_init_default`
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_sdcard_deinit_default(void);

#ifdef __cplusplus
}
#endif
