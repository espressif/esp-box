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
#include <stdint.h>
#include <string.h>

#include <esp_log.h>

#include <esp_rmaker_factory.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_utils.h>

#include "esp_rmaker_internal.h"
#include "esp_rmaker_client_data.h"

#ifdef CONFIG_ESP_RMAKER_USE_ESP_SECURE_CERT_MGR
    #include "esp_secure_cert_read.h"
    /*
     * Since TAG is not used in any other place in this file at the moment,
     * it has been placed inside this #ifdef to avoid -Werror=unused-variable.
     */
    static const char *TAG = "esp_rmaker_client_data";
#endif

extern uint8_t mqtt_server_root_ca_pem_start[] asm("_binary_rmaker_mqtt_server_crt_start");
extern uint8_t mqtt_server_root_ca_pem_end[] asm("_binary_rmaker_mqtt_server_crt_end");

char * esp_rmaker_get_mqtt_host()
{
    char *host = esp_rmaker_factory_get(ESP_RMAKER_MQTT_HOST_NVS_KEY);
#if defined(CONFIG_ESP_RMAKER_SELF_CLAIM) || defined(CONFIG_ESP_RMAKER_ASSISTED_CLAIM)
    if (!host) {
        return strdup(CONFIG_ESP_RMAKER_MQTT_HOST);
    }
#endif /* defined(CONFIG_ESP_RMAKER_SELF_CLAIM) || defined(CONFIG_ESP_RMAKER_ASSISTED_CLAIM) */
    return host;
}

char * esp_rmaker_get_client_cert()
{
#ifdef CONFIG_ESP_RMAKER_USE_ESP_SECURE_CERT_MGR
    uint32_t client_cert_len = 0;
    char *client_cert_addr = NULL;
    if (esp_secure_cert_get_device_cert(&client_cert_addr, &client_cert_len) == ESP_OK) {
        return client_cert_addr;
    } else {
        ESP_LOGE(TAG, "Failed to obtain flash address of device cert");
        ESP_LOGI(TAG, "Attempting to fetch client certificate from NVS");
    }
#endif /* CONFIG_ESP_RMAKER_USE_ESP_SECURE_CERT_MGR */
    return esp_rmaker_factory_get(ESP_RMAKER_CLIENT_CERT_NVS_KEY);
}

size_t esp_rmaker_get_client_cert_len()
{
#ifdef CONFIG_ESP_RMAKER_USE_ESP_SECURE_CERT_MGR
    uint32_t client_cert_len = 0;
    char *client_cert_addr = NULL;
    if (esp_secure_cert_get_device_cert(&client_cert_addr, &client_cert_len) == ESP_OK) {
        return client_cert_len;
    } else {
        ESP_LOGE(TAG, "Failed to obtain flash address of device cert");
        ESP_LOGI(TAG, "Attempting to fetch client certificate from NVS");
    }
#endif /* CONFIG_ESP_RMAKER_USE_ESP_SECURE_CERT_MGR */
    return esp_rmaker_factory_get_size(ESP_RMAKER_CLIENT_CERT_NVS_KEY) + 1; /* +1 for NULL terminating byte */
}

char * esp_rmaker_get_client_key()
{
#ifdef CONFIG_ESP_RMAKER_USE_ESP_SECURE_CERT_MGR
    uint32_t client_key_len = 0;
    char *client_key_addr = NULL;
    if (esp_secure_cert_get_priv_key(&client_key_addr, &client_key_len) == ESP_OK) {
        return client_key_addr;
    } else {
        ESP_LOGE(TAG, "Failed to obtain flash address of private_key");
        ESP_LOGI(TAG, "Attempting to fetch key from NVS");
    }
#endif /* CONFIG_ESP_RMAKER_USE_ESP_SECURE_CERT_MGR */
    return esp_rmaker_factory_get(ESP_RMAKER_CLIENT_KEY_NVS_KEY);
}

