// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <sdkconfig.h>
#include <string.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <json_generator.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_utils.h>
#include "esp_rmaker_internal.h"
#include "esp_rmaker_mqtt.h"
#include "esp_rmaker_mqtt_topics.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include <esp_app_desc.h>
#endif

#define NODE_CONFIG_TOPIC_SUFFIX        "config"

static const char *TAG = "esp_rmaker_node_config";
static esp_err_t esp_rmaker_report_info(json_gen_str_t *jptr)
{
    /* TODO: Error handling */
    esp_rmaker_node_info_t *info = esp_rmaker_node_get_info(esp_rmaker_get_node());
    json_gen_obj_set_string(jptr, "node_id", esp_rmaker_get_node_id());
    json_gen_obj_set_string(jptr, "config_version", ESP_RMAKER_CONFIG_VERSION);
    json_gen_push_object(jptr, "info");
    json_gen_obj_set_string(jptr, "name",  info->name);
    json_gen_obj_set_string(jptr, "fw_version",  info->fw_version);
    json_gen_obj_set_string(jptr, "type",  info->type);
    if (info->subtype) {
        json_gen_obj_set_string(jptr, "subtype",  info->subtype);
    }
    json_gen_obj_set_string(jptr, "model",  info->model);
    const esp_app_desc_t *app_desc;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    app_desc = esp_app_get_description();
#else
    app_desc = esp_ota_get_app_description();
#endif
    json_gen_obj_set_string(jptr, "project_name", (char *)app_desc->project_name);
    json_gen_obj_set_string(jptr, "platform", CONFIG_IDF_TARGET);
    json_gen_pop_object(jptr);
    return ESP_OK;
}

static void esp_rmaker_report_attribute(esp_rmaker_attr_t *attr, json_gen_str_t *jptr)
{
    json_gen_start_object(jptr);
    json_gen_obj_set_string(jptr, "name", attr->name);
    json_gen_obj_set_string(jptr, "value", attr->value);
    json_gen_end_object(jptr);
}

static esp_err_t esp_rmaker_report_node_attributes(json_gen_str_t *jptr)
{
    esp_rmaker_attr_t *attr = esp_rmaker_node_get_first_attribute(esp_rmaker_get_node());
    if (!attr) {
        return ESP_OK;
    }
    json_gen_push_array(jptr, "attributes");
    while (attr) {
        esp_rmaker_report_attribute(attr, jptr);
        attr = attr->next;
    }
    json_gen_pop_array(jptr);
    return ESP_OK;
}

esp_err_t esp_rmaker_report_value(const esp_rmaker_param_val_t *val, char *key, json_gen_str_t *jptr)
{
    if (!key || !jptr) {
        return ESP_FAIL;
    }
    if (!val) {
        json_gen_obj_set_null(jptr, key);
        return ESP_OK;
    }
    switch (val->type) {
        case RMAKER_VAL_TYPE_BOOLEAN:
            json_gen_obj_set_bool(jptr, key, val->val.b);
            break;
        case RMAKER_VAL_TYPE_INTEGER:
            json_gen_obj_set_int(jptr, key, val->val.i);
            break;
        case RMAKER_VAL_TYPE_FLOAT:
            json_gen_obj_set_float(jptr, key, val->val.f);
            break;
        case RMAKER_VAL_TYPE_STRING:
            json_gen_obj_set_string(jptr, key, val->val.s);
            break;
        case RMAKER_VAL_TYPE_OBJECT:
            json_gen_push_object_str(jptr, key, val->val.s);
            break;
        case RMAKER_VAL_TYPE_ARRAY:
            json_gen_push_array_str(jptr, key, val->val.s);
            break;
        default:
            break;
    }
    return ESP_OK;
}

esp_err_t esp_rmaker_report_data_type(esp_rmaker_val_type_t type, char *data_type_key, json_gen_str_t *jptr)
{
    switch (type) {
        case RMAKER_VAL_TYPE_BOOLEAN:
            json_gen_obj_set_string(jptr, data_type_key, "bool");
            break;
        case RMAKER_VAL_TYPE_INTEGER:
            json_gen_obj_set_string(jptr, data_type_key, "int");
            break;
        case RMAKER_VAL_TYPE_FLOAT:
            json_gen_obj_set_string(jptr, data_type_key, "float");
            break;
        case RMAKER_VAL_TYPE_STRING:
            json_gen_obj_set_string(jptr, data_type_key, "string");
            break;
        case RMAKER_VAL_TYPE_OBJECT:
            json_gen_obj_set_string(jptr, data_type_key, "object");
            break;
        case RMAKER_VAL_TYPE_ARRAY:
            json_gen_obj_set_string(jptr, data_type_key, "array");
            break;
        default:
            json_gen_obj_set_string(jptr, data_type_key, "invalid");
            break;
    }
    return ESP_OK;
}

