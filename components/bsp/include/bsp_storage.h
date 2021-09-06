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

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BSP_STORAGE_NONE = 0,
    BSP_STORAGE_SD_CARD,
    BSP_STORAGE_SPIFFS,
    BSP_STORAGE_SEMIHOST,
} bsp_storage_dev_t;

/**
 * @brief Init given storage device with default config
 * 
 * @param dev Storage device. See `bsp_storage_dev_t`.
 * @return 
 *    - ESP_OK: Success
 *    - ESP_ERR_NOT_SUPPORTED: Unsupported storage device
 *    - Others: Fail
 */
esp_err_t bsp_storage_init_default(bsp_storage_dev_t dev);

/**
 * @brief Init given storage device
 * 
 * @param dev Storage device. See `bsp_storage_dev_t`.
 * @param conf Config string list
 * @return
 *    - ESP_OK: Success
 *    - ESP_ERR_NOT_SUPPORTED: Unsupported storage device
 *    - Others: Fail
 */
esp_err_t bsp_storage_init(bsp_storage_dev_t dev, void *conf);

/**
 * @brief Deinit given storage device initialized by default config
 * 
 * @param dev Storage device. See `bsp_storage_dev_t`.
 * @return 
 *    - ESP_OK: Success
 *    - ESP_ERR_NOT_SUPPORTED: Unsupported storage device
 *    - Others: Fail
 */
esp_err_t bsp_storage_deinit_default(bsp_storage_dev_t dev);

/**
 * @brief Get mount point of given storage device
 * 
 * @param dev Storage device. See `bsp_storage_dev_t`.
 * @param p_mont_point Poniter to string pointer
 * @return 
 *    - ESP_OK: Success
 *    - ESP_ERR_NOT_SUPPORTED: Unsupported storage device
 *    - Others: Fail
 */
esp_err_t bsp_storage_get_mount_point(bsp_storage_dev_t dev, char **p_mont_point);

#ifdef __cplusplus
}
#endif
