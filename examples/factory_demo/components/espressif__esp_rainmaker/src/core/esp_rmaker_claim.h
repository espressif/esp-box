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
#include <mbedtls/pk.h>
#include <esp_err.h>
#define MAX_CSR_SIZE        1024
#define MAX_PAYLOAD_SIZE    3072

typedef enum {
    RMAKER_CLAIM_STATE_PK_GENERATED = 1,
    RMAKER_CLAIM_STATE_INIT,
    RMAKER_CLAIM_STATE_INIT_DONE,
    RMAKER_CLAIM_STATE_CSR_GENERATED,
    RMAKER_CLAIM_STATE_VERIFY,
    RMAKER_CLAIM_STATE_VERIFY_DONE,
} esp_rmaker_claim_state_t;

typedef struct {
    esp_rmaker_claim_state_t state;
    unsigned char csr[MAX_CSR_SIZE];
    char payload[MAX_PAYLOAD_SIZE];
    size_t payload_offset;
    size_t payload_len;
    mbedtls_pk_context key;
} esp_rmaker_claim_data_t;

#ifdef CONFIG_ESP_RMAKER_SELF_CLAIM
esp_rmaker_claim_data_t * esp_rmaker_self_claim_init(void);
esp_err_t esp_rmaker_self_claim_perform(esp_rmaker_claim_data_t *claim_data);
#endif
#ifdef CONFIG_ESP_RMAKER_ASSISTED_CLAIM
esp_rmaker_claim_data_t * esp_rmaker_assisted_claim_init(void);
esp_err_t esp_rmaker_assisted_claim_perform(esp_rmaker_claim_data_t *claim_data);
#endif

void esp_rmaker_claim_data_free(esp_rmaker_claim_data_t *claim_data);

