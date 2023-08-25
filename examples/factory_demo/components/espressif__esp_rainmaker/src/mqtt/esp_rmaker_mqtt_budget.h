
/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>
#include <esp_err.h>

esp_err_t esp_rmaker_mqtt_budgeting_init(void);
esp_err_t esp_rmaker_mqtt_budgeting_deinit(void);
esp_err_t esp_rmaker_mqtt_budgeting_stop(void);
esp_err_t esp_rmaker_mqtt_budgeting_start(void);
esp_err_t esp_rmaker_mqtt_increase_budget(uint8_t budget);
esp_err_t esp_rmaker_mqtt_decrease_budget(uint8_t budget);
