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

#include <stdlib.h>
#include <inttypes.h>
#include <esp_log.h>
#include <nvs.h>
#include <esp_event.h>
#include <esp_local_ctrl.h>
#include <wifi_provisioning/manager.h>
#include <esp_rmaker_internal.h>
#include <esp_rmaker_standard_services.h>
#include <esp_https_server.h>
#include <mdns.h>
#include <esp_rmaker_utils.h>

#include <esp_idf_version.h>
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 2, 0)
// Features supported in 4.2

#define ESP_RMAKER_LOCAL_CTRL_SECURITY_TYPE CONFIG_ESP_RMAKER_LOCAL_CTRL_SECURITY

#else

#if CONFIG_ESP_RMAKER_LOCAL_CTRL_SECURITY != 0
#warning "Local control security type is not supported in idf versions below 4.2. Using sec0 by default."
#endif
#define ESP_RMAKER_LOCAL_CTRL_SECURITY_TYPE 0

#endif /* !IDF4.2 */

static const char * TAG = "esp_rmaker_local";

/* Random Port number that will be used by the local control http instance
 * for internal control communication.
 */
#define ESP_RMAKER_LOCAL_CTRL_HTTP_CTRL_PORT    12312
#define ESP_RMAKER_NVS_PART_NAME                "nvs"
#define ESP_RMAKER_NVS_LOCAL_CTRL_NAMESPACE     "local_ctrl"
#define ESP_RMAKER_NVS_LOCAL_CTRL_POP           "pop"
#define ESP_RMAKER_POP_LEN    9

/* Custom allowed property types */
enum property_types {
    PROP_TYPE_NODE_CONFIG = 1,
    PROP_TYPE_NODE_PARAMS,
};

/* Custom flags that can be set for a property */
enum property_flags {
    PROP_FLAG_READONLY = (1 << 0)
};

static bool g_local_ctrl_is_started = false;

static char *g_serv_name;
static bool wait_for_wifi_prov;
/********* Handler functions for responding to control requests / commands *********/

static esp_err_t get_property_values(size_t props_count,
                                     const esp_local_ctrl_prop_t props[],
                                     esp_local_ctrl_prop_val_t prop_values[],
                                     void *usr_ctx)
{
    esp_err_t ret = ESP_OK;
    uint32_t i;
    for (i = 0; i < props_count && ret == ESP_OK ; i++) {
        ESP_LOGD(TAG, "(%"PRIu32") Reading property : %s", i, props[i].name);
        switch (props[i].type) {
            case PROP_TYPE_NODE_CONFIG: {
                char *node_config = esp_rmaker_get_node_config();
                if (!node_config) {
                    ESP_LOGE(TAG, "Failed to allocate memory for %s", props[i].name);
                    ret = ESP_ERR_NO_MEM;
                } else {
                    prop_values[i].size = strlen(node_config);
                    prop_values[i].data = node_config;
                    prop_values[i].free_fn = free;
                }
                break;
            }
            case PROP_TYPE_NODE_PARAMS: {
                char *node_params = esp_rmaker_get_node_params();
                if (!node_params) {
                    ESP_LOGE(TAG, "Failed to allocate memory for %s", props[i].name);
                    ret = ESP_ERR_NO_MEM;
                } else {
                    prop_values[i].size = strlen(node_params);
                    prop_values[i].data = node_params;
                    prop_values[i].free_fn = free;
                }
                break;
            }
            default:
                break;
        }
    }
    if (ret != ESP_OK) {
        for (uint32_t j = 0; j <= i; j++) {
            if (prop_values[j].free_fn) {
                ESP_LOGI(TAG, "Freeing memory for %s", props[j].name);
                prop_values[j].free_fn(prop_values[j].data);
                prop_values[j].free_fn = NULL;
                prop_values[j].data = NULL;
                prop_values[j].size = 0;
            }
        }
    }
    return ret;
}

static esp_err_t set_property_values(size_t props_count,
                                     const esp_local_ctrl_prop_t props[],
                                     const esp_local_ctrl_prop_val_t prop_values[],
                                     void *usr_ctx)
{
    esp_err_t ret = ESP_OK;
    uint32_t i;
    /* First check if any of the properties are read-only properties. If found, just abort */
    for (i = 0; i < props_count; i++) {
        /* Cannot set the value of a read-only property */
        if (props[i].flags & PROP_FLAG_READONLY) {
            ESP_LOGE(TAG, "%s is read-only", props[i].name);
            return ESP_ERR_INVALID_ARG;
        }
    }
    for (i = 0; i < props_count && ret == ESP_OK; i++) {
        switch (props[i].type) {
            case PROP_TYPE_NODE_PARAMS:
                ret = esp_rmaker_handle_set_params((char *)prop_values[i].data,
                        prop_values[i].size, ESP_RMAKER_REQ_SRC_LOCAL);
                break;
            default:
                break;
        }
    }
    return ret;
}

