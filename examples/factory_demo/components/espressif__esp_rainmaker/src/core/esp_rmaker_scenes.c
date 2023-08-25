// Copyright 2022 Espressif Systems (Shanghai) PTE LTD
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

#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include <json_parser.h>
#include <json_generator.h>

#include <esp_rmaker_internal.h>
#include <esp_rmaker_standard_services.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_scenes.h>
#include <esp_rmaker_utils.h>

#define MAX_ID_LEN 8
#define MAX_NAME_LEN 32
#define MAX_INFO_LEN 100
#define MAX_OPERATION_LEN 10
#define MAX_SCENES CONFIG_ESP_RMAKER_SCENES_MAX_SCENES

static const char *TAG = "esp_rmaker_scenes";

typedef struct esp_rmaker_scene_action {
    void *data;
    size_t data_len;
} esp_rmaker_scene_action_t;

typedef struct esp_rmaker_scene {
    char name[MAX_NAME_LEN + 1];        /* +1 for NULL termination */
    char id[MAX_ID_LEN + 1];            /* +1 for NULL termination */
    /* Info is used to store additional information, it is limited to MAX_INFO_LEN bytes. */
    char *info;
    /* Flags can be used to identify the scene. */
    uint32_t flags;
    esp_rmaker_scene_action_t action;
    struct esp_rmaker_scene *next;
} esp_rmaker_scene_t;

typedef enum scenes_operation {
    OPERATION_INVALID,
    OPERATION_ADD,
    OPERATION_EDIT,
    OPERATION_REMOVE,
    OPERATION_ACTIVATE,
    OPERATION_DEACTIVATE,
} scenes_operation_t;

typedef struct {
    esp_rmaker_scene_t *scenes_list;
    int total_scenes;
    bool deactivate_support;
    esp_rmaker_device_t *scenes_service;
} esp_rmaker_scenes_priv_data_t;

static esp_rmaker_scenes_priv_data_t *scenes_priv_data;

static void esp_rmaker_scenes_free(esp_rmaker_scene_t *scene)
{
    if (!scene) {
        return;
    }
    if (scene->action.data) {
        free(scene->action.data);
    }
    if (scene->info) {
        free(scene->info);
    }
    free(scene);
}

static esp_rmaker_scene_t *esp_rmaker_scenes_get_scene_from_id(const char *id)
{
    if (!id) {
        return NULL;
    }
    esp_rmaker_scene_t *scene = scenes_priv_data->scenes_list;
    while(scene) {
        if (strncmp(id, scene->id, sizeof(scene->id)) == 0) {
            ESP_LOGD(TAG, "Scene with id %s found in list for get.", id);
            return scene;
        }
        scene = scene->next;
    }
    ESP_LOGD(TAG, "Scene with id %s not found in list for get.", id);
    return NULL;
}

static esp_err_t esp_rmaker_scenes_add_to_list(esp_rmaker_scene_t *scene)
{
    if (!scene) {
        ESP_LOGE(TAG, "Scene is NULL. Not adding to list.");
        return ESP_ERR_INVALID_ARG;
    }

    if (esp_rmaker_scenes_get_scene_from_id(scene->id) != NULL) {
        ESP_LOGI(TAG, "Scene with id %s already added to list. Not adding again.", scene->id);
        return ESP_FAIL;
    }
    /* Parse list */
    esp_rmaker_scene_t *prev_scene = scenes_priv_data->scenes_list;
    while(prev_scene) {
        if (prev_scene->next) {
            prev_scene = prev_scene->next;
        } else {
            break;
        }
    }

    /* Add to list */
    if (prev_scene) {
        prev_scene->next = scene;
    } else {
        scenes_priv_data->scenes_list = scene;
    }
    ESP_LOGD(TAG, "Scene with id %s added to list.", scene->id);
    scenes_priv_data->total_scenes++;
    return ESP_OK;
}

