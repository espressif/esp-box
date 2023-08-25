/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <sys/queue.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_timer.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"

#include "driver/rmt_tx.h"
#include "driver/rmt_rx.h"

#include "ir_learn.h"
#include "ir_encoder.h"
#include "ir_learn_prv.h"

static const char *TAG = "ir learn";

#define NEC_IR_RESOLUTION_HZ               (1 * 1000 * 1000) // RMT channel default resolution for 1-wire bus, 1MHz, 1tick = 1us

// the memory size of each RMT channel, in words (4 bytes)
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
#define NEC_RMT_DEFAULT_MEM_BLOCK_SYMBOLS   64
#else
#define NEC_RMT_DEFAULT_MEM_BLOCK_SYMBOLS   128*4
#endif

#define NEC_RMT_RX_MEM_BLOCK_SIZE           NEC_RMT_DEFAULT_MEM_BLOCK_SYMBOLS

typedef struct ir_learn_t {
    rmt_channel_handle_t channel_rx;        /*!< rmt rx channel handler */
    rmt_rx_done_event_data_t rmt_rx;     /*!< received RMT symbols */

    struct ir_learn_list_head learn_list;
    struct ir_learn_sub_list_head learn_result;

    QueueHandle_t   receive_queue;          /*!< A queue used to send the raw data to the task from the ISR */
    bool            running;
    uint32_t        pre_time;

    uint8_t        learn_count;
    uint8_t        learned_count;
    uint8_t        learned_sub;
} ir_learn_t;

const static rmt_receive_config_t ir_learn_rmt_rx_cfg = {
    .signal_range_min_ns = 1000,
    .signal_range_max_ns = 20 * 1000 * 1000,
};

static ir_learn_t *ir_learn_ctx = NULL;

static bool ir_learn_rx_done_callback(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data)
{
    BaseType_t task_woken = pdFALSE;
    ir_learn_t *ir_learn = (ir_learn_t *)user_data;

    xQueueSendFromISR(ir_learn->receive_queue, edata, &task_woken);

    return task_woken;
}

static esp_err_t ir_learn_destroy(ir_learn_t *ir_learn)
{
    ir_learn_remove_all_symbol();

    if (ir_learn->channel_rx) {
        rmt_disable(ir_learn->channel_rx);
        rmt_del_channel(ir_learn->channel_rx);
    }

    if (ir_learn->receive_queue) {
        vQueueDelete(ir_learn->receive_queue);
    }

    if (ir_learn->rmt_rx.received_symbols) {
        free(ir_learn->rmt_rx.received_symbols);
    }

    free(ir_learn);

    return ESP_OK;
}

void ir_learn_clean_sub_data(struct ir_learn_sub_list_head *learn_cmd)
{
    ir_learn_sub_list_t *result_it;

    while (!SLIST_EMPTY(learn_cmd)) {
        result_it = SLIST_FIRST(learn_cmd);
        if (result_it->symbols.received_symbols) {
            heap_caps_free(result_it->symbols.received_symbols);
        }
        SLIST_REMOVE_HEAD(learn_cmd, next);
        if (result_it) {
            heap_caps_free(result_it);
        }
    }
    SLIST_INIT(learn_cmd);
    return;
}

void ir_learn_clean_data(struct ir_learn_list_head *learn_history)
{
    ir_learn_list_t *learn_list;

    while (!SLIST_EMPTY(learn_history)) {
        learn_list = SLIST_FIRST(learn_history);

        ir_learn_clean_sub_data(&learn_list->cmd_sub_node);

        SLIST_REMOVE_HEAD(learn_history, next);
        if (learn_list) {
            heap_caps_free(learn_list);
        }
    }
    SLIST_INIT(learn_history);
    return;
}

/*******************************************************************************
* Private functions
*******************************************************************************/

