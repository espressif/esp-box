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

#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_utils.h>

#include "esp_rmaker_internal.h"

static const char *TAG = "esp_rmaker_device";

esp_err_t esp_rmaker_device_delete(const esp_rmaker_device_t *device)
{
    _esp_rmaker_device_t *_device = (_esp_rmaker_device_t *)device;
    if (_device) {
        if (_device->parent) {
            ESP_LOGE(TAG, "Cannot delete device as it is part of a node. Remove it from the node first.");
            return ESP_ERR_INVALID_STATE;
        }
        esp_rmaker_attr_t *attr = _device->attributes;
        while (attr) {
            esp_rmaker_attr_t *next_attr = attr->next;
            esp_rmaker_attribute_delete(attr);
            attr = next_attr;
        }
        _esp_rmaker_param_t *param = _device->params;
        while (param) {
            _esp_rmaker_param_t *next_param = param->next;
            esp_rmaker_param_delete((esp_rmaker_param_t *)param);
            param = next_param;
        }
        if (_device->subtype) {
            free(_device->subtype);
        }
        if (_device->model) {
            free(_device->model);
        }
        if (_device->name) {
            free(_device->name);
        }
        if (_device->type) {
            free(_device->type);
        }
        return ESP_OK;
    }
    return ESP_ERR_INVALID_ARG;
}

static esp_rmaker_device_t *__esp_rmaker_device_create(const char *name, const char *type, void *priv, bool is_service)
{
    if (!name) {
        ESP_LOGE(TAG, "%s name is mandatory", is_service ? "Service":"Device");
        return NULL;
    }
    _esp_rmaker_device_t *_device = MEM_CALLOC_EXTRAM(1, sizeof(_esp_rmaker_device_t));
    if (!_device) {
        ESP_LOGE(TAG, "Failed to allocate memory for %s %s", is_service ? "Service":"Device", name);
        return NULL;
    }
    _device->name = strdup(name);
    if (!_device->name) {
        ESP_LOGE(TAG, "Failed to allocate memory for name for %s %s", is_service ? "Service":"Device", name);
        goto device_create_err;
    }
    if (type) {
        _device->type = strdup(type);
        if (!_device->type) {
            ESP_LOGE(TAG, "Failed to allocate memory for type for %s %s", is_service ? "Service":"Device", name);
            goto device_create_err;
        }
    }
    _device->priv_data = priv;
    _device->is_service = is_service;

    return (esp_rmaker_device_t *)_device;

device_create_err:
    esp_rmaker_device_delete((esp_rmaker_device_t *)_device);
    return NULL;
}

esp_rmaker_device_t *esp_rmaker_device_create(const char *name, const char *type, void *priv)
{
    return __esp_rmaker_device_create(name, type, priv, false);
}
esp_rmaker_device_t *esp_rmaker_service_create(const char *name, const char *type, void *priv)
{
    return __esp_rmaker_device_create(name, type, priv, true);
}

