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
#include <time.h>
#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include <nvs.h>

#include <json_parser.h>
#include <json_generator.h>

#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_mqtt.h>
#include <esp_rmaker_utils.h>
#include "esp_rmaker_mqtt_topics.h"
#include "esp_rmaker_internal.h"

#define TS_DATA_VERSION                         "2021-09-13"

#define ESP_RMAKER_ALERT_KEY                    "esp.alert.str"

#define RMAKER_PARAMS_SIZE_MARGIN       50 /* To accommodate for changes in param values while creating JSON */
#define RMAKER_ALERT_STR_MARGIN         25 /* To accommodate rest of the alert payload {"esp.alert.str":""}  */
#define MAX_TS_DATA_PARAM_NAME          66 /* Time series data param name is of the format <device_name>.<param_name> */

static size_t max_node_params_size = CONFIG_ESP_RMAKER_MAX_PARAM_DATA_SIZE;
/* This buffer will be allocated once and will be reused for all param updates.
 * It may be reallocated if the params size becomes too large */

static char publish_topic[MQTT_TOPIC_BUFFER_SIZE];
static bool esp_rmaker_params_mqtt_init_done;

static const char *TAG = "esp_rmaker_param";


static const char *cb_srcs[ESP_RMAKER_REQ_SRC_MAX] = {
    [ESP_RMAKER_REQ_SRC_INIT] = "Init",
    [ESP_RMAKER_REQ_SRC_CLOUD] = "Cloud",
    [ESP_RMAKER_REQ_SRC_SCHEDULE] = "Schedule",
    [ESP_RMAKER_REQ_SRC_SCENE_ACTIVATE] = "Scene Activate",
    [ESP_RMAKER_REQ_SRC_SCENE_DEACTIVATE] = "Scene Deactivate",
    [ESP_RMAKER_REQ_SRC_LOCAL] = "Local",
};

const char *esp_rmaker_device_cb_src_to_str(esp_rmaker_req_src_t src)
{
    if ((src >= 0) && (src < ESP_RMAKER_REQ_SRC_MAX)) {
        return cb_srcs[src];
    }
    return NULL;
}

esp_rmaker_param_val_t esp_rmaker_bool(bool val)
{
    esp_rmaker_param_val_t param_val = {
        .type = RMAKER_VAL_TYPE_BOOLEAN,
        .val.b = val
    };
    return param_val;
}

esp_rmaker_param_val_t esp_rmaker_int(int val)
{
    esp_rmaker_param_val_t param_val = {
        .type = RMAKER_VAL_TYPE_INTEGER,
        .val.i = val
    };
    return param_val;
}

esp_rmaker_param_val_t esp_rmaker_float(float val)
{
    esp_rmaker_param_val_t param_val = {
        .type = RMAKER_VAL_TYPE_FLOAT,
        .val.f = val
    };
    return param_val;
}

esp_rmaker_param_val_t esp_rmaker_str(const char *val)
{
    esp_rmaker_param_val_t param_val = {
        .type = RMAKER_VAL_TYPE_STRING,
        .val.s = (char *)val
    };
    return param_val;
}

esp_rmaker_param_val_t esp_rmaker_obj(const char *val)
{
    esp_rmaker_param_val_t param_val = {
        .type = RMAKER_VAL_TYPE_OBJECT,
        .val.s = (char *)val
    };
    return param_val;
}

esp_rmaker_param_val_t esp_rmaker_array(const char *val)
{
    esp_rmaker_param_val_t param_val = {
        .type = RMAKER_VAL_TYPE_ARRAY,
        .val.s = (char *)val
    };
    return param_val;
}

static esp_err_t esp_rmaker_populate_params(char *buf, size_t *buf_len, uint8_t flags, bool reset_flags)
{
    esp_err_t err = ESP_OK;
    json_gen_str_t jstr;
    json_gen_str_start(&jstr, buf, *buf_len, NULL, NULL);
    json_gen_start_object(&jstr);
    _esp_rmaker_device_t *device = esp_rmaker_node_get_first_device(esp_rmaker_get_node());
    while (device) {
        bool device_added = false;
        _esp_rmaker_param_t *param = device->params;
        while (param) {
            if (!flags || (param->flags & flags)) {
                if (!device_added) {
                    json_gen_push_object(&jstr, device->name);
                    device_added = true;
                }
                esp_rmaker_report_value(&param->val, param->name, &jstr);
            }
            param = param->next;
        }
        if (device_added) {
            json_gen_pop_object(&jstr);
        }
        device = device->next;
    }
    if (json_gen_end_object(&jstr) < 0) {
        err = ESP_ERR_NO_MEM;
    }
    /* Resetting the flags after creating the JSON in order to handle cases wherein
     * memory has been insufficient and this same function would have to be called
     * again with a larger buffer.
     */
    if (err == ESP_OK) {
        device = esp_rmaker_node_get_first_device(esp_rmaker_get_node());
        while (device) {
            _esp_rmaker_param_t *param = device->params;
            while (param) {
                if (reset_flags) {
                    param->flags &= ~flags;
                }
                param = param->next;
            }
            device = device->next;
        }
    }
    *buf_len = json_gen_str_end(&jstr);
    return err;
}