static esp_err_t esp_rmaker_scenes_remove_from_list(esp_rmaker_scene_t *scene)
{
    if (!scene) {
        ESP_LOGE(TAG, "Scene is NULL. Not removing from list.");
        return ESP_ERR_INVALID_ARG;
    }
    /* Parse list */
    esp_rmaker_scene_t *curr_scene = scenes_priv_data->scenes_list;
    esp_rmaker_scene_t *prev_scene = curr_scene;
    while(curr_scene) {
        if (strncmp(scene->id, curr_scene->id, sizeof(scene->id)) == 0) {
            ESP_LOGD(TAG, "Scene with id %s found in list for removing", scene->id);
            break;
        }
        prev_scene = curr_scene;
        curr_scene = curr_scene->next;
    }
    if (!curr_scene) {
        ESP_LOGE(TAG, "Scene with id %s not found in list. Not removing.", scene->id);
        return ESP_ERR_NOT_FOUND;
    }

    /* Remove from list */
    if (curr_scene == scenes_priv_data->scenes_list) {
        scenes_priv_data->scenes_list = curr_scene->next;
    } else {
        prev_scene->next = curr_scene->next;
    }
    scenes_priv_data->total_scenes--;
    ESP_LOGD(TAG, "Scene with id %s removed from list.", scene->id);
    return ESP_OK;
}

scenes_operation_t esp_rmaker_scenes_get_operation_from_str(char *operation)
{
    if (!operation) {
        return OPERATION_INVALID;
    }
    if (strncmp(operation, "add", strlen(operation)) == 0) {
        return OPERATION_ADD;
    } else if (strncmp(operation, "edit", strlen(operation)) == 0) {
        return OPERATION_EDIT;
    } else if (strncmp(operation, "remove", strlen(operation)) == 0) {
        return OPERATION_REMOVE;
    } else if (strncmp(operation, "activate", strlen(operation)) == 0) {
        return OPERATION_ACTIVATE;
    } else if (strncmp(operation, "deactivate", strlen(operation)) == 0) {
        return OPERATION_DEACTIVATE;
    }
    return OPERATION_INVALID;
}

static scenes_operation_t esp_rmaker_scenes_parse_operation(jparse_ctx_t *jctx, char *id)
{
    char operation_str[MAX_OPERATION_LEN + 1] = {0};        /* +1 for NULL termination */
    scenes_operation_t operation = OPERATION_INVALID;
    json_obj_get_string(jctx, "operation", operation_str, sizeof(operation_str));
    if (strlen(operation_str) <= 0) {
        ESP_LOGE(TAG, "Operation not found in scene with id: %s", id);
        return operation;
    }
    operation = esp_rmaker_scenes_get_operation_from_str(operation_str);
    if (operation == OPERATION_EDIT) {
        /* Get scene temporarily */
        if (esp_rmaker_scenes_get_scene_from_id(id) == NULL) {
            /* Operation is edit, but scene not present already. Consider this as add. */
            ESP_LOGD(TAG, "Operation is edit, but scene with id %s not found. Changing the operation to add.", id);
            operation = OPERATION_ADD;
        }
    } else if (operation == OPERATION_INVALID) {
        ESP_LOGE(TAG, "Invalid scene operation found: %s", operation_str);
    }
    return operation;
}

static esp_err_t esp_rmaker_scenes_parse_info_and_flags(jparse_ctx_t *jctx, char **info, uint32_t *flags)
{
    char _info[MAX_INFO_LEN + 1] = {0};  /* +1 for NULL termination */
    int _flags = 0;

    int err_code = json_obj_get_string(jctx, "info", _info, sizeof(_info));
    if (err_code == OS_SUCCESS) {
        if (*info) {
            free(*info);
            *info = NULL;
        }

        if (strlen(_info) > 0) {
            /* +1 for NULL termination */
            *info = (char *)MEM_CALLOC_EXTRAM(1, strlen(_info) + 1);
            if (*info) {
                strncpy(*info, _info, strlen(_info));
            }
        }
    }

    err_code = json_obj_get_int(jctx, "flags", &_flags);
    if (err_code == OS_SUCCESS) {
        if (flags) {
            *flags = _flags;
        }
    }

    return ESP_OK;
}

