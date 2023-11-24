/*
 * SPDX-FileCopyrightText: 2015-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

#include <stdbool.h>
#include <sys/stat.h>
/* FreeRTOS includes */
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

/* ESP32 includes */
#include "esp_err.h"
#include "esp_check.h"

#include "driver/gpio.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "unity.h"

#include "driver/rmt_tx.h"
#include "driver/rmt_rx.h"

/* IR learn includes */
#include "ir_learn.h"
#include "ir_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_RADAR,
    UI_AIR_SWITCH,
} ui_sensor_monitor_img_type_t;

typedef enum {
    SENSOR_MONITOR_ALIVE_STATE = BIT(0),
    AIR_SWITCH_REVERSE_STATE = BIT(1),
    AIR_POWER_STATE = BIT(2),
    RADAR_SWITCH_STATE = BIT(3),
    RADAR_STATE = BIT(4),
    IR_LEARNING_STATE = BIT(5),
    SENSOR_BASE_CONNECT_STATE = BIT(6),

    NEED_DELETE = BIT(7),
    TX_CH_DELETED = BIT(8),
} sensor_task_state_type_t;

esp_err_t sensor_task_state_event_init(void);
EventBits_t sensor_task_state_event_get_bits(void);
void ui_sensor_monitor_start(void (*fn)(void));
bool sensor_ir_learn_enable(void);

/**
 * @brief Send AC poweron cmd, must enter ui_sensor_monitor page
 *
 * @return
 *      - ESP_OK                On success
 *      - ESP_ERR_NOT_SUPPORTED This function is not supported.
 */
esp_err_t ui_sensor_set_ac_poweron(void);

/**
 * @brief Send AC poweroff cmd, must enter ui_sensor_monitor page
 *
 * @return
 *      - ESP_OK                On success
 *      - ESP_ERR_NOT_SUPPORTED This function is not supported.
 */
esp_err_t ui_sensor_set_ac_poweroff(void);

#ifdef __cplusplus
}
#endif
