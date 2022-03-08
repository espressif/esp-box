/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <string.h>

#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_types.h>

#include <device_driver.h>
#include <rmaker_devices.h>

static const char* TAG = "rmaker_devices";

esp_rmaker_device_t* esp_box_device;
TimerHandle_t report_delay_timer = NULL;

static void report_cb(xTimerHandle tmr)
{
    esp_rmaker_device_t* device = (esp_rmaker_device_t*)pvTimerGetTimerID(tmr);
    esp_rmaker_param_t* name = esp_rmaker_device_get_param_by_name((esp_rmaker_device_t*)device, ESP_RMAKER_DEF_POWER_NAME);
    esp_rmaker_param_val_t* val = esp_rmaker_param_get_val(name);
    esp_rmaker_param_update_and_report(name, esp_rmaker_bool(val->val.b));
}

static esp_err_t write_cb(const esp_rmaker_device_t* device, const esp_rmaker_param_t* param,
    const esp_rmaker_param_val_t val, void* priv_data, esp_rmaker_write_ctx_t* ctx)
{
    esp_err_t err = ESP_FAIL;
    char* type = esp_rmaker_device_get_type(device);
    char* device_name = esp_rmaker_device_get_name(device);
    char* param_name = esp_rmaker_param_get_name(param);

    if (strcmp(type, ESP_RMAKER_DEVICE_LIGHTBULB) == 0) {
        esp_box_light_cb_t* _cb = (esp_box_light_cb_t*)priv_data;
        esp_rmaker_param_val_t* gpio_r = esp_rmaker_param_get_val(esp_rmaker_device_get_param_by_name(device, ESP_RMAKER_GPIO_R_NAME));
        esp_rmaker_param_val_t* gpio_g = esp_rmaker_param_get_val(esp_rmaker_device_get_param_by_name(device, ESP_RMAKER_GPIO_G_NAME));
        esp_rmaker_param_val_t* gpio_b = esp_rmaker_param_get_val(esp_rmaker_device_get_param_by_name(device, ESP_RMAKER_GPIO_B_NAME));
        esp_rmaker_param_val_t* brightness = esp_rmaker_param_get_val(esp_rmaker_device_get_param_by_name(device, ESP_RMAKER_DEF_BRIGHTNESS_NAME));
        esp_rmaker_param_val_t* hue = esp_rmaker_param_get_val(esp_rmaker_device_get_param_by_name(device, ESP_RMAKER_DEF_HUE_NAME));
        esp_rmaker_param_val_t* saturation = esp_rmaker_param_get_val(esp_rmaker_device_get_param_by_name(device, ESP_RMAKER_DEF_SATURATION_NAME));

        if (strcmp(param_name, ESP_RMAKER_GPIO_R_NAME) == 0) {
            if (_cb->gpio_cb) {
                err = _cb->gpio_cb(device_name, val.val.i, gpio_g->val.i, gpio_b->val.i);
            }
        } else if (strcmp(param_name, ESP_RMAKER_GPIO_G_NAME) == 0) {
            if (_cb->gpio_cb) {
                err = _cb->gpio_cb(device_name, gpio_r->val.i, val.val.i, gpio_b->val.i);
            }
        } else if (strcmp(param_name, ESP_RMAKER_GPIO_B_NAME) == 0) {
            if (_cb->gpio_cb) {
                err = _cb->gpio_cb(device_name, gpio_r->val.i, gpio_g->val.i, val.val.i);
            }
        } else if (strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
            if (_cb->status_cb) {
                err = _cb->status_cb(device_name, val.val.b);
            }
        } else if (strcmp(param_name, ESP_RMAKER_DEF_BRIGHTNESS_NAME) == 0) {
            if (_cb->color_cb) {
                err = _cb->color_cb(device_name, hue->val.i, saturation->val.i, val.val.i);
            }
        } else if (strcmp(param_name, ESP_RMAKER_DEF_HUE_NAME) == 0) {
            if (_cb->color_cb) {
                err = _cb->color_cb(device_name, val.val.i, saturation->val.i, brightness->val.i);
            }
        } else if (strcmp(param_name, ESP_RMAKER_DEF_SATURATION_NAME) == 0) {
            if (_cb->color_cb) {
                err = _cb->color_cb(device_name, hue->val.i, val.val.i, brightness->val.i);
            }
        } else if (strcmp(param_name, ESP_RMAKER_VOICE_NAME) == 0) {
            if (_cb->voice_cb) {
                err = _cb->voice_cb(device_name, val.val.s);
            }
        }

    } else if (strcmp(type, ESP_RMAKER_DEVICE_SWITCH) == 0) {
        esp_box_switch_cb_t* _cb = (esp_box_switch_cb_t*)priv_data;
        esp_rmaker_param_val_t* gpio = esp_rmaker_param_get_val(esp_rmaker_device_get_param_by_name(device, ESP_RMAKER_GPIO_NAME));
        esp_rmaker_param_val_t* active_level = esp_rmaker_param_get_val(esp_rmaker_device_get_param_by_name(device, ESP_RMAKER_ACTIVE_LEVEL_NAME));

        if (strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
            if (_cb->status_cb) {
                err = _cb->status_cb(device_name, val.val.b);
            }
        } else if (strcmp(param_name, ESP_RMAKER_GPIO_NAME) == 0) {
            if (_cb->gpio_cb) {
                _cb->gpio_cb(device_name, val.val.i, active_level->val.i);
            }
        } else if (strcmp(param_name, ESP_RMAKER_VOICE_NAME) == 0) {
            if (_cb->voice_cb) {
                err = _cb->voice_cb(device_name, val.val.s);
            }
        } else if (strcmp(param_name, ESP_RMAKER_ACTIVE_LEVEL_NAME) == 0) {
            if (_cb->gpio_cb) {
                _cb->gpio_cb(device_name, gpio->val.i, val.val.i);
            }
        }

    } else if (strcmp(type, ESP_RMAKER_DEVICE_FAN) == 0) {
        esp_box_fan_cb_t* _cb = (esp_box_fan_cb_t*)priv_data;

        if (strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
            if (_cb->status_cb) {
                err = _cb->status_cb(device_name, val.val.b);
            }
        } else if (strcmp(param_name, ESP_RMAKER_GPIO_NAME) == 0) {
            if (_cb->gpio_cb) {
                _cb->gpio_cb(device_name, val.val.i, -1);
            }
        } else if (strcmp(param_name, ESP_RMAKER_VOICE_NAME) == 0) {
            if (_cb->voice_cb) {
                err = _cb->voice_cb(device_name, val.val.s);
            }
        }
    } else {
        ESP_LOGW(TAG, "Unsupported device name: %s, device type: %s from %s", device_name, type, esp_rmaker_device_cb_src_to_str(ctx->src));
    }

    if (err == ESP_OK) {
        esp_rmaker_param_update(param, val);
        if (!report_delay_timer) {
            report_delay_timer = xTimerCreate("report_tmr", 1000 / portTICK_PERIOD_MS, pdFALSE, (void*)device, report_cb);
        }
        xTimerReset(report_delay_timer, 0);
    }
    return ESP_OK;
}

