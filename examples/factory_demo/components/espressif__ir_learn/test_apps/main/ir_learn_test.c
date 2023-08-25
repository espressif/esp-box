// Copyright 2015-2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/* C includes */
#include <stdio.h>
#include <string.h>

/* FreeRTOS includes */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

/* ESP32 includes */
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"

#include "driver/gpio.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "unity.h"

#include "driver/rmt_tx.h"
#include "driver/rmt_rx.h"

/* IR learn includes */
#include "ir_learn.h"
#include "ir_encoder.h"

#define TEST_MEMORY_LEAK_THRESHOLD      (-400)

#define NEC_IR_RESOLUTION_HZ            1000000 // 1MHz resolution, 1 tick = 1us
#define BSP_IR_TX_GPIO              39
#define BSP_IR_RX_GPIO              38
#define BSP_IR_CTRL_GPIO         44

#define IR_ERR_CHECK(con, err, format, ...) if (con) { \
            ESP_LOGE(TAG, format , ##__VA_ARGS__); \
            return err;}

static const char *TAG = "ir_learn_test";

static QueueHandle_t rmt_out_queue = NULL;
static ir_learn_handle_t ir_learn_handle = NULL;

struct ir_learn_list_head learn_off_head;
struct ir_learn_list_head learn_on_head;

struct ir_learn_sub_list_head ir_leran_data_off;
struct ir_learn_sub_list_head ir_leran_data_on;

static bool ir_learn_exit;

static void ir_learn_test_tx_raw(struct ir_learn_sub_list_head *rmt_out)
{
    rmt_tx_channel_config_t tx_channel_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = NEC_IR_RESOLUTION_HZ,
        .mem_block_symbols = 128, // amount of RMT symbols that the channel can store at a time
        .trans_queue_depth = 4,  // number of transactions that allowed to pending in the background, this example won't queue multiple transactions, so queue depth > 1 is sufficient
        .gpio_num = BSP_IR_TX_GPIO,
        // .flags.with_dma = true,
        // .flags.invert_out = true,
    };
    rmt_channel_handle_t tx_channel = NULL;
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_channel_cfg, &tx_channel));

    rmt_carrier_config_t carrier_cfg = {
        .duty_cycle = 0.33,
        .frequency_hz = 38000, // 38KHz
    };
    ESP_ERROR_CHECK(rmt_apply_carrier(tx_channel, &carrier_cfg));

    rmt_transmit_config_t transmit_config = {
        .loop_count = 0, // no loop
    };

    ir_nec_encoder_config_t nec_encoder_cfg = {
        .resolution = NEC_IR_RESOLUTION_HZ,
    };
    rmt_encoder_handle_t nec_encoder = NULL;
    ESP_ERROR_CHECK(ir_encoder_new(&nec_encoder_cfg, &nec_encoder));

    ESP_ERROR_CHECK(rmt_enable(tx_channel));

    ir_learn_sub_list_t *sub_it;
    SLIST_FOREACH(sub_it, rmt_out, next) {
        ESP_LOGI(TAG, "RMT out timediff:%" PRIu32 " ms, num_symbols:%03u",
                 sub_it->timediff / 1000, sub_it->symbols.num_symbols);

        vTaskDelay(pdMS_TO_TICKS(sub_it->timediff / 1000));

        rmt_symbol_word_t *rmt_nec_symbols = sub_it->symbols.received_symbols;
        size_t symbol_num = sub_it->symbols.num_symbols;

        ESP_ERROR_CHECK(rmt_transmit(tx_channel, nec_encoder, rmt_nec_symbols, symbol_num, &transmit_config));
        rmt_tx_wait_all_done(tx_channel, -1);
    }

    rmt_disable(tx_channel);
    rmt_del_channel(tx_channel);
    ir_encoder_del(nec_encoder);
}

void ir_learn_test_tx_task(void *arg)
{
    struct ir_learn_sub_list_head tx_data;

    while (ir_learn_exit == false) {
        if (xQueueReceive(rmt_out_queue, &tx_data, pdMS_TO_TICKS(500)) == pdPASS) {
            ir_learn_test_tx_raw(&tx_data);
        }
    }
    vTaskDelete(NULL);
}

