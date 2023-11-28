/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

#include "OpenAI.h"

#define SSID_SIZE 32
#define PASSWORD_SIZE 64
#define KEY_SIZE 64
#define URL_SIZE 64

typedef struct {
    char ssid[SSID_SIZE];             /* SSID of target AP. */
    char password[PASSWORD_SIZE];     /* Password of target AP. */
    char key[KEY_SIZE];               /* OpenAI key. */
    char url[URL_SIZE];               /* OpenAI Base url. */
} sys_param_t;

esp_err_t settings_factory_reset(void);
esp_err_t settings_read_parameter_from_nvs(void);
sys_param_t *settings_get_parameter(void);
