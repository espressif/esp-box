/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

#include <esp_err.h>
#include <esp_matter.h>

typedef void *app_driver_handle_t;
#ifdef __cplusplus
extern "C" {
#endif

/** Initialize the board and the drivers
 *
 * This initializes the selected board, which then initializes the respective drivers associated with it.
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t app_driver_init(void);

/** Initialize the switch driver
 *
 * This initializes the switch driver associated with the selected board.
 *
 * @return Handle on success.
 * @return NULL in case of failure.
 */
app_driver_handle_t matter_app_driver_switch_init();
esp_err_t app_driver_attribute_update(uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id,
                                      esp_matter_attr_val_t *val);

void start_box(void);

#ifdef __cplusplus
}
#endif
