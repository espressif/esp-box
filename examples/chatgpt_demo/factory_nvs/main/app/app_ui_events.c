/* SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_system.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_tinyuf2.h"

static char *TAG = "NVS: ui-events";

void EventBtnSetupClick(lv_event_t *e)
{
    ESP_LOGI(TAG, "btn click!");
    // Configure USB PHY, Change back to USB-Serial-Jtag

    esp_tinyuf2_uninstall();
    esp_restart();
}
