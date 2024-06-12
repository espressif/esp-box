/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "app_ui_event.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"

#define LIMIT_ROCKER(value) ((value) > 127 ? 127 : ((value) < -128 ? -128 : (value)))

static EventGroupHandle_t g_init_event_grp = NULL;
static EventGroupHandle_t g_app_task_event_grp = NULL;

int rocker_x1 = 0;
int rocker_y1 = 0;
int rocker_x2 = 0;
int rocker_y2 = 0;

TaskHandle_t game_pad_app_task_handle = NULL;
TaskHandle_t rc_app_task_handle = NULL;

uint8_t g_hid_mode = 0;   //0: USB, 1: BLE
uint8_t g_vibration_feedback_state = 0;
static uint8_t g_rocker_calibration_state = 1;

extern adc_oneshot_unit_handle_t game_mode_adc_handle;

/* Minimum, middle, and maximum values of the rocker ADC value */
uint16_t left_rocker_x_adc_value[3] = { 982, 2135, 3790 };
uint16_t left_rocker_y_adc_value[3] = { 537, 2126, 3640 };
uint16_t right_rocker_x_adc_value[3] = { 552, 2125, 3293 };
uint16_t right_rocker_y_adc_value[3] = { 535, 2099, 3785 };

typedef enum {
    APP_ESPNOW_CTRL_INIT,
    APP_ESPNOW_CTRL_BOUND,
    APP_ESPNOW_CTRL_MAX
} app_espnow_ctrl_status_t;

static app_espnow_ctrl_status_t s_espnow_ctrl_status = APP_ESPNOW_CTRL_INIT;

esp_err_t event_state_group_init(void)
{
    g_init_event_grp = xEventGroupCreate();
    ESP_RETURN_ON_FALSE(g_init_event_grp, ESP_ERR_NO_MEM, JOYSTICK_TAG, "Init event group init failed");
    xEventGroupClearBits( g_init_event_grp, USB_HID_INIT_STATE );
    xEventGroupClearBits( g_init_event_grp, BLE_HID_INIT_STATE );
    xEventGroupClearBits( g_init_event_grp, ROCKER_ADC_INIT_STATE );

    g_app_task_event_grp = xEventGroupCreate();
    ESP_RETURN_ON_FALSE(g_app_task_event_grp, ESP_ERR_NO_MEM, JOYSTICK_TAG, "APP task event group init failed");
    xEventGroupClearBits( g_app_task_event_grp, GAMEPAD_APP_TASK_STATE );
    xEventGroupClearBits( g_app_task_event_grp, RC_APP_TASK_STATE );
    ESP_LOGI(JOYSTICK_TAG, "event group init is  OK.");
    return ESP_OK;
}