static void ir_learn_task(void *arg)
{
    int64_t cur_time;
    size_t period;

    ir_learn_result_cb ir_learn_user_callback = (ir_learn_result_cb) arg;

    ESP_LOGI(TAG, "start ir learn task");
    rmt_rx_done_event_data_t learn_data;

    ir_learn_ctx->running = true;
    ir_learn_ctx->learned_count = 0;
    ir_learn_ctx->learned_sub = 0;

    rmt_receive(ir_learn_ctx->channel_rx, ir_learn_ctx->rmt_rx.received_symbols, ir_learn_ctx->rmt_rx.num_symbols, &ir_learn_rmt_rx_cfg);

    while (ir_learn_ctx->running) {
        if (xQueueReceive(ir_learn_ctx->receive_queue, &learn_data, pdMS_TO_TICKS(500)) == pdPASS) {

            if (learn_data.num_symbols < 5) {
                rmt_receive(ir_learn_ctx->channel_rx, \
                            ir_learn_ctx->rmt_rx.received_symbols, ir_learn_ctx->rmt_rx.num_symbols, &ir_learn_rmt_rx_cfg);
                continue;
            }

            cur_time = esp_timer_get_time();
            period   = cur_time - ir_learn_ctx->pre_time;
            ir_learn_ctx->pre_time = esp_timer_get_time();

            if ((period < 500 * 1000)) {
                // ESP_LOGI(TAG, "sub symbol:%u ms, %d", period / 1000, learn_data.num_symbols);
                ir_learn_ctx->learned_sub++;
            } else {
                // ESP_LOGI(TAG, "new symbol:%u ms, %d", period / 1000, learn_data.num_symbols);
                period = 0;
                ir_learn_ctx->learned_sub = 1;
                ir_learn_ctx->learned_count++;
            }

            if (ir_learn_ctx->learned_count <= ir_learn_ctx->learn_count) {
                if (1 == ir_learn_ctx->learned_sub) {
                    ir_learn_add_list_node(&ir_learn_ctx->learn_list);
                }

                ir_learn_list_t *main_it;
                ir_learn_list_t *last = SLIST_FIRST(&ir_learn_ctx->learn_list);
                while ((main_it = SLIST_NEXT(last, next)) != NULL) {
                    last = main_it;
                }

                ir_learn_add_sub_list_node(&last->cmd_sub_node, period, &learn_data);
                if (ir_learn_user_callback) {
                    ir_learn_user_callback(ir_learn_ctx->learned_count, ir_learn_ctx->learned_sub, &last->cmd_sub_node);
                }
            }

            rmt_receive(ir_learn_ctx->channel_rx, \
                        ir_learn_ctx->rmt_rx.received_symbols, ir_learn_ctx->rmt_rx.num_symbols, &ir_learn_rmt_rx_cfg);
        } else if (ir_learn_ctx->learned_sub) {
            ir_learn_ctx->learned_sub = 0;

            if (ir_learn_ctx->learned_count == ir_learn_ctx->learn_count) {
                if (ir_learn_user_callback) {
                    if (ESP_OK == ir_learn_check_valid(&ir_learn_ctx->learn_list, &ir_learn_ctx->learn_result)) {
                        ir_learn_user_callback(IR_LEARN_STATE_END, 0, &ir_learn_ctx->learn_result);
                    } else {
                        ir_learn_user_callback(IR_LEARN_STATE_FAIL, 0, NULL);
                    }
                }
            }
        }
    }

    ESP_LOGI(TAG, "delete ir learn task");
    ir_learn_del(ir_learn_ctx);

    /* Close task */
    vTaskDelete(NULL);
}

esp_err_t ir_learn_restart(ir_learn_handle_t ir_learn_hdl)
{
    assert(ir_learn_hdl && "ir_learn_hdl is null");

    ir_learn_remove_all_symbol();
    ir_learn_hdl->learned_count = 0;

    return ESP_OK;
}

esp_err_t ir_learn_stop(void)
{
    /* Stop running task */
    if (ir_learn_ctx) {
        ir_learn_ctx->running = false;
    } else {
        ESP_LOGW(TAG, "learn task not executed");
    }

    return ESP_OK;
}