void esp_box_light_device_create(esp_box_light_param_t param_list, esp_box_light_cb_t* cb)
{
    esp_rmaker_device_t* device = esp_rmaker_device_create(param_list.unique_name, ESP_RMAKER_DEVICE_LIGHTBULB, cb);
    if (device) {
        esp_rmaker_device_add_cb(device, write_cb, NULL);
        /**
         * @brief Construct standard parameter
         *
         */
        esp_rmaker_device_add_param(device, esp_rmaker_name_param_create(ESP_RMAKER_DEF_NAME_PARAM, param_list.name));
        esp_rmaker_param_t* primary = esp_rmaker_power_param_create(ESP_RMAKER_DEF_POWER_NAME, param_list.power);
        esp_rmaker_device_add_param(device, primary);
        esp_rmaker_device_assign_primary_param(device, primary);

        esp_rmaker_device_add_param(device, esp_rmaker_brightness_param_create(ESP_RMAKER_DEF_BRIGHTNESS_NAME, param_list.brightness));
        esp_rmaker_device_add_param(device, esp_rmaker_hue_param_create(ESP_RMAKER_DEF_HUE_NAME, param_list.hue));
        esp_rmaker_device_add_param(device, esp_rmaker_saturation_param_create(ESP_RMAKER_DEF_SATURATION_NAME, param_list.saturation));

        /**
         * @brief Construct custom parameter
         *
         */
        esp_rmaker_param_t* param = esp_rmaker_param_create(ESP_RMAKER_GPIO_R_NAME, ESP_RMAKER_PARAM_TYPE_GPIO, esp_rmaker_int(param_list.gpio_r), PROP_FLAG_READ | PROP_FLAG_WRITE);
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_TEXT);
        esp_rmaker_param_add_bounds(param, esp_rmaker_int(0), esp_rmaker_int(100), esp_rmaker_int(1));
        esp_rmaker_device_add_param(device, param);

        param = esp_rmaker_param_create(ESP_RMAKER_GPIO_G_NAME, ESP_RMAKER_PARAM_TYPE_GPIO, esp_rmaker_int(param_list.gpio_g), PROP_FLAG_READ | PROP_FLAG_WRITE);
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_TEXT);
        esp_rmaker_param_add_bounds(param, esp_rmaker_int(0), esp_rmaker_int(100), esp_rmaker_int(1));
        esp_rmaker_device_add_param(device, param);

        param = esp_rmaker_param_create(ESP_RMAKER_GPIO_B_NAME, ESP_RMAKER_PARAM_TYPE_GPIO, esp_rmaker_int(param_list.gpio_b), PROP_FLAG_READ | PROP_FLAG_WRITE);
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_TEXT);
        esp_rmaker_param_add_bounds(param, esp_rmaker_int(0), esp_rmaker_int(100), esp_rmaker_int(1));
        esp_rmaker_device_add_param(device, param);

        param = esp_rmaker_param_create(ESP_RMAKER_VOICE_NAME, ESP_RMAKER_PARAM_TYPE_VOICE, esp_rmaker_array(param_list.voice_cmd), PROP_FLAG_READ | PROP_FLAG_WRITE);
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_TEXT);
        esp_rmaker_device_add_param(device, param);

        esp_rmaker_node_add_device(esp_rmaker_get_node(), device);
    }
}

