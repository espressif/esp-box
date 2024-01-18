/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "ui/ui.h"
#include "ui_events.h"
#include "lvgl.h"
#include "bsp/esp-box-3.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "espnow.h"
#include "espnow_ctrl.h"
#include "espnow_utils.h"
#include "app_rocker.h"
#include "app_usb_hid.h"
#include "app_ble_hid.h"
#include "app_button.h"
#include "app_nvs_flash.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JOYSTICK_TAG        "JoyStick"
#define GAME_PAD_APP_TAG    "GAME-PAD APP"
#define RC_APP_TAG          "Remote-Controller APP"

#define GAME_ROCKET_RANGE     125.0
#define RC_ROCKET_RANGE       90.0
typedef enum {
    USB_HID_INIT_STATE = BIT(0),
    ROCKER_ADC_INIT_STATE = BIT(1),
    BLE_HID_INIT_STATE = BIT(2),
} init_state_type_t;

typedef enum {
    GAMEPAD_APP_TASK_STATE = BIT(0),
    RC_APP_TASK_STATE = BIT(1),
} app_task_state_type_t;

esp_err_t event_state_group_init(void);
uint8_t get_vibration_feedback_state(void);

#ifdef __cplusplus
}
#endif
