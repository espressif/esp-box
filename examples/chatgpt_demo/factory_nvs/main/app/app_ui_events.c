/* SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_system.h"
#include "esp_log.h"
#include "lvgl.h"

#if (ESP_IDF_VERSION_MAJOR == 5) && (ESP_IDF_VERSION_MINOR == 3)
#include "hal/usb_fsls_phy_ll.h"
#else
#include "hal/usb_phy_ll.h"
#endif

static char *TAG = "NVS: ui-events";

void EventBtnSetupClick(lv_event_t *e)
{
    ESP_LOGI(TAG, "btn click!");
    // Configure USB PHY, Change back to USB-Serial-Jtag

#if (ESP_IDF_VERSION_MAJOR == 5) && (ESP_IDF_VERSION_MINOR == 3)
    usb_fsls_phy_ll_int_jtag_enable(&USB_SERIAL_JTAG);
#else
    usb_phy_ll_int_jtag_enable(&USB_SERIAL_JTAG);
#endif
    esp_restart();
}
