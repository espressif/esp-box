/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdkconfig.h"

#if CONFIG_BT_BLE_42_FEATURES_SUPPORTED

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsp_board.h"
#include "bsp_hid_indev.h"
#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_hidh.h"
#include "esp_hid_gap.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "hid_mouse";
static void *bsp_mouse_handle = NULL;
static indev_hid_state_t mouse_state = { .x = 0, .y = 0, .press = false };

static void bsp_mouse_default_task(void *pvParam);
static void hid_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data);

esp_err_t indev_hid_init(indev_hid_conf_t *conf)
{
    ESP_RETURN_ON_FALSE(NULL == bsp_mouse_handle, ESP_ERR_INVALID_STATE,
        TAG, "Mouse device already initialized");

    bsp_mouse_handle = malloc(sizeof(indev_hid_conf_t));

    ESP_RETURN_ON_FALSE(NULL != bsp_mouse_handle, ESP_ERR_NO_MEM,
        TAG, "Failed create mouse handle");

    memcpy(bsp_mouse_handle, conf, sizeof(indev_hid_conf_t));

    if (conf->init_ble_service) {
        /* Initialize BLE */
        ESP_ERROR_CHECK(esp_hid_gap_init(ESP_BT_MODE_BLE));
        ESP_ERROR_CHECK(esp_ble_gattc_register_callback(esp_hidh_gattc_event_handler));
    }

    esp_hidh_config_t config = {
        .callback = hid_callback,
        .event_stack_size = 4096,
    };
    ESP_ERROR_CHECK(esp_hidh_init(&config));

    /* Mouse device default task */
    BaseType_t ret_val = xTaskCreatePinnedToCore(
        (TaskFunction_t)        bsp_mouse_default_task,
        (const char * const)    "Mouse Task",
        (const uint32_t)        6 * 1024,
        (void * const)          NULL,
        (UBaseType_t)           1,
        (TaskHandle_t * const)  NULL,
        (const BaseType_t)      0);
    if (pdPASS != ret_val) {
        ESP_LOGE(TAG, "Failed create mouse task");
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

esp_err_t indev_hid_init_default(void)
{
    indev_hid_conf_t conf = {
        .init_ble_service = true,
    };
    return bsp_hid_indev_init(&conf);
}

esp_err_t indev_hid_get_value(indev_hid_state_t *state)
{
    if (NULL == bsp_mouse_handle) {
        return ESP_ERR_INVALID_STATE;
    }

    memcpy(state, &mouse_state, sizeof(indev_hid_state_t));

    return ESP_OK;
}

/* **************** TASK, CALLBACK and EVENT HANDLER **************** */
static esp_err_t bsp_mouse_event_handler(uint8_t *data, uint16_t length, esp_hid_usage_t usage)
{
    ESP_RETURN_ON_FALSE(ESP_HID_USAGE_MOUSE == usage, ESP_ERR_INVALID_ARG,
        TAG, "Invalid usage : %s (%d)", esp_hid_usage_str(usage), usage);

    ESP_RETURN_ON_FALSE(length >= 7, ESP_ERR_INVALID_SIZE,
        TAG, "invalid data length : %u", length);

    int dx = * ((int16_t *) &data[1]);
    int dy = * ((int16_t *) &data[3]);
    int dz = * ((int8_t *)  &data[5]);

    mouse_state.x += dx;
    if (mouse_state.x < 0)              mouse_state.x = 0;
    if (mouse_state.x >= LCD_WIDTH)     mouse_state.x = LCD_WIDTH - 1;

    mouse_state.y += dy;
    if (mouse_state.y >= LCD_HEIGHT)    mouse_state.y = LCD_HEIGHT - 1;
    if (mouse_state.y < 0)              mouse_state.y = 0;

    mouse_state.press = 0 != data[0];

    ESP_LOGD(TAG, "Button : 0x%02X. Pos info : [%3d, %3d]{%3d, %3d}. Scroll : %d", data[0], mouse_state.x, mouse_state.y, dx, dy, dz);
    return ESP_OK;
}

static void hid_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    esp_hidh_event_t event = (esp_hidh_event_t) id;
    esp_hidh_event_data_t *param = (esp_hidh_event_data_t *) event_data;

    switch (event) {
    case ESP_HIDH_OPEN_EVENT: {
        const uint8_t *bda = esp_hidh_dev_bda_get(param->open.dev);
        ESP_LOGI(TAG, ESP_BD_ADDR_STR " OPEN: %s", ESP_BD_ADDR_HEX(bda), esp_hidh_dev_name_get(param->open.dev));
        esp_hidh_dev_dump(param->open.dev, stdout);
        break;
    }
    case ESP_HIDH_BATTERY_EVENT: {
        const uint8_t *bda = esp_hidh_dev_bda_get(param->battery.dev);
        ESP_LOGI(TAG, ESP_BD_ADDR_STR " BATTERY: %d%%", ESP_BD_ADDR_HEX(bda), param->battery.level);
        break;
    }
    case ESP_HIDH_INPUT_EVENT: {
        // const uint8_t *bda = esp_hidh_dev_bda_get(param->input.dev);
        bsp_mouse_event_handler(param->input.data, param->input.length, param->input.usage);
        break;
    }
    case ESP_HIDH_FEATURE_EVENT: {
        const uint8_t *bda = esp_hidh_dev_bda_get(param->feature.dev);
        ESP_LOGI(TAG, ESP_BD_ADDR_STR " FEATURE: %8s, MAP: %2u, ID: %3u, Len: %d", ESP_BD_ADDR_HEX(bda), esp_hid_usage_str(param->feature.usage), param->feature.map_index, param->feature.report_id, param->feature.length);
        ESP_LOG_BUFFER_HEX(TAG, param->feature.data, param->feature.length);
        break;
    }
    case ESP_HIDH_CLOSE_EVENT: {
        const uint8_t *bda = esp_hidh_dev_bda_get(param->close.dev);
        ESP_LOGI(TAG, ESP_BD_ADDR_STR " CLOSE: '%s' %s", ESP_BD_ADDR_HEX(bda), esp_hidh_dev_name_get(param->close.dev), esp_hid_disconnect_reason_str(esp_hidh_dev_transport_get(param->close.dev), param->close.reason));
        esp_hidh_dev_free(param->close.dev);
        break;
    }
    default:
        ESP_LOGI(TAG, "EVENT: %d", event);
        break;
    }
}

static void bsp_mouse_default_task(void *pvParam)
{
    (void) pvParam;

    size_t results_len = 0;
    const uint32_t scan_duration_seconds = 5;
    esp_hid_scan_result_t *results = NULL;

    ESP_LOGI(TAG, "Scaning for HID devices...");

    while (true) {
        esp_hid_scan(scan_duration_seconds, &results_len, &results);
        ESP_LOGI(TAG, "Scan complete. %u devices found", results_len);

        if (results_len > 0) {
            esp_hid_scan_result_t *device = results;
            esp_hidh_dev_t *opened_dev = NULL;

            /* Print device info and open avaliable device(s) */
            while (device) {
                if (device->transport == ESP_HID_TRANSPORT_BLE) {
                    printf("\n");
                    printf("Addr : " ESP_BD_ADDR_STR ", ", ESP_BD_ADDR_HEX(device->bda));
                    printf("Name : %s, ", device->name ? device->name : "");
                    printf("Usage: %s, ", esp_hid_usage_str(device->usage));
                    printf("RSSI : %d, ", device->rssi);
                    printf("APPEARANCE: 0x%04x, ", device->ble.appearance);
                    printf("ADDR_TYPE: '%s'\n", ble_addr_type_str(device->ble.addr_type));
                    opened_dev = esp_hidh_dev_open(device->bda, device->transport, device->ble.addr_type);
                }
                device = device->next;

                if (NULL != opened_dev) {
                    goto task_over;
                }
            }

            printf("\n");

            /* Free scan results */
            esp_hid_scan_results_free(results);
        }

    }

task_over:
    /* Task never returns */
    vTaskDelete(NULL);
}

#else

#include "esp_err.h"
#include "esp_log.h"
#include "indev_hid.h"

static const char *TAG = "hid_indev";

esp_err_t indev_hid_init(indev_hid_conf_t *conf)
{
    (void) conf;

    ESP_LOGE(TAG, "Please turn on \"Enable BLE 4.2 features\" option in menuconfig");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t indev_hid_init_default(void)
{
    ESP_LOGE(TAG, "Please turn on \"Enable BLE 4.2 features\" option in menuconfig");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t indev_hid_get_value(indev_hid_state_t *state)
{
    ESP_LOGE(TAG, "Please turn on \"Enable BLE 4.2 features\" option in menuconfig");
    return ESP_ERR_NOT_SUPPORTED;
}

#endif