void esp_box_switch_device_create(esp_box_switch_param_t param_list, esp_box_switch_cb_t* cb)
{
    esp_rmaker_device_t* device = esp_rmaker_device_create(param_list.unique_name, ESP_RMAKER_DEVICE_SWITCH, cb);
    if (device) {
        esp_rmaker_device_add_cb(device, write_cb, NULL);
        /**
         * @brief Construct standard parameter
         *
         */
        esp_rmaker_device_add_param(device, esp_rmaker_name_param_create(ESP_RMAKER_DEF_NAME_PARAM, param_list.name));
        esp_rmaker_param_t* primary = esp_rmaker_power_param_create(ESP_RMAKER_DEF_POWER_NAME, param_list.power);
        esp_rmaker_device_add_param(device, primary);
        esp_rmaker_device_assign_primary_param(device, primary);

        /**
         * @brief Construct custom parameter
         *
         */
        esp_rmaker_param_t* param = esp_rmaker_param_create(ESP_RMAKER_GPIO_NAME, ESP_RMAKER_PARAM_TYPE_GPIO, esp_rmaker_int(param_list.gpio), PROP_FLAG_READ | PROP_FLAG_WRITE);
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_TEXT);
        esp_rmaker_param_add_bounds(param, esp_rmaker_int(0), esp_rmaker_int(100), esp_rmaker_int(1));
        esp_rmaker_device_add_param(device, param);

        param = esp_rmaker_param_create(ESP_RMAKER_ACTIVE_LEVEL_NAME, ESP_RMAKER_PARAM_TYPE_ACTIVE_LEVEL, esp_rmaker_int(param_list.active_level), PROP_FLAG_READ | PROP_FLAG_WRITE);
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_TEXT);
        esp_rmaker_param_add_bounds(param, esp_rmaker_int(-1), esp_rmaker_int(1), esp_rmaker_int(1));
        esp_rmaker_device_add_param(device, param);

        param = esp_rmaker_param_create(ESP_RMAKER_VOICE_NAME, ESP_RMAKER_PARAM_TYPE_VOICE, esp_rmaker_array(param_list.voice_cmd), PROP_FLAG_READ | PROP_FLAG_WRITE);
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_TEXT);
        esp_rmaker_device_add_param(device, param);

        esp_rmaker_node_add_device(esp_rmaker_get_node(), device);
    }
}

void esp_box_fan_device_create(esp_box_fan_param_t param_list, esp_box_fan_cb_t* cb)
{
    esp_rmaker_device_t* device = esp_rmaker_device_create(param_list.unique_name, ESP_RMAKER_DEVICE_FAN, cb);
    if (device) {
        esp_rmaker_device_add_cb(device, write_cb, NULL);
        /**
         * @brief Construct standard parameter
         *
         */
        esp_rmaker_device_add_param(device, esp_rmaker_name_param_create(ESP_RMAKER_DEF_NAME_PARAM, param_list.name));
        esp_rmaker_param_t* primary = esp_rmaker_power_param_create(ESP_RMAKER_DEF_POWER_NAME, param_list.power);
        esp_rmaker_device_add_param(device, primary);
        esp_rmaker_device_assign_primary_param(device, primary);

        /**
         * @brief Construct custom parameter
         *
         */
        esp_rmaker_param_t* param = esp_rmaker_param_create(ESP_RMAKER_GPIO_NAME, ESP_RMAKER_PARAM_TYPE_GPIO, esp_rmaker_int(param_list.gpio), PROP_FLAG_READ | PROP_FLAG_WRITE);
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_TEXT);
        esp_rmaker_param_add_bounds(param, esp_rmaker_int(0), esp_rmaker_int(100), esp_rmaker_int(1));
        esp_rmaker_device_add_param(device, param);

        param = esp_rmaker_param_create(ESP_RMAKER_VOICE_NAME, ESP_RMAKER_PARAM_TYPE_VOICE, esp_rmaker_array(param_list.voice_cmd), PROP_FLAG_READ | PROP_FLAG_WRITE);
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_TEXT);
        esp_rmaker_device_add_param(device, param);

        esp_rmaker_node_add_device(esp_rmaker_get_node(), device);
    }
}
