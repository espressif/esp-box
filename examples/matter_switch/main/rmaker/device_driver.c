/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <esp_log.h>
#include <sdkconfig.h>
#include <string.h>
#include "esp_check.h"
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_types.h>

#include <app_led.h>
#include <device_driver.h>
#include "driver/gpio.h"
#include <rmaker_devices.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "cJSON.h"
#include "app_sr.h"
#include "app_fan.h"
#include "app_switch.h"

static const char *TAG = "esp_box_driver";


static void cmd_write_to_sr(const sr_cmd_t *cmd)
{
    if (0 != app_sr_search_cmd_from_phoneme(cmd->phoneme, NULL, 1)) {
        ESP_LOGW(TAG, "The phoneme(%s) already be configurated, drop it", cmd->phoneme);
        return;
    }
    uint8_t id_list[8];
    uint8_t cmd_num = app_sr_search_cmd_from_user_cmd(cmd->cmd, id_list, sizeof(id_list));
    if (cmd_num) {
        if (cmd_num < sizeof(id_list)) {
            app_sr_add_cmd(cmd);
        } else {
            ESP_LOGE(TAG, "The voice of an action cannot exceed %d", sizeof(id_list));
            app_sr_modify_cmd(id_list[sizeof(id_list) - 1], cmd);
        }
    }
}

static void parse_cmd(const cJSON *voice_item, sr_cmd_t *cmd)
{
    cJSON *item = cJSON_GetObjectItem(voice_item, "lang");
    if (item->valueint < SR_LANG_MAX) {
        cmd->lang = item->valueint;
    }
    item = cJSON_GetObjectItem(voice_item, "str");
    if (item) {
        strncpy(cmd->str, item->valuestring, SR_CMD_STR_LEN_MAX);
    }
    item = cJSON_GetObjectItem(voice_item, "voice");
    if (item) {
        strncpy(cmd->phoneme, item->valuestring, SR_CMD_PHONEME_LEN_MAX);
    }
}

static esp_err_t parse_sr_config(const char *json_text)
{
    cJSON *root_obj = cJSON_Parse(json_text);
    ESP_RETURN_ON_FALSE(NULL != root_obj, ESP_FAIL, TAG, "Can't parse json");

    cJSON *voice_item_array = root_obj;
    int voice_item_size = cJSON_GetArraySize(voice_item_array);
    ESP_LOGI(TAG, "voice cmd num=%d", voice_item_size);
    for (size_t i = 0; i < voice_item_size; i++) {
        sr_cmd_t cmd = {0};
        cJSON *voice_item = cJSON_GetArrayItem(voice_item_array, i);
        cJSON *sw_item = cJSON_GetObjectItem(voice_item, "status");
        if (NULL == sw_item) { // Customize Color
            uint16_t h = (uint16_t) cJSON_GetObjectItem(voice_item, "hue")->valuedouble;
            uint8_t s = (uint8_t) cJSON_GetObjectItem(voice_item, "saturation")->valuedouble;
            uint8_t v = (uint8_t) cJSON_GetObjectItem(voice_item, "value")->valuedouble;
            if (v < 8) { // make sure the light can be observed
                v = 8;
            }
            app_pwm_led_set_customize_color(h, s, v);
            parse_cmd(voice_item, &cmd);
            cmd.cmd = SR_CMD_CUSTOMIZE_COLOR;
            cmd_write_to_sr(&cmd);
        } else { // light state
            if (cJSON_GetObjectItem(voice_item, "status")->valuedouble == 1) {
                parse_cmd(voice_item, &cmd);
                cmd.cmd = SR_CMD_LIGHT_ON;
                cmd_write_to_sr(&cmd);
            } else {
                parse_cmd(voice_item, &cmd);
                cmd.cmd = SR_CMD_LIGHT_OFF;
                cmd_write_to_sr(&cmd);
            }
        }
    }
    // actully write all commands to sr
    app_sr_update_cmds();

    //free the memory
    cJSON_Delete(root_obj);
    return ESP_OK;
}

esp_err_t app_driver_light_init(char *unique_name, int gpio_r, int gpio_g, int gpio_b, int h, int s, int v, int power, char *voice)
{
    return ESP_OK;
}

esp_err_t app_driver_set_light_color(char *unique_name, int h, int s, int v)
{
    ESP_LOGI(TAG, "Request to set the color of %s to h:%d s:%d v:%d", unique_name, h, s, v);
    return app_pwm_led_set_all_hsv(h, s, v);
}

