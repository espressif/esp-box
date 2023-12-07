/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once

#include "esp_log.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define USB_HID_TAG "USB-HID"
#define HID_ITF_PROTOCOL_GAMEPAD 1
#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

esp_err_t usb_hid_init(void);
void usb_hid_send_joystick_value(uint8_t usb_hid_id, int8_t rocker_x1, int8_t rocker_y1, int8_t rocker_x2, int8_t rocker_y2, uint32_t buttons);

#ifdef __cplusplus
}
#endif
