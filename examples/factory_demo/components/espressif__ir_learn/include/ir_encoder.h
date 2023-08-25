/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>
#include "driver/rmt_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_DECODE_MARGIN_US         200 // error margin of 15us

/**
 * @brief Type of IR NEC encoder configuration
 */
typedef struct {
    uint32_t resolution; /*!< Encoder resolution, in Hz */
} ir_nec_encoder_config_t;

/**
 * @brief Create RMT encoder for encoding IR NEC frame into RMT symbols
 *
 * @param[in] config Encoder configuration
 * @param[out] ret_encoder Returned encoder handle
 * @return
 *      - ESP_ERR_INVALID_ARG for any invalid arguments
 *      - ESP_ERR_NO_MEM out of memory when creating IR NEC encoder
 *      - ESP_OK if creating encoder successfully
 */
esp_err_t ir_encoder_new(const ir_nec_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder);

esp_err_t ir_encoder_del(rmt_encoder_t *encoder);

/**
 * @brief Decode RMT symbols into NEC scan code and print the result
 */
esp_err_t ir_encoder_parse_nec_payload(struct ir_learn_sub_list_head *cmd_list);

#ifdef __cplusplus
}
#endif