esp_err_t app_driver_set_light_power(char *unique_name, bool status)
{
    ESP_LOGI(TAG, "Request to set the power of %s to %d", unique_name, status);
    return app_pwm_led_set_power(status);
}

esp_err_t app_driver_set_light_gpio(char *unique_name, int r, int g, int b)
{
    ESP_LOGI(TAG, "Request to set the GPIO of %s to r:%d g:%d b:%d,", unique_name, r, g, b);
    return app_pwm_led_change_io(r, g, b);
}

esp_err_t app_driver_set_light_voice(char *unique_name, char *voice)
{
    ESP_LOGI(TAG, "Request to set the voice instruction of %s to %s", unique_name, voice);
    parse_sr_config(voice);
    return ESP_OK;
}

esp_err_t app_driver_report_light_status(char *unique_name, bool status, int h, int s, int v)
{
    esp_rmaker_param_update_and_report(esp_rmaker_device_get_param_by_name(
                                           esp_rmaker_node_get_device_by_name(esp_rmaker_get_node(), unique_name), ESP_RMAKER_DEF_POWER_NAME), esp_rmaker_bool(status));

    esp_rmaker_param_update_and_report(esp_rmaker_device_get_param_by_name(
                                           esp_rmaker_node_get_device_by_name(esp_rmaker_get_node(), unique_name), ESP_RMAKER_DEF_HUE_NAME), esp_rmaker_int(h));

    esp_rmaker_param_update_and_report(esp_rmaker_device_get_param_by_name(
                                           esp_rmaker_node_get_device_by_name(esp_rmaker_get_node(), unique_name), ESP_RMAKER_DEF_SATURATION_NAME), esp_rmaker_int(s));

    esp_rmaker_param_update_and_report(esp_rmaker_device_get_param_by_name(
                                           esp_rmaker_node_get_device_by_name(esp_rmaker_get_node(), unique_name), ESP_RMAKER_DEF_BRIGHTNESS_NAME), esp_rmaker_int(v));
    return ESP_OK;
}

esp_err_t app_driver_fan_init(char *unique_name, int gpio, int power, char *voice)
{
    return ESP_OK;
}

esp_err_t app_driver_set_fan_power(char *unique_name, bool status)
{
    app_fan_set_power(status);
    return ESP_OK;
}

esp_err_t app_driver_set_fan_gpio(char *unique_name, int gpio, int active_level)
{
    ESP_LOGI(TAG, "Request to set the gpio of %s to %d", unique_name, gpio);
    return app_fan_change_io(gpio, active_level);
}

esp_err_t app_driver_set_fan_voice(char *unique_name, char *voice)
{
    ESP_LOGI(TAG, "Request to set the voice instruction of %s to %s, but the driver does not support it", unique_name, voice);

    // app_sr_update_voice_cmd(unique_name, voice);

    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t app_driver_report_fan_status(char *unique_name, bool status)
{
    esp_rmaker_param_update_and_report(esp_rmaker_device_get_param_by_name(
                                           esp_rmaker_node_get_device_by_name(esp_rmaker_get_node(), unique_name), ESP_RMAKER_DEF_POWER_NAME), esp_rmaker_bool(status));
    return ESP_OK;
}

esp_err_t app_driver_switch_init(char *unique_name, int gpio, int active_level, int power, char *voice)
{
    return ESP_OK;
}

esp_err_t app_driver_set_switch_power(char *unique_name, bool status)
{
    app_switch_set_power(status);
    return ESP_OK;
}

esp_err_t app_driver_set_switch_gpio(char *unique_name, int gpio, int active_level)
{
    ESP_LOGI(TAG, "Request to set the gpio of %s to %d, active_level: %d", unique_name, gpio, active_level);
    return app_switch_change_io(gpio, active_level);
}

esp_err_t app_driver_set_switch_voice(char *unique_name, char *voice)
{
    ESP_LOGI(TAG, "Request to set the voice instruction of %s to %s, but the driver does not support it", unique_name, voice);
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t app_driver_report_switch_status(char *unique_name, bool status)
{
    esp_rmaker_param_update_and_report(esp_rmaker_device_get_param_by_name(
                                           esp_rmaker_node_get_device_by_name(esp_rmaker_get_node(), unique_name), ESP_RMAKER_DEF_POWER_NAME), esp_rmaker_bool(status));
    return ESP_OK;
}