size_t esp_rmaker_get_client_key_len()
{
#ifdef CONFIG_ESP_RMAKER_USE_ESP_SECURE_CERT_MGR
    uint32_t client_key_len = 0;
    char *client_key_addr = NULL;
    if (esp_secure_cert_get_priv_key(&client_key_addr, &client_key_len) == ESP_OK) {
        return client_key_len;
    } else {
        ESP_LOGE(TAG, "Failed to obtain flash address of private_key");
        ESP_LOGI(TAG, "Attempting to fetch key from NVS");
    }
#endif /* CONFIG_ESP_RMAKER_USE_ESP_SECURE_CERT_MGR */
    return esp_rmaker_factory_get_size(ESP_RMAKER_CLIENT_KEY_NVS_KEY) + 1; /* +1 for NULL terminating byte */
}

char * esp_rmaker_get_client_csr()
{
    return esp_rmaker_factory_get(ESP_RMAKER_CLIENT_CSR_NVS_KEY);
}

esp_rmaker_mqtt_conn_params_t *esp_rmaker_get_mqtt_conn_params()
{
    esp_rmaker_mqtt_conn_params_t *mqtt_conn_params = MEM_CALLOC_EXTRAM(1, sizeof(esp_rmaker_mqtt_conn_params_t));
    
#if defined(CONFIG_ESP_RMAKER_USE_ESP_SECURE_CERT_MGR) && defined(CONFIG_ESP_SECURE_CERT_DS_PERIPHERAL)
    mqtt_conn_params->ds_data = esp_secure_cert_get_ds_ctx();
    if (mqtt_conn_params->ds_data == NULL) /* Get client key only if ds_data is NULL */
#endif /* (defined(CONFIG_ESP_RMAKER_USE_ESP_SECURE_CERT_MGR) && defined(CONFIG_ESP_SECURE_CERT_DS_PERIPHERAL)) */
    {
        if ((mqtt_conn_params->client_key = esp_rmaker_get_client_key()) == NULL) {
            goto init_err;
        }
        mqtt_conn_params->client_key_len = esp_rmaker_get_client_key_len();
    }
    if ((mqtt_conn_params->client_cert = esp_rmaker_get_client_cert()) == NULL) {
        goto init_err;
    }
        mqtt_conn_params->client_cert_len = esp_rmaker_get_client_cert_len();
    if ((mqtt_conn_params->mqtt_host = esp_rmaker_get_mqtt_host()) == NULL) {
        goto init_err;
    }
    mqtt_conn_params->server_cert = (char *)mqtt_server_root_ca_pem_start;
    mqtt_conn_params->client_id = esp_rmaker_get_node_id();
    return mqtt_conn_params;
init_err:
    esp_rmaker_clean_mqtt_conn_params(mqtt_conn_params);
    free(mqtt_conn_params);
    return NULL;
}

void esp_rmaker_clean_mqtt_conn_params(esp_rmaker_mqtt_conn_params_t *mqtt_conn_params)
{
    if (mqtt_conn_params) {
        if (mqtt_conn_params->mqtt_host) {
            free(mqtt_conn_params->mqtt_host);
        }
#ifdef CONFIG_ESP_RMAKER_USE_ESP_SECURE_CERT_MGR
        if (mqtt_conn_params->client_cert) {
            esp_secure_cert_free_device_cert(mqtt_conn_params->client_cert);
        }
#ifdef CONFIG_ESP_SECURE_CERT_DS_PERIPHERAL
        if (mqtt_conn_params->ds_data) {
            esp_secure_cert_free_ds_ctx(mqtt_conn_params->ds_data);
        }
#else  /* !CONFIG_ESP_SECURE_CERT_DS_PERIPHERAL */
        if (mqtt_conn_params->client_key) {
            esp_secure_cert_free_priv_key(mqtt_conn_params->client_key);
        }
#endif /* CONFIG_ESP_SECURE_CERT_DS_PERIPHERAL */
#else  /* !CONFIG_ESP_RMAKER_USE_ESP_SECURE_CERT_MGR */
        if (mqtt_conn_params->client_cert) {
            free(mqtt_conn_params->client_cert);
        }
        if (mqtt_conn_params->client_key) {
            free(mqtt_conn_params->client_key);
        }
#endif /* CONFIG_ESP_RMAKER_USE_ESP_SECURE_CERT_MGR */
    }
}
