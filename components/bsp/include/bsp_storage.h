/**
 * @file bsp_storage.h
 * @brief 
 * @version 0.1
 * @date 2021-07-06
 * 
 * @copyright Copyright (c) 2021
 * 
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

esp_err_t bsp_storage_init(bsp_storage_dev_t dev);

esp_err_t bsp_storage_deinit(bsp_storage_dev_t dev);

esp_err_t bsp_storage_get_mount_point(bsp_storage_dev_t dev, char **p_mont_point);

#ifdef __cplusplus
}
#endif
