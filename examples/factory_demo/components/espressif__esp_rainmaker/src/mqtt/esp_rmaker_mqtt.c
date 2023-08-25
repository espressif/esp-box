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

#include <esp_log.h>
#include <esp_rmaker_mqtt_glue.h>
#include <esp_rmaker_client_data.h>
#include <esp_rmaker_core.h>

#include "esp_rmaker_mqtt.h"
#include "esp_rmaker_mqtt_budget.h"

static const char *TAG = "esp_rmaker_mqtt";
static esp_rmaker_mqtt_config_t g_mqtt_config;

esp_rmaker_mqtt_conn_params_t *esp_rmaker_mqtt_get_conn_params(void)
{
    if (g_mqtt_config.get_conn_params) {
        return g_mqtt_config.get_conn_params();
    } else {
        return esp_rmaker_get_mqtt_conn_params();
    }
}

esp_err_t esp_rmaker_mqtt_setup(esp_rmaker_mqtt_config_t mqtt_config)
{
    g_mqtt_config = mqtt_config;
    g_mqtt_config.setup_done  = true;
    return ESP_OK;
}

esp_err_t esp_rmaker_mqtt_init(esp_rmaker_mqtt_conn_params_t *conn_params)
{
    if (!g_mqtt_config.setup_done) {
        esp_rmaker_mqtt_glue_setup(&g_mqtt_config);
    }
    if (g_mqtt_config.init) {
        esp_err_t err =  g_mqtt_config.init(conn_params);
        if (err == ESP_OK) {
            if (esp_rmaker_mqtt_budgeting_init() != ESP_OK) {
                ESP_LOGE(TAG, "Failied to initialise MQTT Budgeting.");
            }
        }
        return err;
    }
    ESP_LOGW(TAG, "esp_rmaker_mqtt_init not registered");
    return ESP_OK;
}

void esp_rmaker_mqtt_deinit(void)
{
    esp_rmaker_mqtt_budgeting_deinit();
    if (g_mqtt_config.deinit) {
        return g_mqtt_config.deinit();
    }
    ESP_LOGW(TAG, "esp_rmaker_mqtt_deinit not registered");
}

esp_err_t esp_rmaker_mqtt_connect(void)
{
    if (g_mqtt_config.connect) {
        esp_err_t err = g_mqtt_config.connect();
        if (err == ESP_OK) {
            esp_rmaker_mqtt_budgeting_start();
        }
        return err;
    }
    ESP_LOGW(TAG, "esp_rmaker_mqtt_connect not registered");
    return ESP_OK;
}


esp_err_t esp_rmaker_mqtt_disconnect(void)
{
    esp_rmaker_mqtt_budgeting_stop();
    if (g_mqtt_config.disconnect) {
        return g_mqtt_config.disconnect();
    }
    ESP_LOGW(TAG, "esp_rmaker_mqtt_disconnect not registered");
    return ESP_OK;
}

esp_err_t esp_rmaker_mqtt_subscribe(const char *topic, esp_rmaker_mqtt_subscribe_cb_t cb, uint8_t qos, void *priv_data)
{
    if (g_mqtt_config.subscribe) {
        return g_mqtt_config.subscribe(topic, cb, qos, priv_data);
    }
    ESP_LOGW(TAG, "esp_rmaker_mqtt_subscribe not registered");
    return ESP_OK;
}

esp_err_t esp_rmaker_mqtt_unsubscribe(const char *topic)
{
    if (g_mqtt_config.unsubscribe) {
        return g_mqtt_config.unsubscribe(topic);
    }
    ESP_LOGW(TAG, "esp_rmaker_mqtt_unsubscribe not registered");
    return ESP_OK;
}

esp_err_t esp_rmaker_mqtt_publish(const char *topic, void *data, size_t data_len, uint8_t qos, int *msg_id)
{
    if (esp_rmaker_mqtt_is_budget_available() != true) {
        ESP_LOGE(TAG, "Out of MQTT Budget. Dropping publish message.");
        return ESP_FAIL;
    }
    if (g_mqtt_config.publish) {
        esp_err_t err = g_mqtt_config.publish(topic, data, data_len, qos, msg_id);
        if (err == ESP_OK) {
            esp_rmaker_mqtt_decrease_budget(1);
        }
        return err;
    }
    ESP_LOGW(TAG, "esp_rmaker_mqtt_publish not registered");
    return ESP_OK;
}

void esp_rmaker_create_mqtt_topic(char *buf, size_t buf_size, const char *topic_suffix, const char *rule)
{
#ifdef CONFIG_ESP_RMAKER_MQTT_USE_BASIC_INGEST_TOPICS
    snprintf(buf, buf_size, "$aws/rules/%s/node/%s/%s", rule, esp_rmaker_get_node_id(), topic_suffix);
#else
    snprintf(buf, buf_size, "node/%s/%s", esp_rmaker_get_node_id(), topic_suffix);
#endif
}
