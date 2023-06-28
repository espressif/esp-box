/* SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_system.h"
#include "esp_log.h"
#include "hal/usb_phy_ll.h"
#include "lvgl.h"

static char *TAG = "NVS: ui-events";

void EventBtnSetupClick(lv_event_t * e)
{
    ESP_LOGI(TAG, "btn click!");
    // Configure USB PHY, Change back to USB-Serial-Jtag
    usb_phy_ll_int_jtag_enable(&USB_SERIAL_JTAG); 
    esp_restart();
}