void boot_send_btn_handler(void *arg)
{
    if (gpio_get_level(GPIO_NUM_0)) {

        if (!SLIST_EMPTY(&ir_leran_data_off)) {
            esp_rom_printf(DRAM_STR("send rmt out\r\n"));
            xQueueSendFromISR(rmt_out_queue, &ir_leran_data_off, 0);
        }
    }
}

esp_err_t ir_lean_test_send_detect(void)
{
    gpio_config_t io_conf_key;
    io_conf_key.intr_type = GPIO_INTR_ANYEDGE;
    io_conf_key.mode = GPIO_MODE_INPUT;
    io_conf_key.pin_bit_mask = 1ULL << GPIO_NUM_0;
    io_conf_key.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf_key.pull_up_en = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf_key));

    gpio_install_isr_service(0);
    ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_NUM_0, boot_send_btn_handler, NULL));
    return ESP_OK;
}

void ir_learn_test_save_result(struct ir_learn_sub_list_head *data_save, struct ir_learn_sub_list_head *data_src)
{
    assert(data_src && "rmt_symbols is null");

    ir_learn_sub_list_t *sub_it;
    ir_learn_sub_list_t *last;

    last = SLIST_FIRST(data_src);
    while ((sub_it = SLIST_NEXT(last, next)) != NULL) {
        last = sub_it;
    }
    ir_learn_add_sub_list_node(data_save, last->timediff, &last->symbols);

    return;
}

void ir_learn_learn_send_callback(ir_learn_state_t state, uint8_t sub_step, struct ir_learn_sub_list_head *data)
{
    switch (state) {
    case IR_LEARN_STATE_READY:
        ESP_LOGI(TAG, "IR Learn ready");
        break;
    case IR_LEARN_STATE_END:
        ESP_LOGI(TAG, "IR Learn end");
        ir_encoder_parse_nec_payload(data);
        // ir_learn_test_save_result(&ir_leran_data_on, data);
        ir_learn_stop();
        break;
    case IR_LEARN_STATE_FAIL:
        ESP_LOGE(TAG, "IR Learn faield, retry");
        // ir_learn_restart(ir_learn_handle);

        if (ESP_OK == ir_learn_check_valid(&learn_off_head, &ir_leran_data_off)) {
            ESP_LOGI(TAG, "IR Learn off ok");
            ir_encoder_parse_nec_payload(&ir_leran_data_off);
        } else {
            ESP_LOGI(TAG, "IR Learn off failed");
        }

        if (ESP_OK == ir_learn_check_valid(&learn_on_head, &ir_leran_data_on)) {
            ESP_LOGI(TAG, "IR Learn on ok");
            ir_encoder_parse_nec_payload(&ir_leran_data_on);
        } else {
            ESP_LOGI(TAG, "IR Learn on failed");
        }
        ir_learn_stop();
        break;
    case IR_LEARN_STATE_STEP:
    default:
        ESP_LOGI(TAG, "IR Learn step:[%d][%d]", state, sub_step);

        ir_learn_list_t *learn_list;
        ir_learn_list_t *last;

        if (state % 2) {
            ESP_LOGI(TAG, "IR Learn power on");
            if (1 == sub_step) {
                ir_learn_add_list_node(&learn_on_head);
            }
            last = SLIST_FIRST(&learn_on_head);
            while ((learn_list = SLIST_NEXT(last, next)) != NULL) {
                last = learn_list;
            }
            ir_learn_test_save_result(&last->cmd_sub_node, data);
        } else {
            ESP_LOGI(TAG, "IR Learn power off");
            if (1 == sub_step) {
                ir_learn_add_list_node(&learn_off_head);
            }
            last = SLIST_FIRST(&learn_off_head);
            while ((learn_list = SLIST_NEXT(last, next)) != NULL) {
                last = learn_list;
            }
            ir_learn_test_save_result(&last->cmd_sub_node, data);
        }
        break;
    }
    return;
}