static esp_err_t esp_rmaker_report_param_config(_esp_rmaker_param_t *param, json_gen_str_t *jptr)
{
    json_gen_start_object(jptr);
    if (param->name) {
        json_gen_obj_set_string(jptr, "name", param->name);
    }
    if (param->type) {
        json_gen_obj_set_string(jptr, "type", param->type);
    }
    esp_rmaker_report_data_type(param->val.type, "data_type", jptr);
    json_gen_push_array(jptr, "properties");
    if (param->prop_flags & PROP_FLAG_READ) {
        json_gen_arr_set_string(jptr, "read");
    }
    if (param->prop_flags & PROP_FLAG_WRITE) {
        json_gen_arr_set_string(jptr, "write");
    }
    if (param->prop_flags & PROP_FLAG_TIME_SERIES) {
        json_gen_arr_set_string(jptr, "time_series");
    }
    json_gen_pop_array(jptr);
    if (param->bounds) {
        json_gen_push_object(jptr, "bounds");
        esp_rmaker_report_value(&param->bounds->min, "min", jptr);
        esp_rmaker_report_value(&param->bounds->max, "max", jptr);
        if (param->bounds->step.val.i) {
            esp_rmaker_report_value(&param->bounds->step, "step", jptr);
        }
        json_gen_pop_object(jptr);
    }
    if (param->valid_str_list) {
        json_gen_push_array(jptr, "valid_strs");
        for (int i = 0; i < param->valid_str_list->str_list_cnt; i++) {
            json_gen_arr_set_string(jptr, (char *)param->valid_str_list->str_list[i]);
        }
        json_gen_pop_array(jptr);
    }
    if (param->ui_type) {
        json_gen_obj_set_string(jptr, "ui_type", param->ui_type);
    }
    json_gen_end_object(jptr);
    return ESP_OK;
}

static esp_err_t esp_rmaker_report_devices_or_services(json_gen_str_t *jptr, char *key)
{
    _esp_rmaker_device_t *device = esp_rmaker_node_get_first_device(esp_rmaker_get_node());
    if (!device) {
        return ESP_OK;
    }
    bool is_service = false;
    if (strcmp(key, "services") == 0) {
        is_service = true;
    }
    json_gen_push_array(jptr, key);
    while (device) {
        if (device->is_service == is_service) {
            json_gen_start_object(jptr);
            json_gen_obj_set_string(jptr, "name", device->name);
            if (device->type) {
                json_gen_obj_set_string(jptr, "type", device->type);
            }
            if (device->subtype) {
                json_gen_obj_set_string(jptr, "subtype", device->subtype);
            }
            if (device->model) {
                json_gen_obj_set_string(jptr, "model", device->model);
            }
            if (device->attributes) {
                json_gen_push_array(jptr, "attributes");
                esp_rmaker_attr_t *attr = device->attributes;
                while (attr) {
                    esp_rmaker_report_attribute(attr, jptr);
                    attr = attr->next;
                }
                json_gen_pop_array(jptr);
            }
            if (device->primary) {
                json_gen_obj_set_string(jptr, "primary", device->primary->name);
            }
            if (device->params) {
                json_gen_push_array(jptr, "params");
                _esp_rmaker_param_t *param = device->params;
                while (param) {
                    esp_rmaker_report_param_config(param, jptr);
                    param = param->next;
                }
                json_gen_pop_array(jptr);
            }
            json_gen_end_object(jptr);
        }
        device = device->next;
    }
    json_gen_pop_array(jptr);
    return ESP_OK;
}

int __esp_rmaker_get_node_config(char *buf, size_t buf_size)
{
    json_gen_str_t jstr;
    json_gen_str_start(&jstr, buf, buf_size, NULL, NULL);
    json_gen_start_object(&jstr);
    esp_rmaker_report_info(&jstr);
    esp_rmaker_report_node_attributes(&jstr);
    esp_rmaker_report_devices_or_services(&jstr, "devices");
    esp_rmaker_report_devices_or_services(&jstr, "services");
    if (json_gen_end_object(&jstr) < 0) {
        return -1;
    }
    return json_gen_str_end(&jstr);
}

char *esp_rmaker_get_node_config(void)
{
    /* Setting buffer to NULL and size to 0 just to get the required buffer size */
    int req_size = __esp_rmaker_get_node_config(NULL, 0);
    if (req_size < 0) {
        ESP_LOGE(TAG, "Failed to get required size for Node config JSON.");
        return NULL;
    }
    char *node_config = MEM_CALLOC_EXTRAM(1, req_size);
    if (!node_config) {
        ESP_LOGE(TAG, "Failed to allocate %d bytes for node config", req_size);
        return NULL;
    }
    if (__esp_rmaker_get_node_config(node_config, req_size) < 0) {
        free(node_config);
        ESP_LOGE(TAG, "Failed to generate Node config JSON.");
        return NULL;
    }
    ESP_LOGD(TAG, "Generated Node config of length %d", req_size);
    return node_config;
}

esp_err_t esp_rmaker_report_node_config()
{
    char *publish_payload = esp_rmaker_get_node_config();
    if (!publish_payload) {
        ESP_LOGE(TAG, "Could not get node configuration for reporting to cloud");
        return ESP_FAIL;
    }
    char publish_topic[MQTT_TOPIC_BUFFER_SIZE];
    esp_rmaker_create_mqtt_topic(publish_topic, MQTT_TOPIC_BUFFER_SIZE, NODE_CONFIG_TOPIC_SUFFIX, NODE_CONFIG_TOPIC_RULE);
    ESP_LOGI(TAG, "Reporting Node Configuration of length %d bytes.", strlen(publish_payload));
    ESP_LOGD(TAG, "%s", publish_payload);
    esp_err_t ret = esp_rmaker_mqtt_publish(publish_topic, publish_payload, strlen(publish_payload),
                        RMAKER_MQTT_QOS1, NULL);
    free(publish_payload);
    return ret;
}