esp_err_t ir_learn_add_sub_list_node(struct ir_learn_sub_list_head *sub_head, uint32_t timediff, const rmt_rx_done_event_data_t *add_symbol)
{
    esp_err_t ret = ESP_OK;

    ir_learn_sub_list_t *item = (ir_learn_sub_list_t *)malloc(sizeof(ir_learn_sub_list_t));
    ESP_GOTO_ON_FALSE(NULL != item, ESP_ERR_NO_MEM, err, TAG, "no mem to store received RMT symbols");

    item->timediff = timediff;
    item->symbols.num_symbols = add_symbol->num_symbols;
    item->symbols.received_symbols = malloc(add_symbol->num_symbols * sizeof(rmt_symbol_word_t));
    ESP_GOTO_ON_FALSE(item->symbols.received_symbols, ESP_ERR_NO_MEM, err, TAG, "no mem to store received RMT symbols");

    memcpy(item->symbols.received_symbols, add_symbol->received_symbols, add_symbol->num_symbols * sizeof(rmt_symbol_word_t));
    item->next.sle_next = NULL;

    ir_learn_sub_list_t *last = SLIST_FIRST(sub_head);
    if (last == NULL) {
        SLIST_INSERT_HEAD(sub_head, item, next);
    } else {
        ir_learn_sub_list_t *sub_it;
        while ((sub_it = SLIST_NEXT(last, next)) != NULL) {
            last = sub_it;
        }
        SLIST_INSERT_AFTER(last, item, next);
    }
    return ret;

err:
    if (item) {
        free(item);
    }

    if (item->symbols.received_symbols) {
        free(item->symbols.received_symbols);
        item->symbols.received_symbols = NULL;
    }
    return ret;
}

esp_err_t ir_learn_add_list_node(struct ir_learn_list_head *learn_head)
{
    esp_err_t ret = ESP_OK;

    ir_learn_list_t *item = (ir_learn_list_t *)malloc(sizeof(ir_learn_list_t));
    ESP_GOTO_ON_FALSE(NULL != item, ESP_ERR_NO_MEM, err, TAG, "no mem to store received RMT symbols");

    SLIST_INIT(&item->cmd_sub_node);
    item->next.sle_next = NULL;

    ir_learn_list_t *last = SLIST_FIRST(learn_head);
    if (last == NULL) {
        SLIST_INSERT_HEAD(learn_head, item, next);
    } else {
        ir_learn_list_t *it;
        while ((it = SLIST_NEXT(last, next)) != NULL) {
            last = it;
        }
        SLIST_INSERT_AFTER(last, item, next);
    }
    return ret;

err:
    if (item) {
        free(item);
    }
    return ret;
}

esp_err_t ir_learn_remove_all_symbol(void)
{
    ir_learn_clean_data(&ir_learn_ctx->learn_list);
    ir_learn_clean_sub_data(&ir_learn_ctx->learn_result);

    return ESP_OK;
}

