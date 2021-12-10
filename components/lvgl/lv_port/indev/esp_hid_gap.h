/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sdkconfig.h"

#if CONFIG_BT_ENABLED && CONFIG_BT_BLE_42_FEATURES_SUPPORTED

#include "esp_err.h"
#include "esp_log.h"

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gap_bt_api.h"
#include "esp_hid_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct esp_hidh_scan_result_s {
    struct esp_hidh_scan_result_s *next;

    esp_bd_addr_t bda;
    const char *name;
    int8_t rssi;
    esp_hid_usage_t usage;
    esp_hid_transport_t transport; //BT, BLE or USB
    union {
        struct {
            esp_bt_cod_t cod;
            esp_bt_uuid_t uuid;
        } bt;
        struct {
            esp_ble_addr_type_t addr_type;
            uint16_t appearance;
        } ble;
    };
} esp_hid_scan_result_t;

/**
 * @brief Init HID GAP
 * 
 * @param mode See `esp_bt_mode_t`
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t esp_hid_gap_init(uint8_t mode);

/**
 * @brief 
 * 
 * @param seconds Timeout in seconds
 * @param num_results Total scaned result
 * @param results Pointer to results
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t esp_hid_scan(uint32_t seconds, size_t *num_results, esp_hid_scan_result_t **results);

/**
 * @brief Free HID scan result
 * 
 * @param results Pointer to `esp_hid_scan_result_t`
 */
void esp_hid_scan_results_free(esp_hid_scan_result_t *results);

/**
 * @brief Print UUID
 * 
 * @param uuid UUID of device
 */
void print_uuid(esp_bt_uuid_t *uuid);

/**
 * @brief Convert BLE addr type value to string
 * 
 * @param ble_addr_type See `ble_addr_type`
 * @return String of BLE addr type
 */
const char *ble_addr_type_str(esp_ble_addr_type_t ble_addr_type);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_BT_BLE_42_FEATURES_SUPPORTED */
