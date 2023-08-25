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
#include <esp_err.h>
#include <esp_rmaker_ota.h>

#define RMAKER_OTA_NVS_NAMESPACE            "rmaker_ota"
#define RMAKER_OTA_JOB_ID_NVS_NAME          "rmaker_ota_id"
#define RMAKER_OTA_UPDATE_FLAG_NVS_NAME     "ota_update"
#define RMAKER_OTA_FETCH_DELAY              5

typedef struct {
    esp_rmaker_ota_type_t type;
    esp_rmaker_ota_cb_t ota_cb;
    void *priv;
    const char *server_cert;
    char *url;
    char *fw_version;
    int filesize;
    bool ota_in_progress;
    bool rolled_back;
    ota_status_t last_reported_status;
    void *transient_priv;
    char *metadata;
} esp_rmaker_ota_t;

char *esp_rmaker_ota_status_to_string(ota_status_t status);
void esp_rmaker_ota_common_cb(void *priv);
void esp_rmaker_ota_finish_using_params(esp_rmaker_ota_t *ota);
void esp_rmaker_ota_finish_using_topics(esp_rmaker_ota_t *ota);
esp_err_t esp_rmaker_ota_enable_using_params(esp_rmaker_ota_t *ota);
esp_err_t esp_rmaker_ota_report_status_using_params(esp_rmaker_ota_handle_t ota_handle,
            ota_status_t status, char *additional_info);
esp_err_t esp_rmaker_ota_enable_using_topics(esp_rmaker_ota_t *ota);
esp_err_t esp_rmaker_ota_report_status_using_topics(esp_rmaker_ota_handle_t ota_handle,
        ota_status_t status, char *additional_info);