void ir_learn_keep_learn_callback(ir_learn_state_t state, uint8_t sub_step, struct ir_learn_sub_list_head *data)
{
    switch (state) {
    case IR_LEARN_STATE_READY:
        ESP_LOGI(TAG, "IR Learn ready");
        break;
    case IR_LEARN_STATE_END:
        ESP_LOGI(TAG, "IR Learn end");
        ir_encoder_parse_nec_payload(data);
        ir_learn_restart(ir_learn_handle);
        break;
    case IR_LEARN_STATE_FAIL:
        ESP_LOGI(TAG, "IR Learn faield, retry");
        ir_learn_restart(ir_learn_handle);
        break;
    case IR_LEARN_STATE_STEP:
    default:
        ESP_LOGI(TAG, "IR Learn step:[%d][%d]", state, sub_step);
        break;
    }
    return;
}

esp_err_t ir_learn_test(ir_learn_result_cb cb)
{
    esp_err_t ret = ESP_OK;

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = BIT64(BSP_IR_CTRL_GPIO);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = true;
    gpio_config(&io_conf);
    gpio_set_level(BSP_IR_CTRL_GPIO, 0);//enable IR TX

    ir_learn_exit = false;

    rmt_out_queue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
    ESP_GOTO_ON_FALSE(rmt_out_queue, ESP_ERR_NO_MEM, err, TAG, "receive queue creation failed");

    xTaskCreate(ir_learn_test_tx_task, "ir_learn_test_tx_task", 1024 * 4, NULL, 10, NULL);

    ir_lean_test_send_detect();

    ir_learn_cfg_t ir_learn_config = {
        .learn_count = 4,
        .learn_gpio = BSP_IR_RX_GPIO,
        .clk_src = RMT_CLK_SRC_DEFAULT,

        .task_stack = 4096,
        .task_priority = 5,
        .task_affinity = -1,
        .callback = cb,
    };

    ESP_ERROR_CHECK(ir_learn_new(&ir_learn_config, &ir_learn_handle));

    while (ir_learn_exit == false) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

err:
    ir_learn_stop();

    if (rmt_out_queue) {
        vQueueDelete(rmt_out_queue);
    }

    ir_learn_sub_list_t *result_it;

    while (!SLIST_EMPTY(&ir_leran_data_off)) {
        result_it = SLIST_FIRST(&ir_leran_data_off);
        if (result_it->symbols.received_symbols) {
            heap_caps_free(result_it->symbols.received_symbols);
        }
        SLIST_REMOVE_HEAD(&ir_leran_data_off, next);
        if (result_it) {
            heap_caps_free(result_it);
        }
    }
    SLIST_INIT(&ir_leran_data_off);

    while (!SLIST_EMPTY(&ir_leran_data_on)) {
        result_it = SLIST_FIRST(&ir_leran_data_on);
        if (result_it->symbols.received_symbols) {
            heap_caps_free(result_it->symbols.received_symbols);
        }
        SLIST_REMOVE_HEAD(&ir_leran_data_on, next);
        if (result_it) {
            heap_caps_free(result_it);
        }
    }
    SLIST_INIT(&ir_leran_data_on);

    vTaskDelay(pdMS_TO_TICKS(1000));

    return ret;
}

TEST_CASE("IR learn and send test", "[IR][IOT]")
{
    ir_learn_test(ir_learn_learn_send_callback);
}

TEST_CASE("IR keep learn test", "[IR][IOT]")
{
    ir_learn_test(ir_learn_keep_learn_callback);
}

static size_t before_free_8bit;
static size_t before_free_32bit;

static void check_leak(size_t before_free, size_t after_free, const char *type)
{
    ssize_t delta = after_free - before_free;
    printf("MALLOC_CAP_%s: Before %u bytes free, After %u bytes free (delta %d)\n", type, before_free, after_free, delta);
    TEST_ASSERT_MESSAGE(delta >= TEST_MEMORY_LEAK_THRESHOLD, "memory leak");
}

void setUp(void)
{
    before_free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    before_free_32bit = heap_caps_get_free_size(MALLOC_CAP_32BIT);
}

void tearDown(void)
{
    size_t after_free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t after_free_32bit = heap_caps_get_free_size(MALLOC_CAP_32BIT);
    check_leak(before_free_8bit, after_free_8bit, "8BIT");
    check_leak(before_free_32bit, after_free_32bit, "32BIT");
}

void app_main(void)
{
    printf("IR Learn TEST \n");
    // unity_run_menu();
    ir_learn_test(ir_learn_learn_send_callback);
}