/* This function does not use the node_params_buf since this is for external use
 * and we do not want __esp_rmaker_allocate_and_populate_params to overwrite
 * the buffer.
 */
char *esp_rmaker_get_node_params(void)
{
    size_t req_size = 0;
    /* Passing NULL pointer to find the required buffer size */
    esp_err_t err = esp_rmaker_populate_params(NULL, &req_size, 0, false);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get required size for Node params JSON.");
        return NULL;
    }
    /* Keeping some margin just in case some param value changes in between */
    req_size += RMAKER_PARAMS_SIZE_MARGIN;
    char *node_params = MEM_CALLOC_EXTRAM(1, req_size);
    if (!node_params) {
        ESP_LOGE(TAG, "Failed to allocate %d bytes for Node params.", req_size);
        return NULL;
    }
    err = esp_rmaker_populate_params(node_params, &req_size, 0, false);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to generate Node params JSON.");
        free(node_params);
        return NULL;
    }
    return node_params;
}

static char * esp_rmaker_param_get_buf(size_t size)
{
    static char *s_node_params_buf;
    static size_t s_param_buf_size;
    /* If received size is 0, we will just return the pointer to the buffer */
    if (size == 0) {
        return s_node_params_buf;
    }
    /* If s_param_buf_size is not 0, it means that a buffer was already allocated.
     * If the requested "size" is not the same as the existing s_param_buf_size,
     * we need to free existing buffer.
     */
    if ((s_param_buf_size != 0) && (s_param_buf_size != size)) {
        ESP_LOGD(TAG, "Freeing s_node_params_buf of size %d", s_param_buf_size);
        free(s_node_params_buf);
        s_node_params_buf = NULL;
    }
    if (!s_node_params_buf) {
        ESP_LOGD(TAG, "Allocating s_node_params_buf for size %d.", size);
        s_node_params_buf = MEM_CALLOC_EXTRAM(1, size);
        if (!s_node_params_buf) {
            ESP_LOGE(TAG, "Failed to allocate %d bytes for Node params.", size);
            s_param_buf_size = 0;
            return NULL;
        }
        s_param_buf_size = size;
    }
    return s_node_params_buf;
}

static esp_err_t esp_rmaker_allocate_and_populate_params(uint8_t flags, bool reset_flags)
{
    char *node_params_buf = esp_rmaker_param_get_buf(max_node_params_size);
    if (!node_params_buf) {
        return ESP_ERR_NO_MEM;
    }
    /* Typically, max_node_params_size should be sufficient for the parameters */
    size_t req_size = max_node_params_size;
    esp_err_t err = esp_rmaker_populate_params(node_params_buf, &req_size, flags, reset_flags);
    /* If the max_node_params_size was insufficient, we will re-allocate new buffer */
    if (err == ESP_ERR_NO_MEM) {
        ESP_LOGW(TAG, "%d bytes not sufficient for Node params. Reallocating %d bytes.",
                max_node_params_size, req_size + RMAKER_PARAMS_SIZE_MARGIN);
        max_node_params_size = req_size + RMAKER_PARAMS_SIZE_MARGIN; /* Keeping some margin since paramater value size can change */
        node_params_buf = esp_rmaker_param_get_buf(max_node_params_size);
        if (!node_params_buf) {
            return ESP_ERR_NO_MEM;
        }
        req_size = max_node_params_size;
        err = esp_rmaker_populate_params(node_params_buf, &req_size, flags, reset_flags);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to populate node parameters.");
        }
    }
    return err;
}