static esp_err_t esp_rmaker_scenes_parse_action(jparse_ctx_t *jctx, esp_rmaker_scene_action_t *action)
{
    int data_len = 0;
    json_obj_get_object_strlen(jctx, "action", &data_len);
    if (data_len <= 0) {
        ESP_LOGD(TAG, "Action not found in JSON");
        return ESP_OK;
    }
    action->data_len = data_len + 1;

    if (action->data) {
        free(action->data);
    }
    action->data = (void *)MEM_CALLOC_EXTRAM(1, action->data_len);
    if (!action->data) {
        ESP_LOGE(TAG, "Could not allocate action");
        return ESP_ERR_NO_MEM;
    }
    json_obj_get_object_str(jctx, "action", action->data, action->data_len);
    return ESP_OK;
}

static esp_rmaker_scene_t *esp_rmaker_scenes_find_or_create(jparse_ctx_t *jctx, char *id, scenes_operation_t operation)
{
    char name[MAX_NAME_LEN + 1] = {0};      /* +1 for NULL termination */
    esp_rmaker_scene_t *scene = NULL;
    if (operation == OPERATION_ADD) {
        /* Checking if scene with same id already exists. */
        scene = esp_rmaker_scenes_get_scene_from_id(id);
        if (scene) {
            ESP_LOGE(TAG, "Scene with id %s already exists. Not adding it again.", id);
            return NULL;
        }

        /* Get name */
        json_obj_get_string(jctx, "name", name, sizeof(name));
        if (strlen(name) <= 0) {
            ESP_LOGE(TAG, "Name not found for scene with id: %s", id);
            return NULL;
        }

        /* This is a new scene. Fill it. */
        scene = (esp_rmaker_scene_t *)MEM_CALLOC_EXTRAM(1, sizeof(esp_rmaker_scene_t));
        if (!scene) {
            ESP_LOGE(TAG, "Couldn't allocate scene with id: %s", id);
            return NULL;
        }
        strlcpy(scene->id, id, sizeof(scene->id));
        strlcpy(scene->name, name, sizeof(scene->name));
    } else {
        /* This scene should already be present */
        scene = esp_rmaker_scenes_get_scene_from_id(id);
        if (!scene) {
            ESP_LOGE(TAG, "Scene with id %s not found", id);
            return NULL;
        }

        /* Get name */
        if (operation == OPERATION_EDIT) {
            json_obj_get_string(jctx, "name", name, sizeof(name));
            if (strlen(name) > 0) {
                /* If there is name in the request, replace the name in the scene with this new one */
                memset(scene->name, 0, sizeof(scene->name));
                strlcpy(scene->name, name, sizeof(scene->name));
            }
        }
    }
    return scene;
}

static esp_err_t esp_rmaker_scenes_perform_operation(esp_rmaker_scene_t *scene, scenes_operation_t operation)
{
    esp_err_t err = ESP_OK;
    switch (operation) {
        case OPERATION_ADD:
            if (scenes_priv_data->total_scenes < MAX_SCENES) {
                err = esp_rmaker_scenes_add_to_list(scene);
            } else {
                ESP_LOGE(TAG, "Max sceness (%d) reached. Not adding this scene with id %s", MAX_SCENES,
                        scene->id);
                err = ESP_FAIL;
            }
            break;

        case OPERATION_EDIT:
            /* Nothing to do here. name, info, action have already been handled */
            break;

        case OPERATION_REMOVE:
            err = esp_rmaker_scenes_remove_from_list(scene);
            if (err == ESP_OK) {
                esp_rmaker_scenes_free(scene);
            }
            break;

        case OPERATION_ACTIVATE:
            err = esp_rmaker_handle_set_params(scene->action.data, scene->action.data_len, ESP_RMAKER_REQ_SRC_SCENE_ACTIVATE);
            break;

        case OPERATION_DEACTIVATE:
            if (scenes_priv_data->deactivate_support) {
                err = esp_rmaker_handle_set_params(scene->action.data, scene->action.data_len, ESP_RMAKER_REQ_SRC_SCENE_DEACTIVATE);
            } else {
                ESP_LOGW(TAG, "Deactivate operation not supported.");
                err = ESP_ERR_NOT_SUPPORTED;
            }
            break;

        default:
            ESP_LOGE(TAG, "Invalid Operation: %d", operation);
            err = ESP_FAIL;
            break;
    }
    return err;
}

