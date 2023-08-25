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
#pragma once
#include <stdint.h>
#include <esp_rmaker_mqtt_glue.h>

#define ESP_RMAKER_CLIENT_CERT_NVS_KEY   "client_cert"
#define ESP_RMAKER_CLIENT_KEY_NVS_KEY    "client_key"
#define ESP_RMAKER_MQTT_HOST_NVS_KEY     "mqtt_host"
#define ESP_RMAKER_CLIENT_CSR_NVS_KEY    "csr"
#define ESP_RMAKER_CLIENT_RANDOM_NVS_KEY "random"

char *esp_rmaker_get_client_cert();
char *esp_rmaker_get_client_key();
char *esp_rmaker_get_client_csr();
char *esp_rmaker_get_mqtt_host();
esp_rmaker_mqtt_conn_params_t *esp_rmaker_get_mqtt_conn_params();
void esp_rmaker_clean_mqtt_conn_params(esp_rmaker_mqtt_conn_params_t *mqtt_conn_params);
