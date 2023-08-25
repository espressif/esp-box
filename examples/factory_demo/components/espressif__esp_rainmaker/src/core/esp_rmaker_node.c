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
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_rmaker_utils.h>
#include <esp_rmaker_core.h>

#include "esp_rmaker_internal.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include <esp_app_desc.h>
#endif

static const char *TAG = "esp_rmaker_node";

static void esp_rmaker_node_info_free(esp_rmaker_node_info_t *info)
{
    if (info) {
        if (info->name) {
            free(info->name);
        }
        if (info->type) {
            free(info->type);
        }
        if (info->model) {
            free(info->model);
        }
        if (info->fw_version) {
            free(info->fw_version);
        }
        if (info->subtype) {
            free(info->subtype);
        }
        free(info);
    }
}

esp_err_t esp_rmaker_attribute_delete(esp_rmaker_attr_t *attr)
{
    if (attr) {
        if (attr->name) {
            free(attr->name);
        }
        if (attr->value) {
            free(attr->value);
        }
        free(attr);
        return ESP_OK;
    }
    return ESP_ERR_INVALID_ARG;
}

esp_err_t esp_rmaker_node_delete(const esp_rmaker_node_t *node)
{
    _esp_rmaker_node_t *_node = (_esp_rmaker_node_t *)node;
    if (_node) {
        esp_rmaker_attr_t *attr = _node->attributes;
        while (attr) {
            esp_rmaker_attr_t *next_attr = attr->next;
            esp_rmaker_attribute_delete(attr);
            attr = next_attr;
        }
        _esp_rmaker_device_t *device = _node->devices;
        while (device) {
            _esp_rmaker_device_t *next_device = device->next;
            device->parent = NULL;
            esp_rmaker_device_delete((esp_rmaker_device_t *)device);
            device = next_device;
        }
        /* Node ID is created in the context of esp_rmaker_init and just assigned
         * here. So, we would not free it here.
         */
        if (_node->node_id) {
            _node->node_id = NULL;
        }
        if (_node->info) {
            esp_rmaker_node_info_free(_node->info);
        }
        return ESP_OK;
    }
    return ESP_ERR_INVALID_ARG;
}

esp_rmaker_node_t *esp_rmaker_node_create(const char *name, const char *type)
{
    static bool node_created;
    if (node_created) {
        ESP_LOGE(TAG, "Node has already been created. Cannot create another");
        return NULL;
    }
    if (!name || !type) {
        ESP_LOGE(TAG, "Node Name and Type are mandatory.");
        return NULL;
    }
    _esp_rmaker_node_t *node = MEM_CALLOC_EXTRAM(1, sizeof(_esp_rmaker_node_t));
    if (!node) {
        ESP_LOGE(TAG, "Failed to allocate memory for node.");
        return NULL;
    }
    node->node_id = esp_rmaker_get_node_id();
    if (!node->node_id) {
        ESP_LOGE(TAG, "Failed to initialise Node Id. Please perform \"claiming\" using RainMaker CLI.");
        goto node_create_err;
    }
    ESP_LOGI(TAG, "Node ID ----- %s", node->node_id);

    node->info = MEM_CALLOC_EXTRAM(1, sizeof(esp_rmaker_node_info_t));
    if (!node->info) {
        ESP_LOGE(TAG, "Failed to allocate memory for node info.");
        goto node_create_err;
    }
    node->info->name = strdup(name);
    node->info->type = strdup(type);
    const esp_app_desc_t *app_desc;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    app_desc = esp_app_get_description();
#else
    app_desc = esp_ota_get_app_description();
#endif
    node->info->fw_version = strdup(app_desc->version);
    node->info->model = strdup(app_desc->project_name);
    if (!node->info->name || !node->info->type
            || !node->info->fw_version || !node->info->model) {
        ESP_LOGE(TAG, "Failed to allocate memory for node info.");
        goto node_create_err;
    }
    node_created = true;
    return (esp_rmaker_node_t *)node;
node_create_err:
    esp_rmaker_node_delete((esp_rmaker_node_t *)node);
    return NULL;
}