static esp_err_t esp_rmaker_scenes_parse_json(void *data, size_t data_len, esp_rmaker_req_src_t src,
                                              bool *report_params)
{
    char id[MAX_ID_LEN + 1] = {0};          /* +1 for NULL termination */
    scenes_operation_t operation = OPERATION_INVALID;
    int current_scene = 0;
    esp_rmaker_scene_t *scene = NULL;

    /* Get details from JSON */
    jparse_ctx_t jctx;
    if (json_parse_start(&jctx, (char *)data, data_len) != 0) {
        ESP_LOGE(TAG, "Json parse start failed");
        return ESP_FAIL;
    }

    /* Parse all scenes */
    while(json_arr_get_object(&jctx, current_scene) == 0) {
        /* Get ID */
        json_obj_get_string(&jctx, "id", id, sizeof(id));
        if (strlen(id) <= 0) {
            ESP_LOGE(TAG, "ID not found in scene JSON");
            goto cleanup;
        }

        /* Get operation */
        if (src == ESP_RMAKER_REQ_SRC_INIT) {
            /* Scene loaded from NVS. Add it */
            operation = OPERATION_ADD;
        } else {
            operation = esp_rmaker_scenes_parse_operation(&jctx, id);
            if (operation == OPERATION_INVALID) {
                ESP_LOGE(TAG, "Error getting operation");
                goto cleanup;
            }
        }

        /* Find/Create new scene */
        scene = esp_rmaker_scenes_find_or_create(&jctx, id, operation);
        if (!scene) {
            goto cleanup;
        }

        /* Get other scene details */
        if (operation == OPERATION_ADD || operation == OPERATION_EDIT) {
            /* Get info and flags */
            esp_rmaker_scenes_parse_info_and_flags(&jctx, &scene->info, &scene->flags);

            /* Get action */
            esp_rmaker_scenes_parse_action(&jctx, &scene->action);
        }

        /* Set report_params */
        if (operation == OPERATION_ADD || operation == OPERATION_EDIT || operation == OPERATION_REMOVE) {
            *report_params = true;
        } else {
            *report_params = false;
        }

        /* Perform operation */
        esp_rmaker_scenes_perform_operation(scene, operation);

cleanup:
        json_arr_leave_object(&jctx);
        current_scene++;
    }
    json_parse_end(&jctx);
    return ESP_OK;
}

static esp_err_t __esp_rmaker_scenes_get_params(char *buf, size_t *buf_size)
{
    esp_err_t err = ESP_OK;
    esp_rmaker_scene_t *scene = scenes_priv_data->scenes_list;
    json_gen_str_t jstr;
    json_gen_str_start(&jstr, buf, *buf_size, NULL, NULL);
    json_gen_start_array(&jstr);

    while (scene) {
        json_gen_start_object(&jstr);

        /* Add details */
        json_gen_obj_set_string(&jstr, "name", scene->name);
        json_gen_obj_set_string(&jstr, "id", scene->id);
       /* If info and flags is not zero, add it. */
        if (scene->info != NULL) {
            json_gen_obj_set_string(&jstr, "info", scene->info);
        }
        if (scene->flags != 0) {
            json_gen_obj_set_int(&jstr, "flags", scene->flags);
        }

        /* Add action */
        json_gen_push_object_str(&jstr, "action", scene->action.data);

        json_gen_end_object(&jstr);

        /* Go to next scene */
        scene = scene->next;
    }

    if (json_gen_end_array(&jstr) < 0) {
        ESP_LOGE(TAG, "Buffer size %d not sufficient for reporting Scenes Params.", *buf_size);
        err = ESP_ERR_NO_MEM;
    }
    *buf_size = json_gen_str_end(&jstr);
    return err;
}