static char *__esp_rmaker_local_ctrl_get_nvs(const char *key)
{
    char *val = NULL;
    nvs_handle handle;
    esp_err_t err = nvs_open_from_partition(ESP_RMAKER_NVS_PART_NAME, ESP_RMAKER_NVS_LOCAL_CTRL_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return NULL;
    }
    size_t len = 0;
    if ((err = nvs_get_blob(handle, key, NULL, &len)) == ESP_OK) {
        val = MEM_CALLOC_EXTRAM(1, len + 1); /* +1 for NULL termination */
        if (val) {
            nvs_get_blob(handle, key, val, &len);
        }
    }
    nvs_close(handle);
    return val;

}

static esp_err_t __esp_rmaker_local_ctrl_set_nvs(const char *key, const char *val)
{
    nvs_handle handle;
    esp_err_t err = nvs_open_from_partition(ESP_RMAKER_NVS_PART_NAME, ESP_RMAKER_NVS_LOCAL_CTRL_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_set_blob(handle, key, val, strlen(val));
    nvs_commit(handle);
    nvs_close(handle);
    return err;
}

static char *esp_rmaker_local_ctrl_get_pop()
{
    char *pop = __esp_rmaker_local_ctrl_get_nvs(ESP_RMAKER_NVS_LOCAL_CTRL_POP);
    if (pop) {
        return pop;
    }

    ESP_LOGI(TAG, "Couldn't find POP in NVS. Generating a new one.");
    pop = (char *)MEM_CALLOC_EXTRAM(1, ESP_RMAKER_POP_LEN);
    if (!pop) {
        ESP_LOGE(TAG, "Couldn't allocate POP");
        return NULL;
    }
    uint8_t random_bytes[ESP_RMAKER_POP_LEN] = {0};
    esp_fill_random(&random_bytes, sizeof(random_bytes));
    snprintf(pop, ESP_RMAKER_POP_LEN, "%02x%02x%02x%02x", random_bytes[0], random_bytes[1], random_bytes[2], random_bytes[3]);

    __esp_rmaker_local_ctrl_set_nvs(ESP_RMAKER_NVS_LOCAL_CTRL_POP, pop);
    return pop;
}

static int esp_rmaker_local_ctrl_get_security_type()
{
    return ESP_RMAKER_LOCAL_CTRL_SECURITY_TYPE;
}

static esp_err_t esp_rmaker_local_ctrl_service_enable(void)
{
    char *pop_str = esp_rmaker_local_ctrl_get_pop();
    if (!pop_str) {
        ESP_LOGE(TAG, "Get POP failed");
        return ESP_FAIL;
    }
    int sec_ver = esp_rmaker_local_ctrl_get_security_type();

    esp_rmaker_device_t *local_ctrl_service = esp_rmaker_create_local_control_service("Local Control", pop_str, sec_ver, NULL);
    if (!local_ctrl_service) {
        ESP_LOGE(TAG, "Failed to create Schedule Service");
        free(pop_str);
        return ESP_FAIL;
    }
    free(pop_str);

    esp_err_t err = esp_rmaker_node_add_device(esp_rmaker_get_node(), local_ctrl_service);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add service Service");
        return err;
    }

    ESP_LOGD(TAG, "Local Control Service Enabled");
    return err;
}

