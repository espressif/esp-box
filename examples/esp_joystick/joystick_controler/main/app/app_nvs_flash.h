/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once

#include "esp_flash.h"
#include "nvs.h"
#include "nvs_flash.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NVS_PART_NAME      "nvs"
#define NVS_PART_NAMESPACE "test_result"

void flash_write_init(void);
void flash_write_state(char *key, char *value);
uint16_t read_rocker_value_from_flash(char *key);

#ifdef __cplusplus
}
#endif