static esp_err_t ir_learn_check_duration(
    struct ir_learn_list_head *learn_head,
    struct ir_learn_sub_list_head *result_out,
    uint8_t sub_cmd_offset,
    uint32_t sub_num_symbols,
    uint32_t timediff)
{
    esp_err_t ret = ESP_OK;

    uint32_t duration_average0 = 0;
    uint32_t duration_average1 = 0;
    uint8_t learn_total_num = 0;

    ir_learn_list_t *main_it;
    rmt_symbol_word_t *p_symbols, *p_learn_symbols = NULL;
    rmt_rx_done_event_data_t add_symbols;

    add_symbols.num_symbols = sub_num_symbols;
    add_symbols.received_symbols = malloc(sub_num_symbols * sizeof(rmt_symbol_word_t));
    p_learn_symbols = add_symbols.received_symbols;
    ESP_GOTO_ON_FALSE(NULL != p_learn_symbols, ESP_ERR_NO_MEM, err, TAG, "no mem to store received RMT symbols");

    for (int i = 0; i < sub_num_symbols; i++) {
        p_symbols = NULL;
        ret = ESP_OK;
        duration_average0 = 0;
        duration_average1 = 0;
        learn_total_num = 0;

        SLIST_FOREACH(main_it, learn_head, next) {

            ir_learn_sub_list_t *sub_it = SLIST_FIRST(&main_it->cmd_sub_node);
            for (int j = 0; j < sub_cmd_offset; j++) {
                sub_it = SLIST_NEXT(sub_it, next);
            }

            p_symbols = sub_it->symbols.received_symbols;
            p_symbols += i;

            if (duration_average0) {
                if ((p_symbols->duration0 > (duration_average0 / learn_total_num + IR_DECODE_MARGIN_US)) ||
                        (p_symbols->duration0 < (duration_average0 / learn_total_num - IR_DECODE_MARGIN_US))) {
                    ret = ESP_FAIL;
                }
            }
            if (duration_average1) {
                if ((p_symbols->duration1 > (duration_average1 / learn_total_num + IR_DECODE_MARGIN_US)) ||
                        (p_symbols->duration1 < (duration_average1 / learn_total_num - IR_DECODE_MARGIN_US))) {
                    ret = ESP_FAIL;
                }
            }
            ESP_GOTO_ON_FALSE((ESP_OK == ret), ESP_ERR_INVALID_ARG, err, TAG,
                              "add cmd:%d symbol[%d] duration:[%d:%d], expect:[%d:%d]",
                              i, sub_cmd_offset,
                              p_symbols->duration0,
                              p_symbols->duration1,
                              duration_average0 / learn_total_num,
                              duration_average1 / learn_total_num);

            duration_average0 += p_symbols->duration0;
            duration_average1 += p_symbols->duration1;
            learn_total_num++;
        }

        if (learn_total_num && p_symbols) {
            p_learn_symbols->duration0 = duration_average0 / learn_total_num;
            p_learn_symbols->duration1 = duration_average1 / learn_total_num;
            // p_learn_symbols->level0 = p_symbols->level0;
            // p_learn_symbols->level1 = p_symbols->level1;
            p_learn_symbols->level0 = p_symbols->level1;
            p_learn_symbols->level1 = p_symbols->level0;
            p_learn_symbols++;
        }
    }

    ir_learn_add_sub_list_node(result_out, timediff, &add_symbols);

    if (add_symbols.received_symbols) {
        free(add_symbols.received_symbols);
    }
    return ESP_OK;

err:
    if (add_symbols.received_symbols) {
        free(add_symbols.received_symbols);
    }
    return ESP_FAIL;
}

esp_err_t ir_learn_check_valid(struct ir_learn_list_head *learn_head, struct ir_learn_sub_list_head *learn_result)
{
    esp_err_t ret = ESP_OK;
    ir_learn_list_t *learned_it;
    ir_learn_sub_list_t *sub_it;

    uint8_t expect_sub_cmd_num = 0xFF;
    uint8_t sub_cmd_num = 0;
    uint8_t learned_num = 0;

    SLIST_FOREACH(learned_it, learn_head, next) {
        sub_cmd_num = 0;
        learned_num++;
        SLIST_FOREACH(sub_it, &learned_it->cmd_sub_node, next) {
            sub_cmd_num++;
        }
        if (0xFF == expect_sub_cmd_num) {
            expect_sub_cmd_num = sub_cmd_num;
        }
        ESP_LOGI(TAG, "learn list:%d-%d", learned_num, sub_cmd_num);
        ESP_GOTO_ON_FALSE(expect_sub_cmd_num == sub_cmd_num, ESP_ERR_INVALID_SIZE, err, TAG,
                          "learn cmd:[%d], cmd num mismatch:[%d] expect:[%d]",
                          learned_num,
                          sub_cmd_num,
                          expect_sub_cmd_num);
    }

    uint16_t sub_num_symbols;
    uint32_t time_diff;

    for (int i = 0 ; i < sub_cmd_num; i++) {
        sub_num_symbols = 0xFF;
        time_diff = 0xFFFF;
        SLIST_FOREACH(learned_it, learn_head, next) {

            ir_learn_sub_list_t *sub_item = SLIST_FIRST(&learned_it->cmd_sub_node);
            for (int j = 0; j < i; j++) {
                sub_item = SLIST_NEXT(sub_item, next);
            }
            if (0xFF == sub_num_symbols) {
                sub_num_symbols = sub_item->symbols.num_symbols;
            }
            if (0xFFFF == time_diff) {
                time_diff = sub_item->timediff;
            } else {
                time_diff += sub_item->timediff;
            }
            ESP_GOTO_ON_FALSE(sub_num_symbols == sub_item->symbols.num_symbols, ESP_ERR_INVALID_SIZE, err, TAG,
                              "sub cmd symbol mismatch:[%d], expect:[%d]",
                              sub_item->symbols.num_symbols,
                              sub_num_symbols);
        }
        ESP_LOGI(TAG, "add cmd[%d] symbols:%d, diff:%d ms", i, sub_num_symbols, time_diff / learned_num / 1000);
        ret = ir_learn_check_duration(learn_head, learn_result, i, sub_num_symbols, time_diff / learned_num);
        ESP_GOTO_ON_FALSE(ESP_OK == ret, ESP_ERR_INVALID_SIZE, err, TAG, "symbol add failed");
    }
    return ESP_OK;
err:
    return ESP_FAIL;
}

