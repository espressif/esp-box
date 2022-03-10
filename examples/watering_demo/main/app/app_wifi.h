/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once
#include <esp_err.h>
#define CONFIG_APP_WIFI_PROV_TRANSPORT_BLE 1

#ifdef __cplusplus
extern "C" {
#endif

/** Types of Proof of Possession */
typedef enum {
    /** Use MAC address to generate PoP */
    POP_TYPE_MAC,
    /** Use random stream generated and stored in fctry partition during claiming process as PoP */
    POP_TYPE_RANDOM
} app_wifi_pop_type_t;

/**
 * @brief
 *
 */
void app_wifi_init();

/**
 * @brief
 *
 * @param pop_type
 * @return esp_err_t
 */
esp_err_t app_wifi_start(app_wifi_pop_type_t pop_type);

/**
 * @brief
 *
 * @return char*
 */
char *app_wifi_get_prov_payload(void);

/**
 * @brief
 *
 * @return true
 * @return false
 */
bool app_wifi_is_connected(void);

#ifdef __cplusplus
}
#endif
