/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_bt_defs.h"
#if CONFIG_BT_BLE_ENABLED
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#endif
#include "esp_bt_main.h"
#include "esp_bt_device.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_HID_TAG "BLE-HID"
#define HID_RPT_ID_CC_GP_IN        3   // Consumer Control input report ID gamepad
#define HID_CC_IN_RPT_GP_LEN       6   // Consumer Control input report Len gamepad

esp_err_t ble_hid_init(void);
void ble_hid_send_joystick_value(uint16_t joystick_buttons, uint8_t joystick_x, uint8_t joystick_y, uint8_t joystick_z, uint8_t joystick_rx);

#ifdef __cplusplus
}
#endif