static esp_err_t esp_rmaker_report_param_internal(uint8_t flags)
{
    esp_err_t err = esp_rmaker_allocate_and_populate_params(flags, true);
    if (err == ESP_OK) {
        /* Just checking if there are indeed any params to report by comparing with a decent enough
         * length as even the smallest possible data, Eg. '{"d":{"p":0}}' will be > 10 bytes.
         */
        char *node_params_buf = esp_rmaker_param_get_buf(0);
        if (strlen(node_params_buf) > 10) {
            if (flags == RMAKER_PARAM_FLAG_VALUE_CHANGE) {
                esp_rmaker_create_mqtt_topic(publish_topic, sizeof(publish_topic), NODE_PARAMS_LOCAL_TOPIC_SUFFIX, NODE_PARAMS_LOCAL_TOPIC_RULE);
                ESP_LOGI(TAG, "Reporting params: %s", node_params_buf);
            } else if (flags == RMAKER_PARAM_FLAG_VALUE_NOTIFY) {
                esp_rmaker_create_mqtt_topic(publish_topic, sizeof(publish_topic), NODE_PARAMS_ALERT_TOPIC_SUFFIX, NODE_PARAMS_ALERT_TOPIC_RULE);
                ESP_LOGI(TAG, "Notifying params: %s", node_params_buf);
            } else {
                return ESP_FAIL;
            }
            if (esp_rmaker_params_mqtt_init_done) {
                esp_rmaker_mqtt_publish(publish_topic, node_params_buf, strlen(node_params_buf), RMAKER_MQTT_QOS1, NULL);
            } else {
                ESP_LOGW(TAG, "Not reporting params since params mqtt not initialized yet.");
            }
        }
        return ESP_OK;
    }
    return err;
}


static esp_err_t esp_rmaker_device_set_params(_esp_rmaker_device_t *device, jparse_ctx_t *jptr, esp_rmaker_req_src_t src)
{
    _esp_rmaker_param_t *param = device->params;
    while (param) {
        esp_rmaker_param_val_t new_val = {0};
        bool param_found = false;
        switch(param->val.type) {
            case RMAKER_VAL_TYPE_BOOLEAN:
                if (json_obj_get_bool(jptr, param->name, &new_val.val.b) == 0) {
                    new_val.type = RMAKER_VAL_TYPE_BOOLEAN;
                    param_found = true;
                }
                break;
            case RMAKER_VAL_TYPE_INTEGER:
                if (json_obj_get_int(jptr, param->name, &new_val.val.i) == 0) {
                    new_val.type = RMAKER_VAL_TYPE_INTEGER;
                    param_found = true;
                }
                break;
            case RMAKER_VAL_TYPE_FLOAT:
                if (json_obj_get_float(jptr, param->name, &new_val.val.f) == 0) {
                    new_val.type = RMAKER_VAL_TYPE_FLOAT;
                    param_found = true;
                }
                break;
            case RMAKER_VAL_TYPE_STRING: {
                int val_size = 0;
                if (json_obj_get_strlen(jptr, param->name, &val_size) == 0) {
                    val_size++; /* For NULL termination */
                    new_val.val.s = MEM_CALLOC_EXTRAM(1, val_size);
                    if (!new_val.val.s) {
                        return ESP_ERR_NO_MEM;
                    }
                    json_obj_get_string(jptr, param->name, new_val.val.s, val_size);
                    new_val.type = RMAKER_VAL_TYPE_STRING;
                    param_found = true;
                }
                break;
            }
            case RMAKER_VAL_TYPE_OBJECT: {
                int val_size = 0;
                if (json_obj_get_object_strlen(jptr, param->name, &val_size) == 0) {
                    val_size++; /* For NULL termination */
                    new_val.val.s = MEM_CALLOC_EXTRAM(1, val_size);
                    if (!new_val.val.s) {
                        return ESP_ERR_NO_MEM;
                    }
                    json_obj_get_object_str(jptr, param->name, new_val.val.s, val_size);
                    new_val.type = RMAKER_VAL_TYPE_OBJECT;
                    param_found = true;
                }
                break;
            }
            case RMAKER_VAL_TYPE_ARRAY: {
                int val_size = 0;
                if (json_obj_get_array_strlen(jptr, param->name, &val_size) == 0) {
                    val_size++; /* For NULL termination */
                    new_val.val.s = MEM_CALLOC_EXTRAM(1, val_size);
                    if (!new_val.val.s) {
                        return ESP_ERR_NO_MEM;
                    }
                    json_obj_get_array_str(jptr, param->name, new_val.val.s, val_size);
                    new_val.type = RMAKER_VAL_TYPE_ARRAY;
                    param_found = true;
                }
                break;
            }
            default:
                break;
        }
        if (param_found) {
            /* Special handling for ESP_RMAKER_PARAM_NAME. Just update the name instead
             * of calling the registered callback.
             */
            if (param->type && (strcmp(param->type, ESP_RMAKER_PARAM_NAME) == 0)) {
#ifdef CONFIG_RMAKER_NAME_PARAM_CB
                if (device->write_cb) {
                    esp_rmaker_write_ctx_t ctx = {
                        .src = src,
                    };
                    device->write_cb((esp_rmaker_device_t *)device, (esp_rmaker_param_t *)param,
                                new_val, device->priv_data, &ctx);
                } else {
                    esp_rmaker_param_update_and_report((esp_rmaker_param_t *)param, new_val);
                }
#else
                esp_rmaker_param_update_and_report((esp_rmaker_param_t *)param, new_val);
#endif
            } else if (device->write_cb) {
                esp_rmaker_write_ctx_t ctx = {
                    .src = src,
                };
                if (device->write_cb((esp_rmaker_device_t *)device, (esp_rmaker_param_t *)param,
                            new_val, device->priv_data, &ctx) != ESP_OK) {
                    ESP_LOGE(TAG, "Remote update to param %s - %s failed", device->name, param->name);
                }
            }
            if ((new_val.type == RMAKER_VAL_TYPE_STRING) || (new_val.type == RMAKER_VAL_TYPE_OBJECT ||
                        (new_val.type == RMAKER_VAL_TYPE_ARRAY))) {
                if (new_val.val.s) {
                    free(new_val.val.s);
                }
            }
        }
        param = param->next;
    }
    return ESP_OK;
}

