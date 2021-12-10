/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>

#include <esp_rmaker_core.h>
#include <esp_rmaker_ota.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_common_events.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_standard_params.h>
#include "esp_rmaker_utils.h"

#include "bsp_board.h"
#include "bsp_btn.h"
#include <device_driver.h>
#include "esp_check.h"
#include "app_wifi.h"
#include "app_sr.h"
#include "rmaker_devices.h"
#include "settings.h"
#include "app_led.h"
#include "ui_main.h"
#include "ui_net_config.h"

static const char *TAG = "rmaker_main";

static bool g_is_connected = 0;

static esp_err_t esp_box_init(void)
{
    const board_res_desc_t *brd = bsp_board_get_description();
    const sys_param_t *param = settings_get_parameter();
    uint16_t h;
    uint8_t s, v;
    app_pwm_led_get_customize_color(&h, &s, &v);
    uint8_t id_list[8];
    uint8_t cmd_num;
    cmd_num = app_sr_search_cmd_from_user_cmd(SR_CMD_LIGHT_ON, id_list, sizeof(id_list));
    ESP_RETURN_ON_FALSE(cmd_num > 0, ESP_FAIL, TAG, "not found sr command: light on");
    const sr_cmd_t *cmd_light_on = app_sr_get_cmd_from_id(id_list[0]);
    cmd_num = app_sr_search_cmd_from_user_cmd(SR_CMD_LIGHT_OFF, id_list, sizeof(id_list));
    ESP_RETURN_ON_FALSE(cmd_num > 0, ESP_FAIL, TAG, "not found sr command: light off");
    const sr_cmd_t *cmd_light_off = app_sr_get_cmd_from_id(id_list[0]);
    cmd_num = app_sr_search_cmd_from_user_cmd(SR_CMD_CUSTOMIZE_COLOR, id_list, sizeof(id_list));
    ESP_RETURN_ON_FALSE(cmd_num > 0, ESP_FAIL, TAG, "not found sr command: customize color");
    const sr_cmd_t *cmd_cc = app_sr_get_cmd_from_id(id_list[0]);
    char cmd_json[768] = {0};
    snprintf(cmd_json, sizeof(cmd_json),
             "[{\"status\":1,\"voice\":\"%s\",\"lang\":%u,\"str\":\"%s\"},"
             "{\"status\":0,\"voice\":\"%s\",\"lang\":%u,\"str\":\"%s\"},"
             "{\"hue\":%d,\"saturation\":%d,\"value\":%d,\"voice\":\"%s\",\"lang\":%u,\"str\":\"%s\"}]",
             cmd_light_on->phoneme, param->sr_lang, cmd_light_on->str,
             cmd_light_off->phoneme, param->sr_lang, cmd_light_off->str,
             h, s, v, cmd_cc->phoneme, param->sr_lang, cmd_cc->str);
    esp_box_light_param_t def_light_param = {
        .gpio_r = brd->PMOD2->row1[1],
        .gpio_g = brd->PMOD2->row1[2],
        .gpio_b = brd->PMOD2->row1[3],
        .hue = 0,
        .brightness = 100,
        .saturation = 100,
        .power = false,
        .name = "Light",
        .unique_name = "Light",
        .voice_cmd = cmd_json,
    };
    esp_box_light_cb_t *def_light_cb = calloc(1, sizeof(esp_box_light_cb_t));
    def_light_cb->color_cb = app_driver_set_light_color;
    def_light_cb->gpio_cb = app_driver_set_light_gpio;
    def_light_cb->status_cb = app_driver_set_light_power;
    def_light_cb->voice_cb = app_driver_set_light_voice;
    esp_box_light_device_create(def_light_param, def_light_cb);
    app_driver_light_init(def_light_param.unique_name, def_light_param.gpio_r, def_light_param.gpio_g,
                          def_light_param.gpio_b, def_light_param.hue, def_light_param.saturation, def_light_param.brightness,
                          def_light_param.power, def_light_param.voice_cmd);

    esp_box_switch_param_t def_switch_param = {
        .gpio = brd->PMOD2->row1[0],
        .active_level = 1,
        .power = false,
        .name = "Switch",
        .unique_name = "Switch",
        .voice_cmd = "[{\"status\":1,\"voice\":\"da kai kai guan\",\"zh\":\"打开开关\"},"
        "{\"status\":0,\"voice\":\"guan bi kai guan\",\"zh\":\"关闭开关\"}]",
    };
    esp_box_switch_cb_t *def_switch_cb = calloc(1, sizeof(esp_box_switch_cb_t));
    def_switch_cb->gpio_cb = app_driver_set_switch_gpio;
    def_switch_cb->status_cb = app_driver_set_switch_power;
    def_switch_cb->voice_cb = app_driver_set_switch_voice;
    esp_box_switch_device_create(def_switch_param, def_switch_cb);
    app_driver_switch_init(def_switch_param.unique_name, def_switch_param.gpio, def_switch_param.active_level, def_switch_param.power, def_switch_param.voice_cmd);

    esp_box_fan_param_t def_fan_param = {
        .gpio = brd->PMOD2->row2[0],
        .active_level = 1,
        .power = false,
        .name = "Fan",
        .unique_name = "Fan",
        .voice_cmd = "[{\"status\":1,\"voice\":\"da kai feng shan\",\"zh\":\"打开风扇\"},"
        "{\"status\":0,\"voice\":\"guan bi feng shan\",\"zh\":\"关闭风扇\"}]",
    };
    esp_box_fan_cb_t *def_fan_cb = calloc(1, sizeof(esp_box_fan_cb_t));
    def_fan_cb->gpio_cb = app_driver_set_fan_gpio;
    def_fan_cb->status_cb = app_driver_set_fan_power;
    def_fan_cb->voice_cb = app_driver_set_fan_voice;
    esp_box_fan_device_create(def_fan_param, def_fan_cb);
    app_driver_fan_init(def_fan_param.unique_name, def_fan_param.gpio, def_fan_param.power, def_fan_param.voice_cmd);

    return ESP_OK;
}

