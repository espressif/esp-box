/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include "app_ble_hid.h"
#include "esp_hid_gap.h"
#include "esp_hidd.h"
#include "app_button.h"

typedef struct {
    TaskHandle_t task_hdl;
    esp_hidd_dev_t *hid_dev;
    uint8_t protocol_mode;
    uint8_t *buffer;
} local_param_t;

static local_param_t s_ble_hid_param = {0};

const unsigned char joystickReportMap[] = {
    /* 76 bytes */
    0x05, 0x01,  /* Usage Page (Generic Desktop) */
    0x09, 0x05,  /* Usage (Gamepad) */
    0xA1, 0x01,  /* Collection (Application) */
    0x85, 0x03,  /* Report Id (3) */
    0xA1, 0x00,  /* Collection (Physical) */

    0x05, 0x09,  /* Usage Page (Buttons) */
    0x19, 0x01,  /* Usage Minimum (01) - Button 1  */
    0x29, 0x10,  /* Usage Maximum (16) - Button 16 */
    0x15, 0x00,  /* Logical Minimum (0) */
    0x25, 0x01,  /* Logical Maximum (1) */
    0x95, 0x10,  /* Report Count (16)   */
    0x75, 0x01,  /* Report Size (1)     */
    0x81, 0x02,  /* Input (Data, Variable, Absolute) - Button states */
    0x05, 0x01,  /* Usage Page (Generic Desktop) */
    0x09, 0x30,  /* Usage (X)  */
    0x09, 0x31,  /* Usage (Y)  */
    0x09, 0x32,  /* Usage (Z)  */
    0x09, 0x33,  /* Usage (Rx) */
    0x15, 0x81,  /* Logical Minimum (-127) */
    0x25, 0x7F,  /* Logical Maximum (127)  */
    0x95, 0x04,  /* Report Count (4) */
    0x75, 0x08,  /* Report Size (8)  */
    0x81, 0x02,  /* Input (Data, Variable, Absolute) - X & Y coordinate */

    0xC0,        /* End Collection */
    0xC0,        /* End Collection */
};
static esp_hid_raw_report_map_t ble_report_maps[] = {
    {
        .data = joystickReportMap,
        .len = sizeof(joystickReportMap)
    }
};

static esp_hid_device_config_t ble_hid_config = {
    .vendor_id          = 0xe502,
    .product_id         = 0xbbab,
    .version            = 0x0100,
    .device_name        = "ESP-BOX-JoyStick",
    .manufacturer_name  = "Espressif",
    .serial_number      = "1234567890",
    .report_maps        = ble_report_maps,
    .report_maps_len    = 1
};

void ble_hid_send_joystick_value(uint16_t joystick_buttons, uint8_t joystick_x, uint8_t joystick_y, uint8_t joystick_z, uint8_t joystick_rx)
{
    uint8_t buffer[HID_CC_IN_RPT_GP_LEN];

    buffer[0] = joystick_buttons & 0xff;
    buffer[1] = (joystick_buttons >> 8);
    buffer[2] = (joystick_x);           /* LX */
    buffer[3] = (joystick_y) - 1;       /* LY */
    buffer[4] = (joystick_z);           /* RX */
    buffer[5] = (joystick_rx) - 1;      /* RY */
    esp_err_t ret = esp_hidd_dev_input_set(s_ble_hid_param.hid_dev, 0, HID_RPT_ID_CC_GP_IN, buffer, HID_CC_IN_RPT_GP_LEN);
    if (ret != ESP_OK) {
        ESP_LOGE(BLE_HID_TAG, "Failed to send key value via BLE-HID.");
    }
    return;
}

static void ble_hidd_event_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    esp_hidd_event_t event = (esp_hidd_event_t)id;
    esp_hidd_event_data_t *param = (esp_hidd_event_data_t *)event_data;

    switch (event) {
    case ESP_HIDD_START_EVENT: {
        ESP_LOGI(BLE_HID_TAG, "START");
        esp_hid_ble_gap_adv_start();
        break;
    }
    case ESP_HIDD_CONNECT_EVENT: {
        ESP_LOGI(BLE_HID_TAG, "CONNECT");
        break;
    }
    case ESP_HIDD_PROTOCOL_MODE_EVENT: {
        ESP_LOGI(BLE_HID_TAG, "PROTOCOL MODE[%u]: %s", param->protocol_mode.map_index, param->protocol_mode.protocol_mode ? "REPORT" : "BOOT");
        break;
    }
    case ESP_HIDD_CONTROL_EVENT: {
        ESP_LOGI(BLE_HID_TAG, "CONTROL[%u]: %sSUSPEND", param->control.map_index, param->control.control ? "EXIT_" : "");
        break;
    }
    case ESP_HIDD_OUTPUT_EVENT: {
        ESP_LOGI(BLE_HID_TAG, "OUTPUT[%u]: %8s ID: %2u, Len: %d, Data:", param->output.map_index, esp_hid_usage_str(param->output.usage), param->output.report_id, param->output.length);
        ESP_LOG_BUFFER_HEX(BLE_HID_TAG, param->output.data, param->output.length);
        break;
    }
    case ESP_HIDD_FEATURE_EVENT: {
        ESP_LOGI(BLE_HID_TAG, "FEATURE[%u]: %8s ID: %2u, Len: %d, Data:", param->feature.map_index, esp_hid_usage_str(param->feature.usage), param->feature.report_id, param->feature.length);
        ESP_LOG_BUFFER_HEX(BLE_HID_TAG, param->feature.data, param->feature.length);
        break;
    }
    case ESP_HIDD_DISCONNECT_EVENT: {
        ESP_LOGI(BLE_HID_TAG, "DISCONNECT: %s", esp_hid_disconnect_reason_str(esp_hidd_dev_transport_get(param->disconnect.dev), param->disconnect.reason));
        esp_hid_ble_gap_adv_start();
        break;
    }
    case ESP_HIDD_STOP_EVENT: {
        ESP_LOGI(BLE_HID_TAG, "STOP");
        break;
    }
    default:
        break;
    }
    return;
}

esp_err_t ble_hid_init(void)
{
    esp_err_t ret;
#if HID_DEV_MODE == HIDD_IDLE_MODE
    ESP_LOGE(BLE_HID_TAG, "Please turn on BT HID device or BLE!");
    return;
#endif
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(BLE_HID_TAG, "setting hid gap, mode:%d", HID_DEV_MODE);
    ret = esp_hid_gap_init(HID_DEV_MODE);
    ESP_ERROR_CHECK(ret);

#if CONFIG_BT_BLE_ENABLED
    ret = esp_hid_ble_gap_adv_init(ESP_HID_APPEARANCE_GAMEPAD, ble_hid_config.device_name);
    ESP_ERROR_CHECK(ret);

    if ((ret = esp_ble_gatts_register_callback(esp_hidd_gatts_event_handler)) != ESP_OK) {
        ESP_LOGE(BLE_HID_TAG, "GATTS register callback failed: %d", ret);
    }
    ESP_LOGI(BLE_HID_TAG, "setting ble device");
    ESP_ERROR_CHECK(
        esp_hidd_dev_init(&ble_hid_config, ESP_HID_TRANSPORT_BLE, ble_hidd_event_callback, &s_ble_hid_param.hid_dev));
#endif
    return ret;
}