esp_err_t esp_rmaker_handle_set_params(char *data, size_t data_len, esp_rmaker_req_src_t src)
{
    ESP_LOGI(TAG, "Received params: %.*s", data_len, data);
    jparse_ctx_t jctx;
    if (json_parse_start(&jctx, data, data_len) != 0) {
        return ESP_FAIL;
    }
    _esp_rmaker_device_t *device = esp_rmaker_node_get_first_device(esp_rmaker_get_node());
    while (device) {
        if (json_obj_get_object(&jctx, device->name) == 0) {
            esp_rmaker_device_set_params(device, &jctx, src);
            json_obj_leave_object(&jctx);
        }
        device = device->next;
    }
    json_parse_end(&jctx);
    return ESP_OK;
}

static void esp_rmaker_set_params_callback(const char *topic, void *payload, size_t payload_len, void *priv_data)
{
    esp_rmaker_handle_set_params((char *)payload, payload_len, ESP_RMAKER_REQ_SRC_CLOUD);
}

static esp_err_t esp_rmaker_register_for_set_params(void)
{
    char subscribe_topic[MQTT_TOPIC_BUFFER_SIZE];
    snprintf(subscribe_topic, sizeof(subscribe_topic), "node/%s/%s",
                esp_rmaker_get_node_id(), NODE_PARAMS_REMOTE_TOPIC_SUFFIX);
    esp_err_t err = esp_rmaker_mqtt_subscribe(subscribe_topic, esp_rmaker_set_params_callback, RMAKER_MQTT_QOS1, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to subscribe to %s. Error %d", subscribe_topic, err);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t esp_rmaker_param_get_stored_value(_esp_rmaker_param_t *param, esp_rmaker_param_val_t *val)
{
    if (!param || !param->parent || !val) {
        return ESP_FAIL;
    }
    nvs_handle handle;
    esp_err_t err = nvs_open_from_partition(ESP_RMAKER_NVS_PART_NAME, param->parent->name, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return err;
    }
    if ((param->val.type == RMAKER_VAL_TYPE_STRING) || (param->val.type == RMAKER_VAL_TYPE_OBJECT) ||
                (param->val.type == RMAKER_VAL_TYPE_ARRAY)) {
        size_t len = 0;
        if ((err = nvs_get_blob(handle, param->name, NULL, &len)) == ESP_OK) {
            char *s_val = MEM_CALLOC_EXTRAM(1, len + 1);
            if (!s_val) {
                err = ESP_ERR_NO_MEM;
            } else {
                nvs_get_blob(handle, param->name, s_val, &len);
                s_val[len] = '\0';
                val->type = param->val.type;
                val->val.s = s_val;
            }
        } else if ((err = nvs_get_str(handle, param->name, NULL, &len)) == ESP_OK) {
            /* In order to be compatible with the previous nvs_set_str() */
            char *s_val = MEM_CALLOC_EXTRAM(1, len);
            if (!s_val) {
                err = ESP_ERR_NO_MEM;
            } else {
                nvs_get_str(handle, param->name, s_val, &len);
                val->type = param->val.type;
                val->val.s = s_val;
            }
        }
    } else {
        size_t len = sizeof(esp_rmaker_param_val_t);
        err = nvs_get_blob(handle, param->name, val, &len);
    }
    nvs_close(handle);
    return err;
}

esp_err_t esp_rmaker_param_store_value(_esp_rmaker_param_t *param)
{
    if (!param || !param->parent) {
        return ESP_FAIL;
    }
    nvs_handle handle;
    esp_err_t err = nvs_open_from_partition(ESP_RMAKER_NVS_PART_NAME, param->parent->name, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }
    if ((param->val.type == RMAKER_VAL_TYPE_STRING) || (param->val.type == RMAKER_VAL_TYPE_OBJECT) ||
                (param->val.type == RMAKER_VAL_TYPE_ARRAY)) {
        /* Store only if value is not NULL */
        if (param->val.val.s) {
            err = nvs_set_blob(handle, param->name, param->val.val.s, strlen(param->val.val.s));
            nvs_commit(handle);
        } else {
            err = ESP_OK;
        }
    } else {
        err = nvs_set_blob(handle, param->name, &param->val, sizeof(esp_rmaker_param_val_t));
        nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

esp_rmaker_param_val_t *esp_rmaker_param_get_val(esp_rmaker_param_t *param)
{
    if (!param) {
        ESP_LOGE(TAG, "Param handle cannot be NULL.");
        return NULL;
    }
    return &((_esp_rmaker_param_t *)param)->val;
}

esp_err_t esp_rmaker_param_delete(const esp_rmaker_param_t *param)
{
    _esp_rmaker_param_t *_param = (_esp_rmaker_param_t *)param;
    if (_param) {
        if (_param->name) {
            free(_param->name);
        }
        if (_param->type) {
            free(_param->type);
        }
        if (_param->ui_type) {
            free(_param->ui_type);
        }
        free(_param);
        return ESP_OK;
    }
    return ESP_ERR_INVALID_ARG;
}

esp_rmaker_param_t *esp_rmaker_param_create(const char *param_name, const char *type,
        esp_rmaker_param_val_t val, uint8_t properties)
{
    if (!param_name) {
        ESP_LOGE(TAG, "Param name is mandatory");
        return NULL;
    }
    if (properties & PROP_FLAG_TIME_SERIES) {
        if ((val.type == RMAKER_VAL_TYPE_ARRAY) || (val.type == RMAKER_VAL_TYPE_OBJECT)) {
            ESP_LOGE(TAG, "PROP_FLAG_TIME_SERIES not allowed for array/object param types.");
            return NULL;
        }
    }
    _esp_rmaker_param_t *param = MEM_CALLOC_EXTRAM(1, sizeof(_esp_rmaker_param_t));
    if (!param) {
        ESP_LOGE(TAG, "Failed to allocate memory for param %s", param_name);
        return NULL;
    }
    param->name = strdup(param_name);
    if (!param->name) {
        ESP_LOGE(TAG, "Failed to allocate memory for name for param %s.", param_name);
        goto param_create_err;
    }
    if (type) {
        param->type = strdup(type);
        if (!param->type) {
            ESP_LOGE(TAG, "Failed to allocate memory for type for param %s.", param_name);
            goto param_create_err;
        }
    }
    param->val.type = val.type;
    param->prop_flags = properties;
    if ((val.type == RMAKER_VAL_TYPE_STRING) || (val.type == RMAKER_VAL_TYPE_OBJECT) ||
                (val.type == RMAKER_VAL_TYPE_ARRAY)) {
        if (val.val.s) {
             param->val.val.s = strdup(val.val.s);
             if (!param->val.val.s) {
                 ESP_LOGE(TAG, "Failed to allocate memory for the value of param %s.", param_name);
             }
        }
    } else {
        param->val.val = val.val;
    }
    if (properties & PROP_FLAG_TIME_SERIES) {
        /* Time series params will require time sync */
        esp_rmaker_time_sync_init(NULL);
    }
    return (esp_rmaker_param_t *)param;

param_create_err:
    esp_rmaker_param_delete((esp_rmaker_param_t *)param);
    return NULL;
}

esp_err_t esp_rmaker_param_add_bounds(const esp_rmaker_param_t *param,
    esp_rmaker_param_val_t min, esp_rmaker_param_val_t max, esp_rmaker_param_val_t step)
{
    if (!param) {
        ESP_LOGE(TAG, "Param handle cannot be NULL.");
        return ESP_ERR_INVALID_ARG;
    }
    _esp_rmaker_param_t *_param = (_esp_rmaker_param_t *)param;
    if ((_param->val.type != RMAKER_VAL_TYPE_INTEGER) && (_param->val.type != RMAKER_VAL_TYPE_FLOAT)) {
        ESP_LOGE(TAG, "Only integer and float params can have bounds.");
        return ESP_ERR_INVALID_ARG;
    }
    if ((min.type != _param->val.type) || (max.type != _param->val.type) || (step.type != _param->val.type)) {
        ESP_LOGE(TAG, "Cannot set bounds for %s because of value type mismatch.", _param->name);
        return ESP_ERR_INVALID_ARG;
    }
    esp_rmaker_param_bounds_t *bounds = MEM_CALLOC_EXTRAM(1, sizeof(esp_rmaker_param_bounds_t));
    if (!bounds) {
        ESP_LOGE(TAG, "Failed to allocate memory for parameter bounds.");
        return ESP_ERR_NO_MEM;
    }
    bounds->min = min;
    bounds->max = max;
    bounds->step = step;
    if (_param->bounds) {
        free(_param->bounds);
    }
    _param->bounds = bounds;
    return ESP_OK;
}

esp_err_t esp_rmaker_param_add_valid_str_list(const esp_rmaker_param_t *param, const char *strs[], uint8_t count)
{
    if (!param) {
        ESP_LOGE(TAG, "Param handle cannot be NULL.");
        return ESP_ERR_INVALID_ARG;
    }
    _esp_rmaker_param_t *_param = (_esp_rmaker_param_t *)param;
    if (_param->val.type != RMAKER_VAL_TYPE_STRING) {
        ESP_LOGE(TAG, "Only string params can have valid strings array.");
        return ESP_ERR_INVALID_ARG;
    }
    esp_rmaker_param_valid_str_list_t *valid_str_list = MEM_CALLOC_EXTRAM(1, sizeof(esp_rmaker_param_valid_str_list_t));
    if (!valid_str_list) {
        ESP_LOGE(TAG, "Failed to allocate memory for valid strings array.");
        return ESP_ERR_NO_MEM;
    }
    valid_str_list->str_list = strs;
    valid_str_list->str_list_cnt = count;
    if (_param->valid_str_list) {
        free(_param->valid_str_list);
    }
    _param->valid_str_list = valid_str_list;
  return ESP_OK;
}

esp_err_t esp_rmaker_param_add_array_max_count(const esp_rmaker_param_t *param, int count)
{
    if (!param) {
        ESP_LOGE(TAG, "Param handle cannot be NULL.");
        return ESP_ERR_INVALID_ARG;
    }
    _esp_rmaker_param_t *_param = (_esp_rmaker_param_t *)param;
    if (_param->val.type != RMAKER_VAL_TYPE_ARRAY) {
        ESP_LOGE(TAG, "Only array params can have max count.");
        return ESP_ERR_INVALID_ARG;
    }
    esp_rmaker_param_bounds_t *bounds = MEM_CALLOC_EXTRAM(1, sizeof(esp_rmaker_param_bounds_t));
    if (!bounds) {
        ESP_LOGE(TAG, "Failed to allocate memory for parameter bounds.");
        return ESP_ERR_NO_MEM;
    }
    bounds->max = esp_rmaker_int(count);
    if (_param->bounds) {
        free(_param->bounds);
    }
    _param->bounds = bounds;
    return ESP_OK;
}

esp_err_t esp_rmaker_param_add_ui_type(const esp_rmaker_param_t *param, const char *ui_type)
{
    if (!param || !ui_type) {
        ESP_LOGE(TAG, "Param handle or UI type cannot be NULL.");
        return ESP_ERR_INVALID_ARG;
    }
    _esp_rmaker_param_t *_param = (_esp_rmaker_param_t *)param;
    if (_param->ui_type) {
        free(_param->ui_type);
    }
    if ((_param->ui_type = strdup(ui_type)) != NULL ) {
        return ESP_OK;
    } else {
        return ESP_ERR_NO_MEM;
    }
}

esp_err_t esp_rmaker_param_update(const esp_rmaker_param_t *param, esp_rmaker_param_val_t val)
{
    if (!param) {
        ESP_LOGE(TAG, "Param handle cannot be NULL.");
        return ESP_ERR_INVALID_ARG;
    }
    _esp_rmaker_param_t *_param = (_esp_rmaker_param_t *)param;
    if (_param->val.type != val.type) {
        ESP_LOGE(TAG, "New param value type not same as the existing one.");
        return ESP_ERR_INVALID_ARG;
    }
    switch (_param->val.type) {
        case RMAKER_VAL_TYPE_STRING:
        case RMAKER_VAL_TYPE_OBJECT:
        case RMAKER_VAL_TYPE_ARRAY: {
            char *new_val = NULL;
            if (val.val.s) {
                new_val = strdup(val.val.s);
                if (!new_val) {
                    return ESP_FAIL;
                }
            }
            if (_param->val.val.s) {
                free(_param->val.val.s);
            }
            _param->val.val.s = new_val;
            break;
        }
        case RMAKER_VAL_TYPE_BOOLEAN:
        case RMAKER_VAL_TYPE_INTEGER:
        case RMAKER_VAL_TYPE_FLOAT:
            _param->val.val = val.val;
            break;
        default:
            return ESP_ERR_INVALID_ARG;
    }
    _param->flags |= RMAKER_PARAM_FLAG_VALUE_CHANGE;
    if (_param->prop_flags & PROP_FLAG_PERSIST) {
        esp_rmaker_param_store_value(_param);
    }
    return ESP_OK;
}

esp_err_t esp_rmaker_param_report(const esp_rmaker_param_t *param)
{
    if (!param) {
        ESP_LOGE(TAG, "Param handle cannot be NULL.");
        return ESP_ERR_INVALID_ARG;
    }
    return esp_rmaker_report_param_internal(RMAKER_PARAM_FLAG_VALUE_CHANGE);
}

static esp_err_t __esp_rmaker_param_report_time_series_records(json_gen_str_t *jptr, const _esp_rmaker_param_t *param)
{
    json_gen_start_object(jptr);
    time_t current_timestamp = 0;
    time(&current_timestamp);
    json_gen_obj_set_int(jptr, "t", (int)current_timestamp);
    esp_rmaker_report_value(&param->val, "v", jptr);
    json_gen_end_object(jptr);
    return ESP_OK;
}
static esp_err_t __esp_rmaker_param_report_time_series(json_gen_str_t *jptr, const esp_rmaker_param_t *param)
{
    json_gen_start_object(jptr);
    char param_name[MAX_TS_DATA_PARAM_NAME];
    if (!param) {
        return ESP_ERR_INVALID_ARG;
    }
    _esp_rmaker_param_t *_param = (_esp_rmaker_param_t *)param;
    _esp_rmaker_device_t *device = _param->parent;
    if (!device) {
        return ESP_FAIL;
    }
    snprintf(param_name, sizeof(param_name), "%s.%s", device->name, _param->name);
    json_gen_obj_set_string(jptr, "name", param_name);
    esp_rmaker_report_data_type( _param->val.type, "dt", jptr);
    json_gen_push_array(jptr, "records");
    __esp_rmaker_param_report_time_series_records(jptr, _param);
    json_gen_pop_array(jptr);
    json_gen_end_object(jptr);
    return ESP_OK;
}

static esp_err_t esp_rmaker_param_report_time_series(const esp_rmaker_param_t *param)
{
    if (!param) {
        ESP_LOGE(TAG, "Param handle cannot be NULL.");
        return ESP_ERR_INVALID_ARG;
    }
    if (!((_esp_rmaker_param_t *)param)->parent) {
        ESP_LOGE(TAG, "Param \"%s\" has not been added to any device.", ((_esp_rmaker_param_t *)param)->name);
        return ESP_FAIL;
    }
    if (esp_rmaker_time_check() != true) {
        ESP_LOGE(TAG, "Current time not yet available. Cannot report time series data.");
        return ESP_ERR_INVALID_STATE;
    }
    /* node_params_buf will be NULL during the first publish */
    char * node_params_buf = esp_rmaker_param_get_buf(max_node_params_size);
    if (!node_params_buf) {
        return ESP_ERR_NO_MEM;
    }
    esp_err_t err;
    json_gen_str_t jstr;
    int buf_len = max_node_params_size;
    json_gen_str_start(&jstr, node_params_buf, buf_len, NULL, NULL);
    json_gen_start_object(&jstr);
    json_gen_obj_set_string(&jstr, "ts_data_version", TS_DATA_VERSION);
    json_gen_push_array(&jstr, "ts_data");
    if ((err = __esp_rmaker_param_report_time_series(&jstr, param)) != ESP_OK) {
        return err;
    }
    json_gen_pop_array(&jstr);
    json_gen_end_object(&jstr);
    json_gen_str_end(&jstr);
    esp_rmaker_create_mqtt_topic(publish_topic, sizeof(publish_topic), TIME_SERIES_DATA_TOPIC_SUFFIX, TIME_SERIES_DATA_TOPIC_RULE);
    if (esp_rmaker_params_mqtt_init_done) {
        _esp_rmaker_param_t *_param = (_esp_rmaker_param_t *)param;
        _esp_rmaker_device_t *_device = _param->parent;
        ESP_LOGI(TAG, "Reporting Time Series Data for %s.%s", _device->name, _param->name);
        esp_rmaker_mqtt_publish(publish_topic, node_params_buf, strlen(node_params_buf), RMAKER_MQTT_QOS1, NULL);
    }
    return ESP_OK;
}

esp_err_t esp_rmaker_param_notify(const esp_rmaker_param_t *param)
{
    if (!param) {
        ESP_LOGE(TAG, "Param handle cannot be NULL.");
        return ESP_ERR_INVALID_ARG;
    }
    ((_esp_rmaker_param_t *)param)->flags |= (RMAKER_PARAM_FLAG_VALUE_CHANGE | RMAKER_PARAM_FLAG_VALUE_NOTIFY);
    esp_err_t err = esp_rmaker_report_param_internal(RMAKER_PARAM_FLAG_VALUE_NOTIFY);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to report parameter");
    }
    return esp_rmaker_report_param_internal(RMAKER_PARAM_FLAG_VALUE_CHANGE);
}

esp_err_t esp_rmaker_param_update_and_report(const esp_rmaker_param_t *param, esp_rmaker_param_val_t val)
{
    esp_err_t err = esp_rmaker_param_update(param, val);
    /** Report parameter only if the RainMaker has started */
    if ((err == ESP_OK) && (esp_rmaker_get_state() == ESP_RMAKER_STATE_STARTED)) {
        if (((_esp_rmaker_param_t *)param)->prop_flags & PROP_FLAG_TIME_SERIES) {
            esp_rmaker_param_report_time_series(param);
        }
        err = esp_rmaker_param_report(param);
    }
    return err;
}

esp_err_t esp_rmaker_param_update_and_notify(const esp_rmaker_param_t *param, esp_rmaker_param_val_t val)
{
    esp_err_t err = esp_rmaker_param_update(param, val);
    /** Report parameter only if the RainMaker has started */
    if ((err == ESP_OK) && (esp_rmaker_get_state() == ESP_RMAKER_STATE_STARTED)) {
        if (((_esp_rmaker_param_t *)param)->prop_flags & PROP_FLAG_TIME_SERIES) {
            esp_rmaker_param_report_time_series(param);
        }
        err = esp_rmaker_param_notify(param);
    }
    return err;
}

static esp_err_t esp_rmaker_report_all_ts_params(void)
{
    _esp_rmaker_device_t *device = esp_rmaker_node_get_first_device(esp_rmaker_get_node());
    while (device) {
        _esp_rmaker_param_t *param = device->params;
        while (param) {
            if (param->prop_flags & PROP_FLAG_TIME_SERIES) {
                esp_rmaker_param_report_time_series((esp_rmaker_param_t *)param);
            }
            param = param->next;
        }
        device = device->next;
    }
    return ESP_OK;
}


esp_err_t esp_rmaker_report_node_state(void)
{
    esp_err_t err = esp_rmaker_allocate_and_populate_params(0, false);
    if (err == ESP_OK) {
        /* Just checking if there are indeed any params to report by comparing with a decent enough
         * length as even the smallest possible data, Eg. '{"d":{"p":0}}' will be > 10 bytes.
         */
        char *node_params_buf = esp_rmaker_param_get_buf(0);
        if (strlen(node_params_buf) > 10) {
            esp_rmaker_create_mqtt_topic(publish_topic, sizeof(publish_topic), NODE_PARAMS_LOCAL_INIT_TOPIC_SUFFIX, NODE_PARAMS_LOCAL_INIT_RULE);
            ESP_LOGI(TAG, "Reporting params (init): %s", node_params_buf);
            if (esp_rmaker_params_mqtt_init_done) {
                esp_rmaker_mqtt_publish(publish_topic, node_params_buf, strlen(node_params_buf), RMAKER_MQTT_QOS1, NULL);
            } else {
                ESP_LOGW(TAG, "Not reporting params since params mqtt not initialized yet.");
            }
        }
        /* Report all Time Series Params separately */
        return esp_rmaker_report_all_ts_params();
    }
    return err;
}

esp_err_t esp_rmaker_params_mqtt_init(void)
{
    /* Subscribe for parameter update requests */
    esp_err_t err = esp_rmaker_register_for_set_params();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Params MQTT Init done.");
        esp_rmaker_params_mqtt_init_done = true;
        /* Report the current node state i.e. values of all the node parameters */
        esp_rmaker_report_node_state();
    }
    return err;
}

char *esp_rmaker_param_get_name(const esp_rmaker_param_t *param)
{
    if (!param) {
        ESP_LOGE(TAG, "Param handle cannot be NULL.");
        return NULL;
    }
    return ((_esp_rmaker_param_t *)param)->name;
}

char *esp_rmaker_param_get_type(const esp_rmaker_param_t *param)
{
    if (!param) {
        ESP_LOGE(TAG, "Param handle cannot be NULL.");
        return NULL;
    }
    return ((_esp_rmaker_param_t *)param)->type;
}

esp_err_t esp_rmaker_raise_alert(const char *alert_str)
{
    char msg[ESP_RMAKER_MAX_ALERT_LEN + 1]; /* + 1 for NULL terminattion */
    strlcpy(msg, alert_str, sizeof(msg));
    char buf[ESP_RMAKER_MAX_ALERT_LEN + RMAKER_ALERT_STR_MARGIN];
    snprintf(buf, sizeof(buf), "{\"%s\":\"%s\"}", ESP_RMAKER_ALERT_KEY, msg);
    esp_rmaker_create_mqtt_topic(publish_topic, sizeof(publish_topic), NODE_PARAMS_ALERT_TOPIC_SUFFIX, NODE_PARAMS_ALERT_TOPIC_RULE);
    ESP_LOGI(TAG, "Reporting alert: %s", buf);
    return esp_rmaker_mqtt_publish(publish_topic, buf, strlen(buf), RMAKER_MQTT_QOS1, NULL);
}