/* This executes in the context of default event loop task */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base != RMAKER_COMMON_EVENT) {
        return;
    }
    switch (event_id) {
    case RMAKER_MQTT_EVENT_CONNECTED:
        g_is_connected = 1;
        ESP_LOGI(TAG, "RMAKER connected");
        ui_net_config_update_cb(UI_NET_EVT_CLOUD_CONNECTED, NULL);
        ui_acquire();
        ui_main_status_bar_set_cloud(g_is_connected);
        ui_release();
        break;
    case RMAKER_MQTT_EVENT_DISCONNECTED:
        g_is_connected = 0;
        ESP_LOGI(TAG, "RMAKER disconnected");
        ui_acquire();
        ui_main_status_bar_set_cloud(g_is_connected);
        ui_release();
    default:
        break;
    }
}

static void rmaker_task(void *args)
{
    /* Initialize Wi-Fi. Note that, this should be called before esp_rmaker_node_init()
     */
    app_wifi_init();

    /* Initialize the ESP RainMaker Agent.
     * Note that this should be called after app_wifi_init() but before app_wifi_start()
     * */
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP-box", "esp.node.box");
    if (!node) {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        abort();
    }

    /* Initialize Box devices. */
    ESP_ERROR_CHECK(esp_box_init());

    /* Enable OTA */
    // esp_rmaker_ota_config_t ota_config = {
    //     .server_cert = ESP_RMAKER_OTA_DEFAULT_SERVER_CERT,
    // };
    // esp_rmaker_ota_enable(&ota_config, OTA_USING_TOPICS);

    /* Enable timezone service. */
    esp_rmaker_timezone_service_enable();

    esp_rmaker_system_serv_config_t serv_config = {
        .flags = SYSTEM_SERV_FLAGS_ALL,
        .reset_reboot_seconds = 2,
    };
    esp_rmaker_system_service_enable(&serv_config);

    /* Enable insights service. */
    // app_insights_enable();

    ESP_ERROR_CHECK(esp_event_handler_register(RMAKER_COMMON_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    /* Start rmaker core. */
    esp_rmaker_start();

    /* Start the Wi-Fi. */
    esp_err_t err = app_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not start Wifi");
    }

    vTaskDelete(NULL);
}

static void wifi_credential_reset(void *arg)
{
    ESP_LOGW(TAG, "WiFi credential reset");
    esp_rmaker_wifi_reset(0, 2);
}

void app_rmaker_start(void)
{
    bsp_btn_register_callback(BOARD_BTN_ID_BOOT, BUTTON_LONG_PRESS_START, wifi_credential_reset, NULL);

    BaseType_t ret_val = xTaskCreatePinnedToCore(rmaker_task, "RMaker Task", 6 * 1024, NULL, 1, NULL, 0);
    ESP_ERROR_CHECK_WITHOUT_ABORT((pdPASS == ret_val) ? ESP_OK : ESP_FAIL);
}

bool app_rmaker_is_connected(void)
{
    return g_is_connected;
}