esp_err_t ir_learn_new(const ir_learn_cfg_t *cfg, ir_learn_handle_t *ret_ir_learn_hdl)
{
    BaseType_t res;
    esp_err_t ret = ESP_OK;
    ESP_RETURN_ON_FALSE(cfg && ret_ir_learn_hdl, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    ir_learn_ctx = calloc(1, sizeof(ir_learn_t));
    ESP_RETURN_ON_FALSE(ir_learn_ctx, ESP_ERR_NO_MEM, TAG, "no mem for ir_learn_t");

    // Note: must create rmt rx channel before tx channel
    rmt_rx_channel_config_t rx_channel_cfg = {
        .clk_src = cfg->clk_src,
        .gpio_num = cfg->learn_gpio,
        .resolution_hz = NEC_IR_RESOLUTION_HZ,
        .mem_block_symbols = NEC_RMT_RX_MEM_BLOCK_SIZE,
        .flags.with_dma = true,
    };
    ESP_GOTO_ON_ERROR(rmt_new_rx_channel(&rx_channel_cfg, &ir_learn_ctx->channel_rx),
                      err, TAG, "create rmt rx channel failed");

    SLIST_INIT(&ir_learn_ctx->learn_list);

    ir_learn_ctx->learn_count = cfg->learn_count;

    ir_learn_ctx->rmt_rx.num_symbols = NEC_RMT_RX_MEM_BLOCK_SIZE * 4;
    ir_learn_ctx->rmt_rx.received_symbols = (rmt_symbol_word_t *)heap_caps_malloc(\
                                            ir_learn_ctx->rmt_rx.num_symbols * sizeof(rmt_symbol_word_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    ESP_GOTO_ON_FALSE(ir_learn_ctx->rmt_rx.received_symbols, ESP_ERR_NO_MEM, err, TAG, "no mem to store received RMT symbols");

    ir_learn_ctx->receive_queue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
    ESP_GOTO_ON_FALSE(ir_learn_ctx->receive_queue, ESP_ERR_NO_MEM, err, TAG, "receive queue creation failed");

    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = ir_learn_rx_done_callback,
    };
    ESP_GOTO_ON_ERROR(rmt_rx_register_event_callbacks(ir_learn_ctx->channel_rx, &cbs, ir_learn_ctx),
                      err, TAG, "enable rmt rx channel failed");

    ESP_GOTO_ON_ERROR(rmt_enable(ir_learn_ctx->channel_rx), err, TAG, "enable rmt rx channel failed");

    if (cfg->task_affinity < 0) {
        res = xTaskCreate(ir_learn_task, "ir learn task", cfg->task_stack, cfg->callback, cfg->task_priority, NULL);
    } else {
        res = xTaskCreatePinnedToCore(ir_learn_task, "ir learn task", cfg->task_stack, cfg->callback, cfg->task_priority, NULL, cfg->task_affinity);
    }
    ESP_GOTO_ON_FALSE(res == pdPASS, ESP_FAIL, err, TAG, "Create ir learn task fail!");


    if (cfg->callback) {
        cfg->callback(IR_LEARN_STATE_READY, 0, NULL);
    }

    *ret_ir_learn_hdl = ir_learn_ctx;
    return ret;

err:
    if (ir_learn_ctx) {
        ir_learn_del(ir_learn_ctx);
    }

    return ret;
}

esp_err_t ir_learn_del(ir_learn_handle_t ir_learn_hdl)
{
    ESP_RETURN_ON_FALSE(ir_learn_hdl, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    return ir_learn_destroy(ir_learn_hdl);
}