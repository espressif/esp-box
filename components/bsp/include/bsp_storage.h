/**
 * @file bsp_storage.h
 * @brief 
 * @version 0.1
 * @date 2021-07-06
 * 
 * @copyright Copyright 2021 Espressif Systems (Shanghai) Co. Ltd.
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *               http://www.apache.org/licenses/LICENSE-2.0

 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
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

/**
 * @brief Init SPIFFS
 * 
 * @param partition_label Label of the partition which should be used
 * @param mount_point Path where partition should be registered (e.g. "/sdcard")
 * @param max_files Maximum number of files which can be open at the same time
 * @return
 *    - ESP_OK                  Success
 *    - ESP_ERR_NO_MEM          Objects could not be allocated
 *    - ESP_ERR_INVALID_STATE   Already mounted or partition is encrypted
 *    - ESP_ERR_NOT_FOUND       Partition for SPIFFS was not found
 *    - ESP_FAIL                Mount or format fails
 *    - Others                  Fail
 */
esp_err_t bsp_spiffs_init(char *partition_label, char *mount_point, size_t max_files);

/**
 * @brief Init SPIFFS with default config
 * 
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_spiffs_init_default(void);

/**
 * @brief Deinit SPIFFS
 * 
 * @param partition_label Label of the partition which was used
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_spiffs_deinit(char *partition_label);

/**
 * @brief Deinit SPIFFS initialized by `bsp_spiffs_init_default`
 * 
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_spiffs_deinit_default(void);

#ifdef __cplusplus
}
#endif