static char *esp_rmaker_scenes_get_params(void)
{
    size_t req_size = 0;
    esp_err_t err = __esp_rmaker_scenes_get_params(NULL, &req_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get required size for scenes JSON.");
        return NULL;
    }
    char *data = MEM_CALLOC_EXTRAM(1, req_size);
    if (!data) {
        ESP_LOGE(TAG, "Failed to allocate %d bytes for scenes.", req_size);
        return NULL;
    }
    err = __esp_rmaker_scenes_get_params(data, &req_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error occured while trying to populate sceness JSON.");
        free(data);
        return NULL;
    }
    return data;
}

static esp_err_t esp_rmaker_scenes_report_params(void)
{
    char *data = esp_rmaker_scenes_get_params();
    esp_rmaker_param_val_t val = {
        .type = RMAKER_VAL_TYPE_ARRAY,
        .val.s = data,
    };
    esp_rmaker_param_t *param = esp_rmaker_device_get_param_by_type(scenes_priv_data->scenes_service, ESP_RMAKER_PARAM_SCENES);
    esp_rmaker_param_update_and_report(param, val);

    free(data);
    return ESP_OK;
}

static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
            const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (strcmp(esp_rmaker_param_get_type(param), ESP_RMAKER_PARAM_SCENES) != 0) {
        ESP_LOGE(TAG, "Got callback for invalid param with name %s and type %s", esp_rmaker_param_get_name(param), esp_rmaker_param_get_type(param));
        return ESP_ERR_INVALID_ARG;
    }
    if (strlen(val.val.s) <= 0) {
        ESP_LOGI(TAG, "Invalid length for params: %d", strlen(val.val.s));
        return ESP_ERR_INVALID_ARG;
    }
    bool report_params = false;
    esp_rmaker_scenes_parse_json(val.val.s, strlen(val.val.s), ctx->src, &report_params);
    if (ctx->src != ESP_RMAKER_REQ_SRC_INIT) {
        /* Since this is a persisting param, we get a write_cb while booting up. We need not report the param when the source is 'init' as this will get reported when the device first reports all the params. */
        if (report_params) {
            /* report_params is only set for add, edit, remove operations. The scenes params are not changed for
            activate, deactivate operations. So need to report the params in that case. */
            esp_rmaker_scenes_report_params();
        }
    }
    return ESP_OK;
}

esp_err_t esp_rmaker_scenes_enable(void)
{
    scenes_priv_data = (esp_rmaker_scenes_priv_data_t *)MEM_CALLOC_EXTRAM(1, sizeof(esp_rmaker_scenes_priv_data_t));
    if (!scenes_priv_data) {
        ESP_LOGE(TAG, "Couldn't allocate scenes_priv_data");
        return ESP_ERR_NO_MEM;
    }

#ifdef CONFIG_ESP_RMAKER_SCENES_DEACTIVATE_SUPPORT
    scenes_priv_data->deactivate_support = CONFIG_ESP_RMAKER_SCENES_DEACTIVATE_SUPPORT;
#endif

    scenes_priv_data->scenes_service = esp_rmaker_create_scenes_service("Scenes", write_cb, NULL, MAX_SCENES, scenes_priv_data->deactivate_support, NULL);
    if (!scenes_priv_data->scenes_service) {
        ESP_LOGE(TAG, "Failed to create Scenes Service");
        return ESP_FAIL;
    }

    esp_err_t err = esp_rmaker_node_add_device(esp_rmaker_get_node(), scenes_priv_data->scenes_service);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add Scenes Service");
        return err;
    }
    ESP_LOGD(TAG, "Scenes Service Enabled");
    return err;
}