static void app_wifi_init()
{
    esp_event_loop_create_default();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void esp_now_initiator_bind_press_cb(void)
{
    if (s_espnow_ctrl_status == APP_ESPNOW_CTRL_INIT) {
        ESP_LOGI(RC_APP_TAG, "initiator bind press");
        espnow_ctrl_initiator_bind(ESPNOW_ATTRIBUTE_KEY_1, true);
        s_espnow_ctrl_status = APP_ESPNOW_CTRL_BOUND;
        lv_obj_set_style_bg_color(ui_bindBtn, lv_color_hex(0x00FF7F), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        ESP_LOGI(RC_APP_TAG, "this device is already in bound status");
    }
}

static void esp_now_initiator_unbind_press_cb(void)
{
    if (s_espnow_ctrl_status == APP_ESPNOW_CTRL_BOUND) {
        ESP_LOGI(RC_APP_TAG, "initiator unbind press");
        espnow_ctrl_initiator_bind(ESPNOW_ATTRIBUTE_KEY_1, false);
        s_espnow_ctrl_status = APP_ESPNOW_CTRL_INIT;
        lv_obj_set_style_bg_color(ui_bindBtn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        ESP_LOGI(RC_APP_TAG, "this device is not been bound");
    }
}

static void app_espnow_event_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    if (base != ESP_EVENT_ESPNOW) {
        return;
    }

    switch (id) {
    case ESP_EVENT_ESPNOW_CTRL_BIND: {
        espnow_ctrl_bind_info_t *info = (espnow_ctrl_bind_info_t *)event_data;
        ESP_LOGI(RC_APP_TAG, "bind, uuid: " MACSTR ", initiator_type: %d", MAC2STR(info->mac), info->initiator_attribute);
        break;
    }

    case ESP_EVENT_ESPNOW_CTRL_UNBIND: {
        espnow_ctrl_bind_info_t *info = (espnow_ctrl_bind_info_t *)event_data;
        ESP_LOGI(RC_APP_TAG, "unbind, uuid: " MACSTR ", initiator_type: %d", MAC2STR(info->mac), info->initiator_attribute);
        break;
    }

    default:
        break;
    }
}

static void game_pad_app_task(void *pvParameters)
{
    ESP_LOGI(GAME_PAD_APP_TAG, "Game mode task start.");
    vibration_motor_init();

    if ( !(USB_HID_INIT_STATE & xEventGroupGetBits(g_init_event_grp)) ) {
        if (ESP_OK == usb_hid_init()) {
            xEventGroupSetBits( g_init_event_grp, USB_HID_INIT_STATE );
        } else {
            xEventGroupClearBits( g_init_event_grp, USB_HID_INIT_STATE );
        }
    }

    if ( !(BLE_HID_INIT_STATE & xEventGroupGetBits(g_init_event_grp)) ) {
        if (ESP_OK == ble_hid_init()) {
            xEventGroupSetBits( g_init_event_grp, BLE_HID_INIT_STATE );
        } else {
            xEventGroupClearBits( g_init_event_grp, BLE_HID_INIT_STATE );
        }
    }

    if (1 == read_rocker_value_from_flash("hid_mode")) {
        g_hid_mode = 1;
        lv_obj_set_x(ui_hidModeSelectBtnPart, 22);
        lv_label_set_text(ui_hidModeSelectLab, "BLE");
    } else {
        g_hid_mode = 0;
        lv_obj_set_x(ui_hidModeSelectBtnPart, -22);
        lv_label_set_text(ui_hidModeSelectLab, "USB");
    }

    box_rc_button_init();

    if ( !(ROCKER_ADC_INIT_STATE & xEventGroupGetBits(g_init_event_grp))) {
        if (ESP_OK == rocker_adc_init()) {
            xEventGroupSetBits( g_init_event_grp, ROCKER_ADC_INIT_STATE );
        } else {
            xEventGroupClearBits( g_init_event_grp, ROCKER_ADC_INIT_STATE );
        }
    }

    if (1 == read_rocker_value_from_flash("calibrate_state")) {
        left_rocker_x_adc_value[0] = read_rocker_value_from_flash("left_x_min");
        left_rocker_x_adc_value[1] = read_rocker_value_from_flash("left_x_mid");
        left_rocker_x_adc_value[2] = read_rocker_value_from_flash("left_x_max");

        left_rocker_y_adc_value[0] = read_rocker_value_from_flash("left_y_min");
        left_rocker_y_adc_value[1] = read_rocker_value_from_flash("left_y_mid");
        left_rocker_y_adc_value[2] = read_rocker_value_from_flash("left_y_max");

        right_rocker_x_adc_value[0] = read_rocker_value_from_flash("right_x_min");
        right_rocker_x_adc_value[1] = read_rocker_value_from_flash("right_x_mid");
        right_rocker_x_adc_value[2] = read_rocker_value_from_flash("right_x_max");

        right_rocker_y_adc_value[0] = read_rocker_value_from_flash("right_y_min");
        right_rocker_y_adc_value[1] = read_rocker_value_from_flash("right_y_mid");
        right_rocker_y_adc_value[2] = read_rocker_value_from_flash("right_y_max");
    }

    while (1) {
        while (1 == g_rocker_calibration_state) {
            uint16_t rocker_adc_value[4] = {0};
            get_rocker_adc_value_in_game_mode(rocker_adc_value);

            bsp_display_lock(0);
            if (rocker_adc_value[0] >= left_rocker_x_adc_value[1]) {
                rocker_x1 = (int)(((rocker_adc_value[0] - left_rocker_x_adc_value[1] * 1.0) / ((left_rocker_x_adc_value[2] - left_rocker_x_adc_value[1]) * 1.0)) * GAME_ROCKET_RANGE);
                lv_obj_set_x(ui_leftRockerBtn, rocker_x1 / 5.0);
            } else {
                rocker_x1 = (int)(((rocker_adc_value[0] - left_rocker_x_adc_value[1] * 1.0) / ((left_rocker_x_adc_value[1] - left_rocker_x_adc_value[0]) * 1.0)) * GAME_ROCKET_RANGE);
                lv_obj_set_x(ui_leftRockerBtn, rocker_x1 / 5.0);
            }

            if (rocker_adc_value[1] >= left_rocker_y_adc_value[1]) {
                rocker_y1 = (int)((-(rocker_adc_value[1] - left_rocker_y_adc_value[1] * 1.0) / ((left_rocker_y_adc_value[2] - left_rocker_y_adc_value[1]) * 1.0)) * GAME_ROCKET_RANGE);
                lv_obj_set_y(ui_leftRockerBtn, rocker_y1 / 5.0);
            } else {
                rocker_y1 = (int)((-(rocker_adc_value[1] - left_rocker_y_adc_value[1] * 1.0) / ((left_rocker_y_adc_value[1] - left_rocker_y_adc_value[0]) * 1.0)) * GAME_ROCKET_RANGE);
                lv_obj_set_y(ui_leftRockerBtn, rocker_y1 / 5.0);
            }

            if (rocker_adc_value[2] >= right_rocker_x_adc_value[1]) {
                rocker_x2 = (int)(((rocker_adc_value[2] - right_rocker_x_adc_value[1] * 1.0) / ((right_rocker_x_adc_value[2] - right_rocker_x_adc_value[1]) * 1.0)) * GAME_ROCKET_RANGE);
                lv_obj_set_x(ui_rightRockerBtn, rocker_x2 / 5.0);
            } else {
                rocker_x2 = (int)(((rocker_adc_value[2] - right_rocker_x_adc_value[1] * 1.0) / ((right_rocker_x_adc_value[1] - right_rocker_x_adc_value[0]) * 1.0)) * GAME_ROCKET_RANGE);
                lv_obj_set_x(ui_rightRockerBtn, rocker_x2 / 5.0);
            }

            if (rocker_adc_value[3] >= right_rocker_y_adc_value[1]) {
                rocker_y2 = (int)((-(rocker_adc_value[3] - right_rocker_y_adc_value[1] * 1.0) / ((right_rocker_y_adc_value[2] - right_rocker_y_adc_value[1]) * 1.0)) * GAME_ROCKET_RANGE);
                lv_obj_set_y(ui_rightRockerBtn, rocker_y2 / 5.0);
            } else {
                rocker_y2 = (int)((-(rocker_adc_value[3] - right_rocker_y_adc_value[1] * 1.0) / ((right_rocker_y_adc_value[1] - right_rocker_y_adc_value[0]) * 1.0)) * GAME_ROCKET_RANGE);
                lv_obj_set_y(ui_rightRockerBtn, rocker_y2 / 5.0);
            }

            bsp_display_unlock();

            rocker_x1 = LIMIT_ROCKER(rocker_x1);
            rocker_y1 = LIMIT_ROCKER(rocker_y1);
            rocker_x2 = LIMIT_ROCKER(rocker_x2);
            rocker_y2 = LIMIT_ROCKER(rocker_y2);

            if (1 == g_hid_mode) {
                uint16_t button = (uint16_t)get_pressed_button_value();
                ble_hid_send_joystick_value(button, rocker_x1, rocker_y1, rocker_x2, rocker_y2);
            } else {
                if (tud_mounted()) {
                    usb_hid_send_joystick_value(HID_ITF_PROTOCOL_GAMEPAD, rocker_x1, rocker_y1, rocker_x2, rocker_y2, get_pressed_button_value());
                }
            }
            vTaskDelay(pdMS_TO_TICKS(2));

            if ( !(GAMEPAD_APP_TASK_STATE & xEventGroupGetBits(g_app_task_event_grp)) ) {
                ESP_LOGI(GAME_PAD_APP_TAG, "Game mode task deleted.");
                box_rc_button_delete();
                vTaskDelete(NULL);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void rc_app_task(void *pvParameters)
{
    ESP_LOGI(RC_APP_TAG, "RC mode task start.");

    espnow_storage_init();

    app_wifi_init();

    esp_wifi_set_channel(13, WIFI_SECOND_CHAN_NONE);

    espnow_config_t espnow_config = ESPNOW_INIT_CONFIG_DEFAULT();
    espnow_init(&espnow_config);

    esp_event_handler_register(ESP_EVENT_ESPNOW, ESP_EVENT_ANY_ID, app_espnow_event_handler, NULL);

    box_rc_button_init();

    if ( s_espnow_ctrl_status == APP_ESPNOW_CTRL_INIT ) {
        lv_obj_set_style_bg_color(ui_bindBtn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_bg_color(ui_bindBtn, lv_color_hex(0x00FF7F), LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if ( !(ROCKER_ADC_INIT_STATE & xEventGroupGetBits(g_init_event_grp))) {
        if (ESP_OK == rocker_adc_init()) {
            xEventGroupSetBits( g_init_event_grp, ROCKER_ADC_INIT_STATE );
        } else {
            xEventGroupClearBits( g_init_event_grp, ROCKER_ADC_INIT_STATE );
        }
    }

    if (1 == read_rocker_value_from_flash("calibrate_state")) {
        left_rocker_x_adc_value[0] = read_rocker_value_from_flash("left_x_min");
        left_rocker_x_adc_value[1] = read_rocker_value_from_flash("left_x_mid");
        left_rocker_x_adc_value[2] = read_rocker_value_from_flash("left_x_max");

        left_rocker_y_adc_value[0] = read_rocker_value_from_flash("left_y_min");
        left_rocker_y_adc_value[1] = read_rocker_value_from_flash("left_y_mid");
        left_rocker_y_adc_value[2] = read_rocker_value_from_flash("left_y_max");

        right_rocker_x_adc_value[0] = read_rocker_value_from_flash("right_x_min");
        right_rocker_x_adc_value[1] = read_rocker_value_from_flash("right_x_mid");
        right_rocker_x_adc_value[2] = read_rocker_value_from_flash("right_x_max");

        right_rocker_y_adc_value[0] = read_rocker_value_from_flash("right_y_min");
        right_rocker_y_adc_value[1] = read_rocker_value_from_flash("right_y_mid");
        right_rocker_y_adc_value[2] = read_rocker_value_from_flash("right_y_max");
    }

    uint16_t rocker_adc_value[4] = {0};
    while (1) {
        while (1 == g_rocker_calibration_state) {
            get_rocker_adc_value_in_rc_mode(rocker_adc_value, 10, 0.6);

            if (rocker_adc_value[0] >= left_rocker_x_adc_value[1]) {
                rocker_x1 = (int)(((rocker_adc_value[0] - left_rocker_x_adc_value[1] * 1.0) / ((left_rocker_x_adc_value[2] - left_rocker_x_adc_value[1]) * 1.0)) * RC_ROCKET_RANGE);
            } else {
                rocker_x1 = (int)(((rocker_adc_value[0] - left_rocker_x_adc_value[1] * 1.0) / ((left_rocker_x_adc_value[1] - left_rocker_x_adc_value[0]) * 1.0)) * RC_ROCKET_RANGE);
            }

            if (rocker_adc_value[1] >= left_rocker_y_adc_value[1]) {
                rocker_y1 = (int)(((rocker_adc_value[1] - left_rocker_y_adc_value[1] * 1.0) / ((left_rocker_y_adc_value[2] - left_rocker_y_adc_value[1]) * 1.0)) * RC_ROCKET_RANGE);
            } else {
                rocker_y1 = (int)(((rocker_adc_value[1] - left_rocker_y_adc_value[1] * 1.0) / ((left_rocker_y_adc_value[1] - left_rocker_y_adc_value[0]) * 1.0)) * RC_ROCKET_RANGE);
            }

            if (rocker_adc_value[2] >= right_rocker_x_adc_value[1]) {
                rocker_x2 = (int)(((rocker_adc_value[2] - right_rocker_x_adc_value[1] * 1.0) / ((right_rocker_x_adc_value[2] - right_rocker_x_adc_value[1]) * 1.0)) * RC_ROCKET_RANGE);
            } else {
                rocker_x2 = (int)(((rocker_adc_value[2] - right_rocker_x_adc_value[1] * 1.0) / ((right_rocker_x_adc_value[1] - right_rocker_x_adc_value[0]) * 1.0)) * RC_ROCKET_RANGE);
            }

            if (rocker_adc_value[3] >= right_rocker_y_adc_value[1]) {
                rocker_y2 = (int)(((rocker_adc_value[3] - right_rocker_y_adc_value[1] * 1.0) / ((right_rocker_y_adc_value[2] - right_rocker_y_adc_value[1]) * 1.0)) * RC_ROCKET_RANGE);
            } else {
                rocker_y2 = (int)(((rocker_adc_value[3] - right_rocker_y_adc_value[1] * 1.0) / ((right_rocker_y_adc_value[1] - right_rocker_y_adc_value[0]) * 1.0)) * RC_ROCKET_RANGE);
            }

            if (s_espnow_ctrl_status == APP_ESPNOW_CTRL_BOUND) {
                rc_channel_state_t channel_state = get_rc_button_state();
                espnow_ctrl_initiator_send(ESPNOW_ATTRIBUTE_KEY_1, ESPNOW_ATTRIBUTE_POWER, channel_state.channel_1_status, channel_state.channel_2_status,
                                           rocker_x1, rocker_y1, rocker_x2, rocker_y2, channel_state.channel_3_status, channel_state.channel_4_status);
            } else {
                static int count = 0;
                count++;
                if (count % 1000 == 0) {
                    ESP_LOGI(RC_APP_TAG, "please double click to bind the devices firstly");
                }
            }
            char dP_label_data[2];
            char dR_label_data[2];
            char dT_label_data[5];
            char dY_label_data[2];
            sprintf(dP_label_data, "%d", rocker_y2);
            sprintf(dR_label_data, "%d", rocker_x2);
            sprintf(dT_label_data, "%d", rocker_y1);
            strcat(dT_label_data, "%");
            sprintf(dY_label_data, "%d", rocker_x1);

            bsp_display_lock(0);
            lv_label_set_text(ui_dPLabelData, dP_label_data);
            lv_label_set_text(ui_dRLabelData, dR_label_data);
            lv_label_set_text(ui_dTLabelData, dT_label_data);
            lv_label_set_text(ui_dYLabelData, dY_label_data);
            lv_obj_set_x(ui_dLeftRockerBtn, (int)((rocker_x1 / 90.0) * 28));
            lv_obj_set_y(ui_dLeftRockerBtn, (int)(-(rocker_y1 / 90.0) * 28));
            lv_obj_set_x(ui_dRightRockerBtn, (int)((rocker_x2 / 90.0) * 28));
            lv_obj_set_y(ui_dRightRockerBtn, (int)(-(rocker_y2 / 90.0) * 28));
            bsp_display_unlock();

            vTaskDelay(pdMS_TO_TICKS(15));

            if ( !(RC_APP_TASK_STATE & xEventGroupGetBits(g_app_task_event_grp)) ) {
                ESP_LOGI(RC_APP_TAG, "RC mode task deleted.");
                box_rc_button_delete();
                vTaskDelete(NULL);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_ui_event_mainToGamePad(lv_event_t *e)
{
    ESP_LOGI(GAME_PAD_APP_TAG, "Game mode start.");
    xEventGroupSetBits( g_app_task_event_grp, GAMEPAD_APP_TASK_STATE );
    xTaskCreate(game_pad_app_task, "game_pad_app_task", 1024 * 4, NULL, 10, &game_pad_app_task_handle);
}

void app_ui_event_mainToNesEmulator(lv_event_t *e)
{
    ESP_LOGI(JOYSTICK_TAG, "NES Emulator start.");
}

void app_ui_event_mainToRcAPP(lv_event_t *e)
{
    ESP_LOGI(JOYSTICK_TAG, "RC mode start.");
    xEventGroupSetBits( g_app_task_event_grp, RC_APP_TASK_STATE );
    xTaskCreate(rc_app_task, "rc_app_task", 1024 * 4, NULL, 10, &rc_app_task_handle);
}

void app_ui_event_gameReturnBtn(lv_event_t *e)
{
    ESP_LOGI(GAME_PAD_APP_TAG, "Game mode end.");
    xEventGroupClearBits( g_app_task_event_grp, GAMEPAD_APP_TASK_STATE );
}

void app_ui_event_espnow_bind(lv_event_t *e)
{
    ESP_LOGI(RC_APP_TAG, "espnow bind.");
    if ( s_espnow_ctrl_status == APP_ESPNOW_CTRL_INIT ) {
        esp_now_initiator_bind_press_cb();
    } else {
        esp_now_initiator_unbind_press_cb();
    }
}

void app_ui_event_rocker_calibration(lv_event_t *e)
{
    char rocker_value_str[6];
    uint16_t rocker_adc_value[4] = {0};
    ESP_LOGI(GAME_PAD_APP_TAG, "rocker_calibration.");
    g_rocker_calibration_state = 0;
    ESP_LOGI(GAME_PAD_APP_TAG, "Release all rocker");
    lv_obj_clear_flag(ui_RockerCalibrationSettingPanel, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(ui_rockerCalibrationStartBtnText, "Calibrating");
    lv_label_set_text(ui_rockerCalibrationTipsLab, "Release all rocker");
    lv_refr_now(NULL);
    vTaskDelay(pdMS_TO_TICKS(1500));
    left_rocker_x_adc_value[1] = 0;
    left_rocker_y_adc_value[1] = 0;
    right_rocker_x_adc_value[1] = 0;
    right_rocker_y_adc_value[1] = 0;

    for (int i = 0; i < 10; i++) {
        get_rocker_adc_value_in_game_mode(rocker_adc_value);
        left_rocker_x_adc_value[1] += rocker_adc_value[0];
        left_rocker_y_adc_value[1] += rocker_adc_value[1];
        right_rocker_x_adc_value[1] += rocker_adc_value[2];
        right_rocker_y_adc_value[1] += rocker_adc_value[3];
        printf("%d, %d, %d, %d\n", rocker_adc_value[0], rocker_adc_value[1], rocker_adc_value[2], rocker_adc_value[3]);
    }
    left_rocker_x_adc_value[1] = left_rocker_x_adc_value[1] / 10.0;
    left_rocker_y_adc_value[1] = left_rocker_y_adc_value[1] / 10.0;
    right_rocker_x_adc_value[1] = right_rocker_x_adc_value[1] / 10.0;
    right_rocker_y_adc_value[1] = right_rocker_y_adc_value[1] / 10.0;
    printf("left x-mid: %d, left y-mid: %d, right x-mid: %d, right y-mid: %d\n", left_rocker_x_adc_value[1], left_rocker_y_adc_value[1],
           right_rocker_x_adc_value[1], right_rocker_y_adc_value[1]);
    sprintf(rocker_value_str, "%d", left_rocker_x_adc_value[1]);
    flash_write_state("left_x_mid", rocker_value_str);

    sprintf(rocker_value_str, "%d", left_rocker_y_adc_value[1]);
    flash_write_state("left_y_mid", rocker_value_str);

    sprintf(rocker_value_str, "%d", right_rocker_x_adc_value[1]);
    flash_write_state("right_x_mid", rocker_value_str);

    sprintf(rocker_value_str, "%d", right_rocker_y_adc_value[1]);
    flash_write_state("right_y_mid", rocker_value_str);

    //get left rocker x-adc min value
    ESP_LOGI(GAME_PAD_APP_TAG, "Push the left rocker to the far left");
    lv_label_set_text(ui_rockerCalibrationTipsLab, "Push the left rocker to the far left");
    lv_refr_now(NULL);
    // Wait for the rocker to push into position
    while (rocker_adc_value[0] > left_rocker_x_adc_value[1] - 600) {
        get_rocker_adc_value_in_game_mode(rocker_adc_value);
        lv_obj_set_x(ui_leftRockerCalibrateBtn, -18);
        lv_refr_now(NULL);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelay(pdMS_TO_TICKS(500));
    left_rocker_x_adc_value[0] = 0;
    // Update the left rocker X axis minimum
    for (int i = 0; i < 5; i++) {
        get_rocker_adc_value_in_game_mode(rocker_adc_value);
        left_rocker_x_adc_value[0] += rocker_adc_value[0];
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    left_rocker_x_adc_value[0] = left_rocker_x_adc_value[0] / 5;
    printf("left x min: %d\n", left_rocker_x_adc_value[0]);

    sprintf(rocker_value_str, "%d", left_rocker_x_adc_value[0]);
    flash_write_state("left_x_min", rocker_value_str);

    lv_obj_set_x(ui_leftRockerCalibrateBtn, 0);
    lv_label_set_text(ui_rockerCalibrationTipsLab, "Release left rocker");
    lv_refr_now(NULL);
    vTaskDelay(pdMS_TO_TICKS(1500));
    //get left rocker x-adc max value
    ESP_LOGI(GAME_PAD_APP_TAG, "Push the left rocker to the far right");
    lv_label_set_text(ui_rockerCalibrationTipsLab, "Push the left rocker to the far right");
    lv_refr_now(NULL);
    // Wait for the rocker to push into position
    while (rocker_adc_value[0] < left_rocker_x_adc_value[1] + 600) {
        get_rocker_adc_value_in_game_mode(rocker_adc_value);
        lv_obj_set_x(ui_leftRockerCalibrateBtn, 18);
        lv_refr_now(NULL);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    left_rocker_x_adc_value[2] = 0;
    // Update the left rocker X axis maximum
    for (int i = 0; i < 5; i++) {
        get_rocker_adc_value_in_game_mode(rocker_adc_value);
        left_rocker_x_adc_value[2] += rocker_adc_value[0];
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    left_rocker_x_adc_value[2] = left_rocker_x_adc_value[2] / 5;
    printf("left x max: %d\n", left_rocker_x_adc_value[2]);
    sprintf(rocker_value_str, "%d", left_rocker_x_adc_value[2]);
    flash_write_state("left_x_max", rocker_value_str);

    lv_obj_set_x(ui_leftRockerCalibrateBtn, 0);
    ESP_LOGI(GAME_PAD_APP_TAG, "Release left rocker");
    lv_label_set_text(ui_rockerCalibrationTipsLab, "Release left rocker");
    lv_refr_now(NULL);
    vTaskDelay(pdMS_TO_TICKS(1500));

    //get left rocker y-adc max value
    ESP_LOGI(GAME_PAD_APP_TAG, "Push the left rocker to the far top");
    lv_label_set_text(ui_rockerCalibrationTipsLab, "Push the left rocker to the far top");
    lv_refr_now(NULL);
    // Wait for the rocker to push into position
    while (rocker_adc_value[1] < left_rocker_y_adc_value[1] + 600) {
        get_rocker_adc_value_in_game_mode(rocker_adc_value);
        lv_obj_set_y(ui_leftRockerCalibrateBtn, -18);
        lv_refr_now(NULL);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    left_rocker_y_adc_value[2] = 0;
    // Update the left rocker Y axis maximum
    for (int i = 0; i < 5; i++) {
        get_rocker_adc_value_in_game_mode(rocker_adc_value);
        left_rocker_y_adc_value[2] += rocker_adc_value[1];
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    left_rocker_y_adc_value[2] = left_rocker_y_adc_value[2] / 5;
    printf("left y max: %d\n", left_rocker_y_adc_value[2]);
    sprintf(rocker_value_str, "%d", left_rocker_y_adc_value[2]);
    flash_write_state("left_y_max", rocker_value_str);
    lv_obj_set_y(ui_leftRockerCalibrateBtn, 0);
    lv_label_set_text(ui_rockerCalibrationTipsLab, "Release left rocker");
    lv_refr_now(NULL);
    vTaskDelay(pdMS_TO_TICKS(1500));
    //get left rocker y-adc min value
    ESP_LOGI(GAME_PAD_APP_TAG, "Push the left rocker to the far bottom");
    lv_label_set_text(ui_rockerCalibrationTipsLab, "Push the left rocker to the far bottom");
    lv_refr_now(NULL);
    // Wait for the rocker to push into position
    while (rocker_adc_value[1] > left_rocker_y_adc_value[1] - 600) {
        get_rocker_adc_value_in_game_mode(rocker_adc_value);
        lv_obj_set_y(ui_leftRockerCalibrateBtn, 18);
        lv_refr_now(NULL);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    left_rocker_y_adc_value[0] = 0;
    // Update the left rocker Y axis minimum
    for (int i = 0; i < 5; i++) {
        get_rocker_adc_value_in_game_mode(rocker_adc_value);
        left_rocker_y_adc_value[0] += rocker_adc_value[1];
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    left_rocker_y_adc_value[0] = left_rocker_y_adc_value[0] / 5;
    printf("left y min: %d\n", left_rocker_y_adc_value[0]);
    sprintf(rocker_value_str, "%d", left_rocker_y_adc_value[0]);
    flash_write_state("left_y_min", rocker_value_str);
    lv_obj_set_y(ui_leftRockerCalibrateBtn, 0);
    lv_label_set_text(ui_rockerCalibrationTipsLab, "Release left rocker");
    lv_refr_now(NULL);
    vTaskDelay(pdMS_TO_TICKS(1500));

    ESP_LOGI(GAME_PAD_APP_TAG, "Push the right rocker to the far left");
    lv_label_set_text(ui_rockerCalibrationTipsLab, "Push the right rocker to the far left");
    lv_refr_now(NULL);
    // Wait for the rocker to push into position
    while (rocker_adc_value[2] > right_rocker_x_adc_value[1] - 600) {
        get_rocker_adc_value_in_game_mode(rocker_adc_value);
        lv_obj_set_x(ui_rightRockerCalibrateBtn, -18);
        lv_refr_now(NULL);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    right_rocker_x_adc_value[0] = 0;
    // Update the right rocker X axis minimum
    for (int i = 0; i < 5; i++) {
        get_rocker_adc_value_in_game_mode(rocker_adc_value);
        right_rocker_x_adc_value[0] += rocker_adc_value[2];
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    right_rocker_x_adc_value[0] = right_rocker_x_adc_value[0] / 5;
    printf("right x min: %d\n", right_rocker_x_adc_value[0]);
    sprintf(rocker_value_str, "%d", right_rocker_x_adc_value[0]);
    flash_write_state("right_x_min", rocker_value_str);
    lv_obj_set_x(ui_rightRockerCalibrateBtn, 0);
    lv_label_set_text(ui_rockerCalibrationTipsLab, "Release right rocker");
    lv_refr_now(NULL);
    vTaskDelay(pdMS_TO_TICKS(1500));

    //get right rocker x-adc max value
    ESP_LOGI(GAME_PAD_APP_TAG, "Push the right rocker to the far right");
    lv_label_set_text(ui_rockerCalibrationTipsLab, "Push the right rocker to the far right");
    lv_refr_now(NULL);
    // Wait for the rocker to push into position
    while (rocker_adc_value[2] < right_rocker_x_adc_value[1] + 600) {
        get_rocker_adc_value_in_game_mode(rocker_adc_value);
        lv_obj_set_x(ui_rightRockerCalibrateBtn, 18);
        lv_refr_now(NULL);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    right_rocker_x_adc_value[2] = 0;
    // Update the right rocker X axis maximum
    for (int i = 0; i < 5; i++) {
        get_rocker_adc_value_in_game_mode(rocker_adc_value);
        right_rocker_x_adc_value[2] += rocker_adc_value[2];
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    right_rocker_x_adc_value[2] = right_rocker_x_adc_value[2] / 5;
    printf("right x max: %d\n", right_rocker_x_adc_value[2]);
    sprintf(rocker_value_str, "%d", right_rocker_x_adc_value[2]);
    flash_write_state("right_x_max", rocker_value_str);
    lv_obj_set_x(ui_rightRockerCalibrateBtn, 0);
    ESP_LOGI(GAME_PAD_APP_TAG, "Release right rocker");
    lv_label_set_text(ui_rockerCalibrationTipsLab, "Release right rocker");
    lv_refr_now(NULL);
    vTaskDelay(pdMS_TO_TICKS(1500));

    //get right rocker y-adc max value
    ESP_LOGI(GAME_PAD_APP_TAG, "Push the right rocker to the far top");
    lv_label_set_text(ui_rockerCalibrationTipsLab, "Push the right rocker to the far top");
    lv_refr_now(NULL);
    // Wait for the rocker to push into position
    while (rocker_adc_value[3] < right_rocker_y_adc_value[1] + 600) {
        get_rocker_adc_value_in_game_mode(rocker_adc_value);
        lv_obj_set_y(ui_rightRockerCalibrateBtn, -18);
        lv_refr_now(NULL);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    right_rocker_y_adc_value[2] = 0;
    // Update the right rocker Y axis maximum
    for (int i = 0; i < 5; i++) {
        get_rocker_adc_value_in_game_mode(rocker_adc_value);
        right_rocker_y_adc_value[2] += rocker_adc_value[3];
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    right_rocker_y_adc_value[2] = right_rocker_y_adc_value[2] / 5;
    printf("right y max: %d\n", right_rocker_y_adc_value[2]);
    sprintf(rocker_value_str, "%d", right_rocker_y_adc_value[2]);
    flash_write_state("right_y_max", rocker_value_str);
    lv_obj_set_y(ui_rightRockerCalibrateBtn, 0);
    lv_label_set_text(ui_rockerCalibrationTipsLab, "Release right rocker");
    lv_refr_now(NULL);
    vTaskDelay(pdMS_TO_TICKS(1500));
    //get right rocker y-adc min value
    ESP_LOGI(GAME_PAD_APP_TAG, "Push the right rocker to the far bottom");
    lv_label_set_text(ui_rockerCalibrationTipsLab, "Push the right rocker to the far bottom");
    lv_refr_now(NULL);
    // Wait for the rocker to push into position
    while (rocker_adc_value[3] > right_rocker_y_adc_value[1] - 600) {
        get_rocker_adc_value_in_game_mode(rocker_adc_value);
        lv_obj_set_y(ui_rightRockerCalibrateBtn, 18);
        lv_refr_now(NULL);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    right_rocker_y_adc_value[0] = 0;
    // Update the right rocker Y axis minimum
    for (int i = 0; i < 5; i++) {
        get_rocker_adc_value_in_game_mode(rocker_adc_value);
        right_rocker_y_adc_value[0] += rocker_adc_value[3];
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    right_rocker_y_adc_value[0] = right_rocker_y_adc_value[0] / 5;
    printf("right y min: %d\n", right_rocker_y_adc_value[0]);
    sprintf(rocker_value_str, "%d", right_rocker_y_adc_value[0]);
    flash_write_state("right_y_min", rocker_value_str);

    lv_obj_set_y(ui_rightRockerCalibrateBtn, 0);
    lv_label_set_text(ui_rockerCalibrationTipsLab, "Calibration success!");
    lv_refr_now(NULL);
    vTaskDelay(pdMS_TO_TICKS(2000));
    lv_obj_add_flag(ui_RockerCalibrationSettingPanel, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(ui_rockerCalibrationTipsLab, "Release all rocker");
    lv_label_set_text(ui_rockerCalibrationStartBtnText, "Start");

    flash_write_state("calibrate_state", "1");

    g_rocker_calibration_state = 1;
}

void app_ui_event_rcReturnBtn(lv_event_t *e)
{
    ESP_LOGI(RC_APP_TAG, "RC mode end.");
    xEventGroupClearBits( g_app_task_event_grp, RC_APP_TASK_STATE );
}

void app_ui_event_hid_mode_select(lv_event_t *e)
{
    g_hid_mode = !g_hid_mode;
    if (g_hid_mode) {
        ESP_LOGI(GAME_PAD_APP_TAG, "BLE HID mode.");
        lv_obj_set_x(ui_hidModeSelectBtnPart, 22);
        lv_label_set_text(ui_hidModeSelectLab, "BLE");
        flash_write_state("hid_mode", "1");
    } else {
        ESP_LOGI(GAME_PAD_APP_TAG, "USB HID mode.");
        lv_obj_set_x(ui_hidModeSelectBtnPart, -22);
        lv_label_set_text(ui_hidModeSelectLab, "USB");
        flash_write_state("hid_mode", "0");
    }
}

void app_ui_event_vibration_feedback(lv_event_t *e)
{
    g_vibration_feedback_state = !g_vibration_feedback_state;
    if (g_vibration_feedback_state) {
        ESP_LOGI(GAME_PAD_APP_TAG, "Vibration Feedback ON.");
        lv_obj_set_x(ui_VibrationFeedbackBtnPart, 22);
        lv_label_set_text(ui_VibrationFeedbackStateLab, "ON");
        lv_obj_set_style_bg_color(ui_VibrationFeedbackBtnPart, lv_color_hex(0xF2A860), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(ui_VibrationFeedbackBtn, lv_color_hex(0xF2A860), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        ESP_LOGI(GAME_PAD_APP_TAG, "Vibration Feedback OFF.");
        lv_obj_set_x(ui_VibrationFeedbackBtnPart, -22);
        lv_label_set_text(ui_VibrationFeedbackStateLab, "OFF");
        lv_obj_set_style_bg_color(ui_VibrationFeedbackBtnPart, lv_color_hex(0xA8A8A8), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(ui_VibrationFeedbackBtn, lv_color_hex(0xA8A8A8), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

uint8_t get_vibration_feedback_state(void)
{
    return g_vibration_feedback_state;
}
