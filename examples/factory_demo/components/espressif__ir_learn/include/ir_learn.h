/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>
#include <sys/queue.h>
#include <sys/queue.h>
#include "driver/rmt_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Type of IR learn handle
 */
typedef struct ir_learn_t *ir_learn_handle_t;

typedef enum {
    IR_LEARN_STATE_STEP,

    IR_LEARN_STATE_READY = 20,
    IR_LEARN_STATE_END,
    IR_LEARN_STATE_FAIL,
} ir_learn_state_t;

typedef struct ir_learn_sub_list_t {
    uint32_t timediff;
    rmt_rx_done_event_data_t symbols;     /*!< received RMT symbols */
    SLIST_ENTRY(ir_learn_sub_list_t) next;
} ir_learn_sub_list_t;

typedef struct ir_learn_list_t {
    SLIST_HEAD(ir_learn_sub_list_head, ir_learn_sub_list_t) cmd_sub_node;
    SLIST_ENTRY(ir_learn_list_t) next;
} ir_learn_list_t;

SLIST_HEAD(ir_learn_list_head, ir_learn_list_t);

typedef void (*ir_learn_result_cb)(ir_learn_state_t, uint8_t, struct ir_learn_sub_list_head *);

/**
 * @brief IR learn configuration
 */
typedef struct {
    int learn_count;
    int learn_gpio; /*!< GPIO number that consumed by the sensor */
    rmt_clock_source_t clk_src; /*!< RMT clock source */

    ir_learn_result_cb callback;
    int task_priority;      /*!< ir learn task priority */
    int task_stack;         /*!< ir learn task stack size */
    int task_affinity;      /*!< ir learn task pinned to core (-1 is no affinity) */
} ir_learn_cfg_t;

esp_err_t ir_learn_restart(ir_learn_handle_t ir_learn_hdl);

esp_err_t ir_learn_stop(void);

esp_err_t ir_learn_new(const ir_learn_cfg_t *cfg, ir_learn_handle_t *ret_ir_learn_hdl);

esp_err_t ir_learn_del(ir_learn_handle_t ir_learn_hdl);

esp_err_t ir_learn_add_symbol(const rmt_rx_done_event_data_t add_symbol);

esp_err_t ir_learn_remove_all_symbol(void);

esp_err_t ir_learn_add_list_node(struct ir_learn_list_head *learn_head);

esp_err_t ir_learn_add_sub_list_node(struct ir_learn_sub_list_head *sub_head, uint32_t timediff, const rmt_rx_done_event_data_t *add_symbol);

esp_err_t ir_learn_check_valid(struct ir_learn_list_head *learn_head, struct ir_learn_sub_list_head *learn_result);

void ir_learn_clean_sub_data(struct ir_learn_sub_list_head *learn_cmd);

void ir_learn_clean_data(struct ir_learn_list_head *learn_history);

#ifdef __cplusplus
}
#endif