esp_err_t esp_rmaker_node_add_fw_version(const esp_rmaker_node_t *node, const char *fw_version)
{
    if (!node || !fw_version) {
        ESP_LOGE(TAG, "Node handle or fw version cannot be NULL.");
        return ESP_ERR_INVALID_ARG;
    }
    esp_rmaker_node_info_t *info = esp_rmaker_node_get_info(node);
    if (!info) {
        ESP_LOGE(TAG, "Failed to get Node Info.");
        return ESP_ERR_INVALID_ARG;
    }
    if (info->fw_version) {
        free(info->fw_version);
    }
    info->fw_version = strdup(fw_version);
    if (!info->fw_version) {
        ESP_LOGE(TAG, "Failed to allocate memory for fw version.");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t esp_rmaker_node_add_model(const esp_rmaker_node_t *node, const char *model)
{
    if (!node || !model) {
        ESP_LOGE(TAG, "Node handle or model cannot be NULL.");
        return ESP_ERR_INVALID_ARG;
    }
    esp_rmaker_node_info_t *info = esp_rmaker_node_get_info(node);
    if (!info) {
        ESP_LOGE(TAG, "Failed to get Node Info.");
        return ESP_ERR_INVALID_ARG;
    }
    if (info->model) {
        free(info->model);
    }
    info->model = strdup(model);
    if (!info->model) {
        ESP_LOGE(TAG, "Failed to allocate memory for node model.");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t esp_rmaker_node_add_subtype(const esp_rmaker_node_t *node, const char *subtype)
{
    if (!node || !subtype) {
        ESP_LOGE(TAG, "Node handle or subtype cannot be NULL.");
        return ESP_ERR_INVALID_ARG;
    }
    esp_rmaker_node_info_t *info = esp_rmaker_node_get_info(node);
    if (!info) {
        ESP_LOGE(TAG, "Failed to get Node Info.");
        return ESP_ERR_INVALID_ARG;
    }
    if (info->subtype) {
        free(info->subtype);
    }
    info->subtype = strdup(subtype);
    if (!info->subtype) {
        ESP_LOGE(TAG, "Failed to allocate memory for node subtype.");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t esp_rmaker_node_add_attribute(const esp_rmaker_node_t *node, const char *attr_name, const char *value)
{
    if (!node || !attr_name || !value) {
        ESP_LOGE(TAG, "Node handle, attribute name or value cannot be NULL.");
        return ESP_ERR_INVALID_ARG;
    }
    esp_rmaker_attr_t *attr = ((_esp_rmaker_node_t *)node)->attributes;
    while(attr && attr->next) {
        if (strcmp(attr->name, attr_name) == 0) {
            ESP_LOGE(TAG, "Node attribute with name %s already exists.", attr_name);
            return ESP_FAIL;
        }
        attr = attr->next;
    }
    esp_rmaker_attr_t *new_attr = MEM_CALLOC_EXTRAM(1, sizeof(esp_rmaker_attr_t));
    if (!new_attr) {
        ESP_LOGE(TAG, "Failed to create node attribute %s.", attr_name);
        return ESP_ERR_NO_MEM;
    }
    new_attr->name = strdup(attr_name);
    new_attr->value = strdup(value);
    if (!new_attr->name || !new_attr->value) {
        ESP_LOGE(TAG, "Failed to allocate memory for name/value for attribute %s.", attr_name);
        esp_rmaker_attribute_delete(new_attr);
        return ESP_ERR_NO_MEM;
    }

    if (attr) {
        attr->next = new_attr;
    } else {
        ((_esp_rmaker_node_t *)node)->attributes = new_attr;
    }
    ESP_LOGI(TAG, "Node attribute %s created", attr_name);
    return ESP_OK;
}

esp_err_t esp_rmaker_node_add_device(const esp_rmaker_node_t *node, const esp_rmaker_device_t *device)
{
    if (!node || !device) {
        ESP_LOGE(TAG, "Node or Device/Service handle cannot be NULL.");
        return ESP_ERR_INVALID_ARG;
    }
    _esp_rmaker_node_t *_node = (_esp_rmaker_node_t *)node;
    _esp_rmaker_device_t *_new_device = (_esp_rmaker_device_t *)device;
    _esp_rmaker_device_t *_device = _node->devices;
    while(_device) {
        if (strcmp(_device->name, _new_device->name) == 0) {
            ESP_LOGE(TAG, "%s with name %s already exists", _new_device->is_service ? "Service":"Device", _new_device->name);
            return ESP_ERR_INVALID_ARG;
        }
        if (_device->next) {
            _device = _device->next;
        } else {
            break;
        }
    }
    if (_device) {
        _device->next = _new_device;
    } else {
        _node->devices = _new_device;
    }
    _new_device->parent = node;
    return ESP_OK;
}

esp_err_t esp_rmaker_node_remove_device(const esp_rmaker_node_t *node, const esp_rmaker_device_t *device)
{
    if (!node || !device) {
        ESP_LOGE(TAG, "Node or Device/Service handle cannot be NULL.");
        return ESP_ERR_INVALID_ARG;
    }
    _esp_rmaker_node_t *_node = (_esp_rmaker_node_t *)node;
    _esp_rmaker_device_t *_device = (_esp_rmaker_device_t *)device;

    _esp_rmaker_device_t *tmp_device = _node->devices;
    _esp_rmaker_device_t *prev_device = NULL;
    while(tmp_device) {
        if (tmp_device == _device) {
            break;
        }
        prev_device = tmp_device;
        tmp_device = tmp_device->next;
    }
    if (!tmp_device) {
         ESP_LOGE(TAG, "Device %s not found in node %s", _device->name, _node->info->name);
         return ESP_ERR_INVALID_ARG;
    }
    if (tmp_device == _node->devices) {
        _node->devices = tmp_device->next;
    } else {
        prev_device->next = tmp_device->next;
    }
    tmp_device->parent = NULL;
    return ESP_OK;
}

esp_rmaker_device_t *esp_rmaker_node_get_device_by_name(const esp_rmaker_node_t *node, const char *device_name)
{
    if (!node || !device_name) {
        ESP_LOGE(TAG, "Node handle or device name cannot be NULL");
        return NULL;
    }
    _esp_rmaker_device_t *device = ((_esp_rmaker_node_t *)node)->devices;
    while(device) {
        if (strcmp(device->name, device_name) == 0) {
            break;
        }
        device = device->next;
    }
    return (esp_rmaker_device_t *)device;
}

_esp_rmaker_device_t *esp_rmaker_node_get_first_device(const esp_rmaker_node_t *node)
{
    _esp_rmaker_node_t *_node = (_esp_rmaker_node_t *)node;
    if (!_node) {
        ESP_LOGE(TAG, "Node handle cannot be NULL.");
        return NULL;
    }
    return _node->devices;
}

esp_rmaker_node_info_t *esp_rmaker_node_get_info(const esp_rmaker_node_t *node)
{
    _esp_rmaker_node_t *_node = (_esp_rmaker_node_t *)node;
    if (!_node) {
        ESP_LOGE(TAG, "Node handle cannot be NULL.");
        return NULL;
    }
    return _node->info;
}

esp_rmaker_attr_t *esp_rmaker_node_get_first_attribute(const esp_rmaker_node_t *node)
{
    _esp_rmaker_node_t *_node = (_esp_rmaker_node_t *)node;
    if (!_node) {
        ESP_LOGE(TAG, "Node handle cannot be NULL.");
        return NULL;
    }
    return _node->attributes;
}

char *esp_rmaker_node_get_id(const esp_rmaker_node_t *node)
{
    _esp_rmaker_node_t *_node = (_esp_rmaker_node_t *)node;
    if (!_node) {
        ESP_LOGE(TAG, "Node handle cannot be NULL.");
        return NULL;
    }
    return _node->node_id;
}