static esp_err_t __esp_rmaker_start_local_ctrl_service(const char *serv_name)
{
    if (!serv_name) {
        ESP_LOGE(TAG, "Service name cannot be empty.");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Starting ESP Local control with HTTP Transport and security version: %d", esp_rmaker_local_ctrl_get_security_type());
    /* Set the configuration */
    static httpd_ssl_config_t https_conf = HTTPD_SSL_CONFIG_DEFAULT();
    https_conf.transport_mode = HTTPD_SSL_TRANSPORT_INSECURE;
    https_conf.port_insecure = CONFIG_ESP_RMAKER_LOCAL_CTRL_HTTP_PORT;
    https_conf.httpd.ctrl_port = ESP_RMAKER_LOCAL_CTRL_HTTP_CTRL_PORT;

    mdns_init();
    mdns_hostname_set(serv_name);

    esp_local_ctrl_config_t config = {
        .transport = ESP_LOCAL_CTRL_TRANSPORT_HTTPD,
        .transport_config = {
            .httpd = &https_conf
        },
        .handlers = {
            /* User defined handler functions */
            .get_prop_values = get_property_values,
            .set_prop_values = set_property_values,
            .usr_ctx         = NULL,
            .usr_ctx_free_fn = NULL
        },
        /* Maximum number of properties that may be set */
        .max_properties = 10
    };

    /* If sec1, add security type details to the config */
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#define PROTOCOMM_SEC_DATA protocomm_security1_params_t
#else
#define PROTOCOMM_SEC_DATA  protocomm_security_pop_t
#endif /* ESP_IDF_VERSION */
    PROTOCOMM_SEC_DATA *pop = NULL;
#if ESP_RMAKER_LOCAL_CTRL_SECURITY_TYPE == 1
        char *pop_str = esp_rmaker_local_ctrl_get_pop();
        /* Note: pop_str shouldn't be freed. If it gets freed, the pointer which is internally copied in esp_local_ctrl_start() will become invalid which would cause corruption. */

        int sec_ver = esp_rmaker_local_ctrl_get_security_type();

        if (sec_ver != 0 && pop_str) {
            pop = (PROTOCOMM_SEC_DATA *)MEM_CALLOC_EXTRAM(1, sizeof(PROTOCOMM_SEC_DATA));
            if (!pop) {
                ESP_LOGE(TAG, "Failed to allocate pop");
                free(pop_str);
                return ESP_ERR_NO_MEM;
            }
            pop->data = (uint8_t *)pop_str;
            pop->len = strlen(pop_str);
        }

        config.proto_sec.version = sec_ver;
        config.proto_sec.custom_handle = NULL;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        config.proto_sec.sec_params = pop;
#else
        config.proto_sec.pop = pop;
#endif /* ESP_IDF_VERSION */
#endif

    /* Start esp_local_ctrl service */
    ESP_ERROR_CHECK(esp_local_ctrl_start(&config));

    /* Add node_id in mdns */
    mdns_service_txt_item_set("_esp_local_ctrl", "_tcp", "node_id", esp_rmaker_get_node_id());

    if (pop) {
        free(pop);
    }

    ESP_LOGI(TAG, "esp_local_ctrl service started with name : %s", serv_name);

    /* Create the Node Config property */
    esp_local_ctrl_prop_t node_config = {
        .name        = "config",
        .type        = PROP_TYPE_NODE_CONFIG,
        .size        = 0,
        .flags       = PROP_FLAG_READONLY,
        .ctx         = NULL,
        .ctx_free_fn = NULL
    };

    /* Create the Node Params property */
    esp_local_ctrl_prop_t node_params = {
        .name        = "params",
        .type        = PROP_TYPE_NODE_PARAMS,
        .size        = 0,
        .flags       = 0,
        .ctx         = NULL,
        .ctx_free_fn = NULL
    };

    /* Now register the properties */
    ESP_ERROR_CHECK(esp_local_ctrl_add_property(&node_config));
    ESP_ERROR_CHECK(esp_local_ctrl_add_property(&node_params));

    /* update the global status */
    g_local_ctrl_is_started = true;
    esp_rmaker_post_event(RMAKER_EVENT_LOCAL_CTRL_STARTED, (void *)serv_name, strlen(serv_name) + 1);
    return ESP_OK;
}

static void esp_rmaker_local_ctrl_prov_event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "Event %"PRIu32, event_id);
    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
            case WIFI_PROV_START:
                wait_for_wifi_prov = true;
                break;
            case WIFI_PROV_DEINIT:
                if (wait_for_wifi_prov == true) {
                    wait_for_wifi_prov = false;
                    if (g_serv_name) {
                        __esp_rmaker_start_local_ctrl_service(g_serv_name);
                        free(g_serv_name);
                        g_serv_name = NULL;
                    }
                }
                esp_event_handler_unregister(WIFI_PROV_EVENT, WIFI_PROV_START, &esp_rmaker_local_ctrl_prov_event_handler);
                esp_event_handler_unregister(WIFI_PROV_EVENT, WIFI_PROV_DEINIT, &esp_rmaker_local_ctrl_prov_event_handler);
                break;
            default:
                break;
        }
    }
}

bool esp_rmaker_local_ctrl_service_started(void)
{
    return g_local_ctrl_is_started;
}

esp_err_t esp_rmaker_init_local_ctrl_service(void)
{
    /* ESP Local Control uses protocomm_httpd, which is also used by SoftAP Provisioning.
     * If local control is started before provisioning ends, it fails because only one protocomm_httpd
     * instance is allowed at a time.
     * So, we check for the WIFI_PROV_START event, and if received, wait for the WIFI_PROV_DEINIT event
     * before starting local control.
     * This would not be required in case of BLE Provisioning, but this code has no easy way of knowing
     * what provisioning transport is being used and hence this logic will come into picture for both,
     * SoftAP and BLE provisioning.
     */
    esp_event_handler_register(WIFI_PROV_EVENT, WIFI_PROV_START, &esp_rmaker_local_ctrl_prov_event_handler, NULL);
    esp_event_handler_register(WIFI_PROV_EVENT, WIFI_PROV_DEINIT, &esp_rmaker_local_ctrl_prov_event_handler, NULL);
    return ESP_OK;
}

esp_err_t esp_rmaker_start_local_ctrl_service(const char *serv_name)
{
    if (ESP_RMAKER_LOCAL_CTRL_SECURITY_TYPE == 1) {
        esp_rmaker_local_ctrl_service_enable();
    }

    if (!wait_for_wifi_prov) {
        return __esp_rmaker_start_local_ctrl_service(serv_name);
    }

    ESP_LOGI(TAG, "Waiting for Wi-Fi provisioning to finish.");
    g_serv_name = strdup(serv_name);
    if (g_serv_name) {
        return ESP_OK;
    }
    return ESP_ERR_NO_MEM;
}