esp_err_t esp_rmaker_device_add_param(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param)
{
    if (!device || !param) {
        ESP_LOGE(TAG, "Device or Param handle cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    _esp_rmaker_device_t *_device = (_esp_rmaker_device_t *)device;
    _esp_rmaker_param_t *_new_param = (_esp_rmaker_param_t *)param;

    _esp_rmaker_param_t *_param = _device->params;
    while(_param) {
        if (strcmp(_param->name, _new_param->name) == 0) {
            ESP_LOGE(TAG, "Parameter with name %s already exists in Device %s", _new_param->name, _device->name);
            return ESP_ERR_INVALID_ARG;
        }
        if (_param->next) {
            _param = _param->next;
        } else {
            break;
        }
    }
    _new_param->parent = _device;
    if (_param) {
        _param->next = _new_param;
    } else {
        _device->params = _new_param;
    }
    /* We check the stored value here, and not during param creation, because a parameter
     * in itself isn't unique. However, it is unique within a given device and hence can
     * be uniquely represented in storage only when added to a device.
     */
    esp_rmaker_param_val_t stored_val;
    stored_val.type = _new_param->val.type;
    if (_new_param->prop_flags & PROP_FLAG_PERSIST) {
        if (esp_rmaker_param_get_stored_value(_new_param, &stored_val) == ESP_OK) {
            if ((_new_param->val.type == RMAKER_VAL_TYPE_STRING) || (_new_param->val.type == RMAKER_VAL_TYPE_OBJECT)
                    || (_new_param->val.type == RMAKER_VAL_TYPE_ARRAY)) {
                if (_new_param->val.val.s) {
                    free(_new_param->val.val.s);
                }
            }
            _new_param->val = stored_val;
            /* The device callback should be invoked once with the stored value, so
             * that applications can do initialisations as required.
             */
            if (_device->write_cb) {
                /* However, the callback should be invoked, only if the parameter is not
                 * of type ESP_RMAKER_PARAM_NAME, as it has special handling internally.
                 */
                if (!(_new_param->type && strcmp(_new_param->type, ESP_RMAKER_PARAM_NAME) == 0)) {
                    esp_rmaker_write_ctx_t ctx = {
                        .src = ESP_RMAKER_REQ_SRC_INIT,
                    };
                    _device->write_cb(device, param, stored_val, _device->priv_data, &ctx);
                }
            }
        } else {
            esp_rmaker_param_store_value(_new_param);
        }
    }
    ESP_LOGD(TAG, "Param %s added in %s", _new_param->name, _device->name);
    return ESP_OK;
}

/* Add a new Device Attribute */
esp_err_t esp_rmaker_device_add_attribute(const esp_rmaker_device_t *device, const char *attr_name, const char *val)
{
    if (!device || !attr_name || !val) {
        ESP_LOGE(TAG, "Device handle, attribute name or value cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    _esp_rmaker_device_t *_device = ( _esp_rmaker_device_t *)device;
    esp_rmaker_attr_t *attr = _device->attributes;
    while(attr) {
        if (strcmp(attr_name, attr->name) == 0) {
            ESP_LOGE(TAG, "Attribute with name %s already exists in Device %s", attr_name, _device->name);
            return ESP_ERR_INVALID_ARG;
        }
        if (attr->next) {
            attr = attr->next;
        } else {
            break;
        }
    }
    esp_rmaker_attr_t *new_attr = MEM_CALLOC_EXTRAM(1, sizeof(esp_rmaker_attr_t));
    if (!new_attr) {
        ESP_LOGE(TAG, "Failed to allocate memory for device attribute");
        return ESP_ERR_NO_MEM;
    }
    new_attr->name = strdup(attr_name);
    new_attr->value = strdup(val);
    if (!new_attr->name || !new_attr->value) {
        ESP_LOGE(TAG, "Failed to allocate memory for device attribute name or value");
        esp_rmaker_attribute_delete(new_attr);
        return ESP_ERR_NO_MEM;
    }
    if (attr) {
        attr->next = new_attr;
    } else {
        _device->attributes = new_attr;
    }
    ESP_LOGD(TAG, "Device attribute %s.%s added", _device->name, attr_name);
    return ESP_OK;
}

/* Add a device subtype */
esp_err_t esp_rmaker_device_add_subtype(const esp_rmaker_device_t *device, const char *subtype)
{
    if (!device || !subtype) {
        ESP_LOGE(TAG, "Device handle or subtype cannot be NULL.");
        return ESP_ERR_INVALID_ARG;
    }
    _esp_rmaker_device_t *_device = (_esp_rmaker_device_t *)device;
    if (_device->subtype) {
        free(_device->subtype);
    }
    if ((_device->subtype = strdup(subtype)) != NULL ){
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to allocate memory for device subtype");
        return ESP_ERR_NO_MEM;
    }
}

/* Add a device model */
esp_err_t esp_rmaker_device_add_model(const esp_rmaker_device_t *device, const char *model)
{
    if (!device || !model) {
        ESP_LOGE(TAG, "Device handle or model cannot be NULL.");
        return ESP_ERR_INVALID_ARG;
    }
    _esp_rmaker_device_t *_device = (_esp_rmaker_device_t *)device;
    if (_device->model) {
        free(_device->model);
    }
    if ((_device->model = strdup(model)) != NULL ){
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to allocate memory for device model");
        return ESP_ERR_NO_MEM;
    }
}

esp_err_t esp_rmaker_device_assign_primary_param(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param)
{
    if (!device || !param) {
        ESP_LOGE(TAG,"Device or Param handle cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    ((_esp_rmaker_device_t *)device)->primary = (_esp_rmaker_param_t *)param;
    return ESP_OK;
}

esp_err_t esp_rmaker_device_add_cb(const esp_rmaker_device_t *device, esp_rmaker_device_write_cb_t write_cb, esp_rmaker_device_read_cb_t read_cb)
{
    if (!device) {
        ESP_LOGE(TAG, "Device handle cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    _esp_rmaker_device_t *_device = (_esp_rmaker_device_t *)device;
    _device->write_cb = write_cb;
    _device->read_cb = read_cb;
    return ESP_OK;
}

char *esp_rmaker_device_get_name(const esp_rmaker_device_t *device)
{
    if (!device) {
        ESP_LOGE(TAG, "Device handle cannot be NULL.");
        return NULL;
    }
    return ((_esp_rmaker_device_t *)device)->name;
}

char *esp_rmaker_device_get_type(const esp_rmaker_device_t *device)
{
    if (!device) {
        ESP_LOGE(TAG, "Device handle cannot be NULL.");
        return NULL;
    }
    return ((_esp_rmaker_device_t *)device)->type;
}

esp_rmaker_param_t *esp_rmaker_device_get_param_by_type(const esp_rmaker_device_t *device, const char *param_type)
{
    if (!device || !param_type) {
        ESP_LOGE(TAG, "Device handle or param type cannot be NULL");
        return NULL;
    }
    _esp_rmaker_param_t *param = ((_esp_rmaker_device_t *)device)->params;
    while(param) {
        if (strcmp(param->type, param_type) == 0) {
            break;
        }
        param = param->next;
    }
    return (esp_rmaker_param_t *)param;
}

esp_rmaker_param_t *esp_rmaker_device_get_param_by_name(const esp_rmaker_device_t *device, const char *param_name)
{
    if (!device || !param_name) {
        ESP_LOGE(TAG, "Device handle or param name cannot be NULL");
        return NULL;
    }
    _esp_rmaker_param_t *param = ((_esp_rmaker_device_t *)device)->params;
    while(param) {
        if (strcmp(param->name, param_name) == 0) {
            break;
        }
        param = param->next;
    }
    return (esp_rmaker_param_t *)param;
}
