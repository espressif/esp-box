/*
 * SPDX-FileCopyrightText: 2015-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "bsp_board.h"
#include "bsp/esp-bsp.h"
#include "settings.h"

#include "lvgl.h"
#include "app_led.h"
#include "app_fan.h"
#include "app_switch.h"
#include "ui_main.h"
#include "ui_sensor_monitor.h"

#define TEST_MEMORY_LEAK_THRESHOLD      (-400)
#define IR_RESOLUTION_HZ                1000000 // 1MHz resolution, 1 tick = 1us

#define UPDATE_TIME_PERIOD              300
#define POWER_ON_PATH                   BSP_SPIFFS_MOUNT_POINT"/my_learn_off.cfg"
#define POWER_OFF_PATH                  BSP_SPIFFS_MOUNT_POINT"/my_learn_off.cfg"

#define IR_ERR_CHECK(con, err, format, ...) if (con) { \
            ESP_LOGE(TAG, format , ##__VA_ARGS__); \
            return err;}

static const char *TAG = "ui_sensor_monitor";

//IR learning
static QueueHandle_t rmt_out_queue = NULL;
static ir_learn_handle_t ir_learn_handle = NULL;

struct ir_learn_list_head learn_off_head;
struct ir_learn_list_head learn_on_head;

struct ir_learn_sub_list_head ir_leran_read_on;
struct ir_learn_sub_list_head ir_leran_read_off;

struct ir_learn_sub_list_head ir_leran_data_off;
struct ir_learn_sub_list_head ir_leran_data_on;

LV_FONT_DECLARE(font_en_12);
LV_FONT_DECLARE(font_en_bold_10);
LV_FONT_DECLARE(font_en_16);
LV_FONT_DECLARE(font_en_22);
LV_FONT_DECLARE(font_cn_gb1_16);
LV_FONT_DECLARE(font_cn_gb2_16);

LV_IMG_DECLARE(icon_temp)
LV_IMG_DECLARE(icon_humidity)
LV_IMG_DECLARE(icon_degree)
LV_IMG_DECLARE(icon_percent)
LV_IMG_DECLARE(icon_rader_on)
LV_IMG_DECLARE(icon_rader_off)
LV_IMG_DECLARE(icon_air_switch)
LV_IMG_DECLARE(icon_esp_sensor_base)


// static ui_sensor_monitor_img_type_t g_active_air_ctrl_btn_type = UI_AIR_SWITCH;

static void (*g_sensor_monitor_end_cb)(void) = NULL;

//aht21
static float temperature = 0;
static float humidity = 0;
static uint8_t Temp = 0;
static uint8_t Hum = 0;

typedef struct {
    ui_sensor_monitor_img_type_t type;
    const char *name;
    lv_img_dsc_t const *img_on;
    lv_img_dsc_t const *img_off;
} btn_image_src_t;

typedef struct {
    const char *name;
    lv_img_dsc_t const *img;
} image_src_t;

static const btn_image_src_t air_ctrl_btn_src_list[] = {
    { .type = UI_RADAR, .name = "Radar", .img_on = &icon_rader_on, .img_off = &icon_rader_off },
    { .type = UI_AIR_SWITCH, .name = "Air Switch", .img_on = &icon_air_switch, .img_off = &icon_air_switch },
};

static const image_src_t sensor_monitor_img_src_list[] = {
    { .name = "Temp", .img = &icon_temp },
    { .name = "Hum", .img = &icon_humidity },
    { .name = "sensor base", .img = &icon_esp_sensor_base },
    { .name = "degree", .img = &icon_degree },
    { .name = "percent", .img = &icon_percent },
};

typedef struct {
    char *tips_info;
    uint16_t next_time;
} user_tips_info_t;

static lv_obj_t *esp_sensor_base_img = NULL;
static lv_obj_t *tips_lab = NULL;
static lv_obj_t *btn_next = NULL;
static lv_obj_t *btn_return = NULL;
static lv_obj_t *reversal_lab = NULL;
static lv_obj_t *temp_sensor_panel = NULL;
static lv_obj_t *temp_value_label;
static lv_obj_t *hum_value_label;
static lv_obj_t *radar_panel = NULL;
static lv_obj_t *radar_image = NULL;
static lv_obj_t *radar_btn = NULL;
static lv_obj_t *radar_btn_lab = NULL;
static lv_obj_t *air_ctrl_panel = NULL;
static lv_obj_t *btn_ir_setting = NULL;
static lv_obj_t *ir_learning_tips_lab = NULL;
static lv_obj_t *ir_learning_btn = NULL;
static lv_obj_t *ir_learning_prompt_words = NULL;
static lv_obj_t *ir_learning_state_lab = NULL;
static lv_obj_t *air_switch_reversal_btn = NULL;
static lv_obj_t *relearning_lab = NULL;
static lv_obj_t *relearning_btn = NULL;
static lv_obj_t *ac_switch_btn = NULL;
static lv_obj_t *ac_switch_btn_lab = NULL;
static lv_obj_t *ir_learning_settings_close_btn = NULL;

static lv_timer_t *timer_handle;
static user_tips_info_t user_tips_info[3];
static QueueHandle_t user_info_queue = NULL;

static EventGroupHandle_t sensor_monitor_event_grp = NULL;

bool ir_learn_enable = false;

bool sensor_ir_learn_enable(void)
{
    return ir_learn_enable;
}

esp_err_t ui_sensor_set_ac_poweroff(void)
{
    if (!SLIST_EMPTY(&ir_leran_read_off)) {
        if (rmt_out_queue) {
            xQueueSendFromISR(rmt_out_queue, &ir_leran_read_off, 0);
        }
        ui_acquire();
        if (AIR_SWITCH_REVERSE_STATE & xEventGroupGetBits(sensor_monitor_event_grp)) {
            lv_label_set_text(ac_switch_btn_lab, "Turn off the air");
        } else {
            lv_label_set_text(ac_switch_btn_lab, "Turn on the air");
        }
        ui_release();
        xEventGroupClearBits(sensor_monitor_event_grp, AIR_POWER_STATE);
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "not supported");
        return ESP_ERR_NOT_SUPPORTED;
    }
}

esp_err_t ui_sensor_set_ac_poweron(void)
{
    if (!SLIST_EMPTY(&ir_leran_read_on)) {
        if (rmt_out_queue) {
            xQueueSendFromISR(rmt_out_queue, &ir_leran_read_on, 0);
        }
        ui_acquire();
        if (AIR_SWITCH_REVERSE_STATE & xEventGroupGetBits(sensor_monitor_event_grp)) {
            lv_label_set_text(ac_switch_btn_lab, "Turn on the air");
        } else {
            lv_label_set_text(ac_switch_btn_lab, "Turn off the air");
        }
        ui_release();
        xEventGroupSetBits(sensor_monitor_event_grp, AIR_POWER_STATE);
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "not supported");
        return ESP_ERR_NOT_SUPPORTED;
    }
}

esp_err_t sensor_task_state_event_init(void)
{
    sensor_monitor_event_grp = xEventGroupCreate();
    ESP_RETURN_ON_FALSE(sensor_monitor_event_grp, ESP_ERR_NO_MEM, TAG, "event group init failed");

    sys_param_t *param = settings_get_parameter();

    if (true == param->radar_en) {
        bsp_board_get_sensor_handle()->set_radar_enable(true);
        xEventGroupSetBits(sensor_monitor_event_grp, RADAR_SWITCH_STATE);
    } else {
        bsp_board_get_sensor_handle()->set_radar_enable(false);
        xEventGroupClearBits(sensor_monitor_event_grp, RADAR_SWITCH_STATE);
    }

    return ESP_OK;
}

EventBits_t sensor_task_state_event_get_bits(void)
{
    ESP_RETURN_ON_FALSE(sensor_monitor_event_grp, ESP_ERR_INVALID_STATE, TAG, "event group don't init");
    return xEventGroupGetBits(sensor_monitor_event_grp);
}

static void user_info_send(uint8_t num, char *tips, uint16_t next_delay)
{
    if (tips && (num < sizeof(user_tips_info) / sizeof(user_tips_info[0]))) {
        user_tips_info[num].next_time = next_delay;
        memset(user_tips_info[num].tips_info, 0, 30);
        memcpy(user_tips_info[num].tips_info, tips, strlen(tips) > 30 ? 30 : strlen(tips));
        if (user_info_queue) {
            xQueueSendFromISR(user_info_queue, &user_tips_info[num], 0);
        }
    }
}

esp_err_t ir_learn_save_cfg(char *filepath, struct ir_learn_sub_list_head *cmd_list)
{
    esp_err_t ret = ESP_OK;

    FILE *fp = fopen(filepath, "wb");
    ESP_GOTO_ON_FALSE(fp, ESP_FAIL, err, TAG,  "Failed open file:%s", filepath);

    uint8_t cmd_num = 0;
    ir_learn_sub_list_t *sub_it;
    SLIST_FOREACH(sub_it, cmd_list, next) {
        cmd_num++;
    }
    ESP_LOGD(TAG, "save cmd_num:%d", cmd_num);
    fwrite(&cmd_num, 1, sizeof(cmd_num), fp);

    cmd_num = 0;
    SLIST_FOREACH(sub_it, cmd_list, next) {

        uint32_t timediff = sub_it->timediff;
        fwrite(&timediff, 1, sizeof(uint32_t), fp);

        size_t symbol_num = sub_it->symbols.num_symbols;
        fwrite(&symbol_num, 1, sizeof(size_t), fp);

        ESP_LOGD(TAG, "save cmd :%d, symbols:%d", cmd_num++, symbol_num);
        rmt_symbol_word_t *rmt_nec_symbols = sub_it->symbols.received_symbols;
        fwrite(rmt_nec_symbols, 1, symbol_num * sizeof(rmt_symbol_word_t), fp);
    }
    fclose(fp);
err:
    return ret;
}

esp_err_t ir_learn_read_cfg(char *filepath, struct ir_learn_sub_list_head *cmd_list)
{
    esp_err_t ret = ESP_OK;

    FILE *fp = fopen(filepath, "r");
    ESP_GOTO_ON_FALSE(fp, ESP_FAIL, err, TAG,  "Failed open file:%s", filepath);

    uint8_t total_cmd_num = 0;
    fread(&total_cmd_num, 1, sizeof(total_cmd_num), fp);
    ESP_LOGD(TAG, "total cmd_num:%d", total_cmd_num);

    for (int i = 0; i < total_cmd_num; i++) {
        uint32_t timediff;
        fread(&timediff, 1, sizeof(uint32_t), fp);

        rmt_rx_done_event_data_t symbols;
        fread(&symbols.num_symbols, 1, sizeof(size_t), fp);
        ESP_LOGD(TAG, "read cmd :%d, symbols:%d", i, symbols.num_symbols);
        symbols.received_symbols = (rmt_symbol_word_t *)heap_caps_malloc(symbols.num_symbols * sizeof(rmt_symbol_word_t), \
                                   MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        fread(symbols.received_symbols, 1, symbols.num_symbols * sizeof(rmt_symbol_word_t), fp);
        ir_learn_add_sub_list_node(cmd_list, timediff, &symbols);
        if (symbols.received_symbols) {
            free(symbols.received_symbols);
        }
    }
    fclose(fp);
err:
    return ret;
}

static void ir_learn_test_tx_raw(struct ir_learn_sub_list_head *rmt_out)
{
    rmt_tx_channel_config_t tx_channel_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = IR_RESOLUTION_HZ,
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

    ir_encoder_config_t nec_encoder_cfg = {
        .resolution = IR_RESOLUTION_HZ,
    };
    rmt_encoder_handle_t nec_encoder = NULL;
    ESP_ERROR_CHECK(ir_encoder_new(&nec_encoder_cfg, &nec_encoder));

    ESP_ERROR_CHECK(rmt_enable(tx_channel));

    ir_learn_sub_list_t *sub_it;
    SLIST_FOREACH(sub_it, rmt_out, next) {
        ESP_LOGD(TAG, "RMT out timediff:%" PRIu32 " ms, symbols:%03u",
                 sub_it->timediff / 1000, sub_it->symbols.num_symbols);

        vTaskDelay(pdMS_TO_TICKS(sub_it->timediff / 1000));

        rmt_symbol_word_t *rmt_nec_symbols = sub_it->symbols.received_symbols;
        size_t symbol_num = sub_it->symbols.num_symbols;

        ESP_ERROR_CHECK(rmt_transmit(tx_channel, nec_encoder, rmt_nec_symbols, symbol_num, &transmit_config));
        rmt_tx_wait_all_done(tx_channel, -1);
    }

    rmt_disable(tx_channel);
    rmt_del_channel(tx_channel);
    nec_encoder->del(nec_encoder);
}

static void ir_learn_test_tx_task(void *arg)
{
    struct ir_learn_sub_list_head tx_data;

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = BIT64(BSP_IR_CTRL_GPIO);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = true;
    gpio_config(&io_conf);
    gpio_set_level(BSP_IR_CTRL_GPIO, 0);//enable IR TX

    rmt_out_queue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
    if (NULL == rmt_out_queue) {
        ESP_LOGW(TAG, "receive queue creation failed");
    }

    if (IR_LEARNING_STATE & xEventGroupGetBits(sensor_monitor_event_grp)) {
        esp_err_t ret = ir_learn_read_cfg(POWER_ON_PATH, &ir_leran_read_on);
        if (ret == ESP_OK) {
            ESP_LOGD(TAG, "air_on ir-data read OK.");
        }
        ret = ir_learn_read_cfg(POWER_OFF_PATH, &ir_leran_read_off);
        if (ret == ESP_OK) {
            ESP_LOGD(TAG, "air_off ir-data read OK.");
        }
    }

    xEventGroupClearBits(sensor_monitor_event_grp, NEED_DELETE);

    while (1) {
        if ((NEED_DELETE & xEventGroupGetBits(sensor_monitor_event_grp))) {
            xEventGroupSetBits(sensor_monitor_event_grp, TX_CH_DELETED);
            ESP_LOGD(TAG, "ir learn tx_task delete.");
            if (rmt_out_queue) {
                vQueueDelete(rmt_out_queue);
                rmt_out_queue = NULL;
            }
            vTaskDelete(NULL);
        }

        if (xQueueReceive(rmt_out_queue, &tx_data, pdMS_TO_TICKS(500)) == pdPASS) {
            ir_learn_test_tx_raw(&tx_data);
        }
    }
}

static void ir_learn_test_save_result(struct ir_learn_sub_list_head *data_save, struct ir_learn_sub_list_head *data_src)
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

static void ir_learn_learn_send_callback(ir_learn_state_t state, uint8_t sub_step, struct ir_learn_sub_list_head *data)
{
    const sys_param_t *param = settings_get_parameter();
    static uint8_t air_on_ir = 0;
    static uint8_t air_off_ir = 0;
    switch (state) {
    case IR_LEARN_STATE_EXIT:
        ESP_LOGI(TAG, "IR Learn exit");
        break;
    case IR_LEARN_STATE_READY:
        ESP_LOGI(TAG, "IR Learn ready");
        xEventGroupClearBits(sensor_monitor_event_grp, IR_LEARNING_STATE);
        ui_acquire();
        lv_obj_add_flag(ir_learning_tips_lab, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ir_learning_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ir_learning_prompt_words, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ir_learning_state_lab, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ir_learning_state_lab, "Press the button");
        ui_release();
        break;
    case IR_LEARN_STATE_END:
    case IR_LEARN_STATE_FAIL:
        if (ESP_OK == ir_learn_check_valid(&learn_on_head, &ir_leran_data_on)) {
            ESP_LOGI(TAG, "IR Learn on ok");
            air_on_ir = 1;
        } else {
            ESP_LOGI(TAG, "IR Learn on failed");
            air_on_ir = 0;
        }

        if (ESP_OK == ir_learn_check_valid(&learn_off_head, &ir_leran_data_off)) {
            ESP_LOGI(TAG, "IR Learn off ok");
            air_off_ir = 1;
        } else {
            ESP_LOGI(TAG, "IR Learn off failed");
            air_off_ir = 0;
        }

        if (air_on_ir && air_off_ir) {
            ui_acquire();
            lv_label_set_text(ir_learning_state_lab, "Learning successful.");
            ui_release();
            vTaskDelay(pdMS_TO_TICKS(1500));

            ui_acquire();
            lv_obj_add_flag(ir_learning_state_lab, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ir_learning_prompt_words, LV_OBJ_FLAG_HIDDEN);

            lv_obj_clear_flag(btn_ir_setting, LV_OBJ_FLAG_HIDDEN);
            if (SR_LANG_EN == param->sr_lang) {
                lv_label_set_text(ir_learning_tips_lab,
                                  "Click the buton or say\n 'Turn on/off the Air'\n to control your air.");
            } else {
                lv_label_set_text(ir_learning_tips_lab,
                                  "Click the buton or say\n '打开/关闭空调'\n to control your air.");
            }
            lv_obj_clear_flag(ir_learning_tips_lab, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(ac_switch_btn, LV_OBJ_FLAG_HIDDEN);
            ui_release();

            ir_learn_save_cfg(POWER_ON_PATH, &ir_leran_data_on);
            esp_err_t ret = ir_learn_read_cfg(POWER_ON_PATH, &ir_leran_read_on);
            if (ret == ESP_OK) {
                ESP_LOGD(TAG, "air_on ir-data read OK.");
            }
            ir_learn_save_cfg(POWER_OFF_PATH, &ir_leran_data_off);
            ret = ir_learn_read_cfg(POWER_OFF_PATH, &ir_leran_read_off);
            if (ret == ESP_OK) {
                ESP_LOGD(TAG, "air_off ir-data read OK.");
            }
            xEventGroupSetBits(sensor_monitor_event_grp, IR_LEARNING_STATE);
            ir_learn_stop(&ir_learn_handle);
            ir_learn_enable = false;
        } else {
            xEventGroupClearBits(sensor_monitor_event_grp, IR_LEARNING_STATE);
            ESP_LOGI(TAG, "IR Learn ready");

            user_info_send(0, "Learning failed, retry!", 1500);
            user_info_send(1, "Press the button", 0);
            ir_learn_clean_data(&learn_on_head);
            ir_learn_clean_data(&learn_off_head);
            ir_learn_clean_sub_data(&ir_leran_data_on);
            ir_learn_clean_sub_data(&ir_leran_data_off);
            ir_learn_restart(ir_learn_handle);
        }
        break;
    case IR_LEARN_STATE_STEP:
    default:
        ESP_LOGI(TAG, "IR Learn step:[%d][%d]", state, sub_step);

        ir_learn_list_t *learn_list;
        ir_learn_list_t *last;
        if (state % 2) {
            if (1 == sub_step) {
                ir_learn_add_list_node(&learn_on_head);
                user_info_send(0, "Press button again", 0);
            }
            last = SLIST_FIRST(&learn_on_head);
            while ((learn_list = SLIST_NEXT(last, next)) != NULL) {
                last = learn_list;
            }
            ir_learn_test_save_result(&last->cmd_sub_node, data);
        } else {
            if (1 == sub_step) {
                ir_learn_add_list_node(&learn_off_head);
            }
            last = SLIST_FIRST(&learn_off_head);
            while ((learn_list = SLIST_NEXT(last, next)) != NULL) {
                last = learn_list;
            }
            ir_learn_test_save_result(&last->cmd_sub_node, data);
            if (state / 2 == 1) {
                if (1 == sub_step) {
                    user_info_send(0, "First learning successful", 1200);
                    user_info_send(1, "Learn again, ready!", 0);
                }
            }
        }
        break;
    }
    return;
}

static esp_err_t ir_learn_start(ir_learn_result_cb cb)
{
    esp_err_t ret = ESP_OK;

    ir_learn_cfg_t ir_learn_config = {
        .learn_count = 4,
        .learn_gpio = BSP_IR_RX_GPIO,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution = IR_RESOLUTION_HZ,

        .task_stack = 4096,
        .task_priority = 5,
        .task_affinity = 1,
        .callback = cb,
    };
    ir_learn_enable = true;

    ESP_ERROR_CHECK(ir_learn_new(&ir_learn_config, &ir_learn_handle));
    return ret;
}

static void ui_sensor_monitor_page_return_click_cb(lv_event_t *e)
{
    ir_learn_clean_data(&learn_on_head);
    ir_learn_clean_data(&learn_off_head);
    ir_learn_clean_sub_data(&ir_leran_data_on);
    ir_learn_clean_sub_data(&ir_leran_data_off);
    ir_learn_clean_sub_data(&ir_leran_read_on);
    ir_learn_clean_sub_data(&ir_leran_read_off);
    xEventGroupClearBits(sensor_monitor_event_grp, SENSOR_MONITOR_ALIVE_STATE);

    if (timer_handle) {
        lv_timer_del(timer_handle);
        timer_handle = NULL;
    }

    if (user_info_queue) {
        vQueueDelete(user_info_queue);
        user_info_queue = NULL;
    }
    for (int i = 0; i < sizeof(user_tips_info) / sizeof(user_tips_info[0]); i++) {
        free(user_tips_info[i].tips_info);
    }

    if (0 == (IR_LEARNING_STATE & xEventGroupGetBits(sensor_monitor_event_grp))) {
        ir_learn_stop(&ir_learn_handle);
        ir_learn_enable = false;
    }

    xEventGroupSetBits(sensor_monitor_event_grp, NEED_DELETE);
    xEventGroupWaitBits(sensor_monitor_event_grp, NEED_DELETE | TX_CH_DELETED, 1, 1, portMAX_DELAY);

    lv_obj_t *obj = lv_event_get_user_data(e);
    if (ui_get_btn_op_group()) {
        lv_group_remove_all_objs(ui_get_btn_op_group());
    }
#if !CONFIG_BSP_BOARD_ESP32_S3_BOX_Lite
    bsp_btn_rm_all_callback(BSP_BUTTON_MAIN);
#endif
    lv_obj_del(obj);
    if (g_sensor_monitor_end_cb) {
        g_sensor_monitor_end_cb();
    }
}

#if !CONFIG_BSP_BOARD_ESP32_S3_BOX_Lite
static void btn_return_down_cb(void *handle, void *arg)
{
    lv_obj_t *obj = (lv_obj_t *) arg;
    ui_acquire();
    lv_event_send(obj, LV_EVENT_CLICKED, NULL);
    ui_release();
}
#endif

static void ui_next_btn_event(lv_event_t *e)
{
    lv_obj_add_flag(esp_sensor_base_img, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(tips_lab, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(btn_next, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(btn_return, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(temp_sensor_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(radar_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(air_ctrl_panel, LV_OBJ_FLAG_HIDDEN);
}

static void ui_sensor_monitor_btn_ir_setting_event(lv_event_t *e)
{
    lv_obj_add_flag(btn_return, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(temp_sensor_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(radar_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(air_ctrl_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(reversal_lab, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(relearning_lab, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(relearning_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ir_learning_settings_close_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(air_switch_reversal_btn, LV_OBJ_FLAG_HIDDEN);
}

static void ui_sensor_monitor_btn_ir_learning_event(lv_event_t *e)
{
    if (SENSOR_BASE_CONNECT_STATE & xEventGroupGetBits(sensor_monitor_event_grp)) {
        ir_learn_start(ir_learn_learn_send_callback);
    }
}

static void ui_sensor_monitor_btn_ac_switch_event(lv_event_t *e)
{
    if (SENSOR_BASE_CONNECT_STATE & xEventGroupGetBits(sensor_monitor_event_grp)) {
        if (AIR_POWER_STATE & xEventGroupGetBits(sensor_monitor_event_grp)) {
            ui_sensor_set_ac_poweroff();
        } else {
            ui_sensor_set_ac_poweron();
        }
    }
}

static void update_timer_cb(lv_timer_t *timer)
{
    static uint8_t recv_delay = 0;
    user_tips_info_t user_tips;

    if (0 == recv_delay) {
        if (pdPASS == xQueueReceive(user_info_queue, (void *)&user_tips, 0)) {
            if (ir_learning_state_lab) {
                lv_label_set_text(ir_learning_state_lab, user_tips.tips_info);
                ESP_LOGI(TAG, "tips_info: %s", user_tips.tips_info);
            }
            recv_delay = user_tips.next_time / UPDATE_TIME_PERIOD;
        }
    }
    if (recv_delay) {
        recv_delay--;
    }

    /**
     * @brief update temperature, humidity
     */
    esp_err_t ret = bsp_board_get_sensor_handle()->get_humiture(&temperature, &humidity);
    if (ret != ESP_OK) {
        lv_label_set_text(temp_value_label, "0");
        lv_label_set_text(hum_value_label, "0");
    } else {
        Temp = (uint8_t)temperature;
        float fractionalPart = temperature - Temp;
        Temp -= 3;
        if (fractionalPart > 0.5) {
            Temp++;
        }
        char value_str[2] = {0};
        sprintf(value_str, "%d", Temp);
        lv_label_set_text(temp_value_label, value_str);

        Hum = (uint8_t)humidity;
        fractionalPart = humidity - Hum;
        if (fractionalPart > 0.5) {
            Hum++;
        }
        if (Hum >= 100) {
            Hum = 99;
        }
        sprintf(value_str, "%d", Hum);
        lv_label_set_text(hum_value_label, value_str);
    }

    /**
     * @brief update radar status
     */
    if ((RADAR_SWITCH_STATE & xEventGroupGetBits(sensor_monitor_event_grp))) {
        if (true == bsp_board_get_sensor_handle()->get_radar_status()) {
            xEventGroupSetBits(sensor_monitor_event_grp, RADAR_STATE);
            if (SENSOR_MONITOR_ALIVE_STATE & sensor_task_state_event_get_bits()) {
                lv_img_set_src(radar_image, air_ctrl_btn_src_list[0].img_on);
            }
        } else {
            xEventGroupClearBits(sensor_monitor_event_grp, RADAR_STATE);
            if (SENSOR_MONITOR_ALIVE_STATE & sensor_task_state_event_get_bits()) {
                lv_img_set_src(radar_image, air_ctrl_btn_src_list[0].img_off);
            }
        }
    }
}

static void ui_radar_btn_event(lv_event_t *e)
{
    sys_param_t *param = settings_get_parameter();

    if (SENSOR_BASE_CONNECT_STATE & xEventGroupGetBits(sensor_monitor_event_grp)) {
        if (RADAR_SWITCH_STATE & xEventGroupGetBits(sensor_monitor_event_grp)) {
            xEventGroupClearBits(sensor_monitor_event_grp, RADAR_SWITCH_STATE);
            param->radar_en = false;
        } else {
            xEventGroupSetBits(sensor_monitor_event_grp, RADAR_SWITCH_STATE);
            param->radar_en = true;
        }
        bsp_board_get_sensor_handle()->set_radar_enable(param->radar_en);
        settings_write_parameter_to_nvs();

        if (!(RADAR_SWITCH_STATE & xEventGroupGetBits(sensor_monitor_event_grp))) {
            lv_obj_set_style_border_color(radar_btn, lv_color_hex(0x9E9E9E), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(radar_btn_lab, "OFF");
            lv_obj_set_style_bg_color(radar_btn_lab, lv_color_hex(0x9E9E9E), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_x(radar_btn_lab, -8);
            lv_img_set_src(radar_image, air_ctrl_btn_src_list[0].img_off);
        } else {
            lv_obj_set_style_border_color(radar_btn, lv_color_hex(0xEB4839), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(radar_btn_lab, "ON");
            lv_obj_set_style_bg_color(radar_btn_lab, lv_color_hex(0xEB4839), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_x(radar_btn_lab, 8);
        }
    }
}

static void ui_sensor_monitor_air_switch_reversal_btn_event(lv_event_t *e)
{
    if (AIR_SWITCH_REVERSE_STATE & xEventGroupGetBits(sensor_monitor_event_grp)) {
        xEventGroupClearBits(sensor_monitor_event_grp, AIR_SWITCH_REVERSE_STATE);
        if (AIR_POWER_STATE & xEventGroupGetBits(sensor_monitor_event_grp)) {
            xEventGroupClearBits(sensor_monitor_event_grp, AIR_POWER_STATE);
        } else {
            xEventGroupSetBits(sensor_monitor_event_grp, AIR_POWER_STATE);
        }
    } else {
        xEventGroupSetBits(sensor_monitor_event_grp, AIR_SWITCH_REVERSE_STATE);
        if (AIR_POWER_STATE & xEventGroupGetBits(sensor_monitor_event_grp)) {
            xEventGroupClearBits(sensor_monitor_event_grp, AIR_POWER_STATE);
        } else {
            xEventGroupSetBits(sensor_monitor_event_grp, AIR_POWER_STATE);
        }
    }
    lv_obj_add_flag(reversal_lab, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(relearning_lab, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(relearning_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ir_learning_settings_close_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(air_switch_reversal_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(btn_return, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(temp_sensor_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(radar_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(air_ctrl_panel, LV_OBJ_FLAG_HIDDEN);
}

static void ui_sensor_monitor_relearn_btn_event(lv_event_t *e)
{
    ESP_LOGD(TAG, "relearn.");
    lv_obj_add_flag(reversal_lab, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(relearning_lab, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(relearning_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ir_learning_settings_close_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(air_switch_reversal_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ac_switch_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(btn_return, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(temp_sensor_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(radar_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(air_ctrl_panel, LV_OBJ_FLAG_HIDDEN);

    ir_learn_clean_data(&learn_on_head);
    ir_learn_clean_data(&learn_off_head);
    ir_learn_clean_sub_data(&ir_leran_data_on);
    ir_learn_clean_sub_data(&ir_leran_data_off);
    ir_learn_clean_sub_data(&ir_leran_read_on);
    ir_learn_clean_sub_data(&ir_leran_read_off);

    if (remove(POWER_ON_PATH) == 0 && remove(POWER_OFF_PATH) == 0) {
        ESP_LOGD(TAG, "remove file succes.\n");
    } else {
        ESP_LOGE(TAG, "remove file failed.\n");
    }
    if (0 == (IR_LEARNING_STATE & xEventGroupGetBits(sensor_monitor_event_grp))) {
        ESP_LOGD(TAG, "ir_learn_stop.\n");
        ir_learn_stop(&ir_learn_handle);
        ir_learn_enable = false;
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    ir_learn_start(ir_learn_learn_send_callback);
}

static void ir_learning_settings_close_event(lv_event_t *e)
{
    lv_obj_add_flag(reversal_lab, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(relearning_lab, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(relearning_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ir_learning_settings_close_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(air_switch_reversal_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(btn_return, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(temp_sensor_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(radar_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(air_ctrl_panel, LV_OBJ_FLAG_HIDDEN);
}

void ui_sensor_monitor_start(void (*fn)(void))
{
    ESP_LOGI(TAG, "sensor monitor initialize");
    g_sensor_monitor_end_cb = fn;
    const sys_param_t *param = settings_get_parameter();
    struct stat file_stat;
    if (stat(POWER_ON_PATH, &file_stat) == 0 && stat(POWER_OFF_PATH, &file_stat) == 0) {
        xEventGroupSetBits(sensor_monitor_event_grp, IR_LEARNING_STATE);
    } else {
        xEventGroupClearBits(sensor_monitor_event_grp, IR_LEARNING_STATE);
    }

    xTaskCreatePinnedToCore(ir_learn_test_tx_task, "ir_learn_test_tx_task", 1024 * 4, NULL, 10, NULL, 1);

    user_info_queue = xQueueCreate(sizeof(user_tips_info) / sizeof(user_tips_info[0]), sizeof(user_tips_info_t));
    if (NULL == user_info_queue) {
        ESP_LOGW(TAG, "user info queue creation failed");
    }

    for (int i = 0; i < sizeof(user_tips_info) / sizeof(user_tips_info[0]); i++) {
        user_tips_info[i].tips_info = (char *)heap_caps_malloc(30, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (NULL == user_tips_info[i].tips_info) {
            ESP_LOGW(TAG, "user_info[%d] malloc failed", i);
        }
    }

    xEventGroupSetBits(sensor_monitor_event_grp, SENSOR_MONITOR_ALIVE_STATE);
    xEventGroupClearBits(sensor_monitor_event_grp, AIR_SWITCH_REVERSE_STATE);
    xEventGroupClearBits(sensor_monitor_event_grp, AIR_POWER_STATE);
    xEventGroupSetBits(sensor_monitor_event_grp, SENSOR_BASE_CONNECT_STATE);
    lv_obj_t *page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(page, lv_obj_get_width(lv_obj_get_parent(page)), lv_obj_get_height(lv_obj_get_parent(page)) - lv_obj_get_height(ui_main_get_status_bar()));
    lv_obj_set_style_border_width(page, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(page, lv_obj_get_style_bg_color(lv_scr_act(), LV_STATE_DEFAULT), LV_PART_MAIN);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align_to(page, ui_main_get_status_bar(), LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

    btn_return = lv_btn_create(page);
    lv_obj_set_size(btn_return, 24, 24);
    lv_obj_add_style(btn_return, &ui_button_styles()->style, 0);
    lv_obj_add_style(btn_return, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(btn_return, &ui_button_styles()->style_focus, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_return, &ui_button_styles()->style_focus, LV_STATE_FOCUSED);
    lv_obj_align(btn_return, LV_ALIGN_TOP_LEFT, 0, -8);
    lv_obj_t *return_btn_text = lv_label_create(btn_return);
    lv_label_set_text_static(return_btn_text, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(return_btn_text, lv_color_make(158, 158, 158), LV_STATE_DEFAULT);
    lv_obj_center(return_btn_text);
    lv_obj_add_event_cb(btn_return, ui_sensor_monitor_page_return_click_cb, LV_EVENT_CLICKED, page);

    reversal_lab = lv_label_create(page);
    lv_obj_set_width(reversal_lab, 260);
    lv_obj_set_align(reversal_lab, LV_ALIGN_CENTER);
    lv_obj_set_style_text_align(reversal_lab, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(reversal_lab, "If the power button is reversed, click \"Reversal\" button below to fix it.");
    lv_obj_set_style_text_color(reversal_lab, lv_color_make(40, 40, 40), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(reversal_lab, &font_cn_gb2_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(reversal_lab, LV_ALIGN_CENTER, 20, -75);
    lv_obj_add_flag(reversal_lab, LV_OBJ_FLAG_HIDDEN);

    air_switch_reversal_btn = lv_btn_create(page);
    lv_obj_add_style(air_switch_reversal_btn, &ui_button_styles()->style, 0);
    lv_obj_add_style(air_switch_reversal_btn, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(air_switch_reversal_btn, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(air_switch_reversal_btn, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);
    lv_obj_set_size(air_switch_reversal_btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(air_switch_reversal_btn, LV_ALIGN_CENTER, 0, -30);
    lv_obj_add_flag(air_switch_reversal_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(air_switch_reversal_btn, ui_sensor_monitor_air_switch_reversal_btn_event, LV_EVENT_CLICKED, page);
    lv_obj_t *air_switch_reversal_btn_lab = lv_label_create(air_switch_reversal_btn);
    lv_obj_set_style_text_font(air_switch_reversal_btn_lab, &font_cn_gb2_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(air_switch_reversal_btn_lab, "Reversal");
    lv_obj_set_style_text_color(air_switch_reversal_btn_lab, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
    lv_obj_center(air_switch_reversal_btn_lab);

    relearning_lab = lv_label_create(page);
    lv_obj_set_width(relearning_lab, 260);
    lv_obj_set_align(relearning_lab, LV_ALIGN_CENTER);
    lv_obj_set_style_text_align(relearning_lab, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(relearning_lab, "Click the \"Relearn\" button to clear learning history and start IR learning again.");
    lv_obj_set_style_text_color(relearning_lab, lv_color_make(40, 40, 40), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(relearning_lab, &font_cn_gb2_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(relearning_lab, LV_ALIGN_CENTER, 20, 20);
    lv_obj_add_flag(relearning_lab, LV_OBJ_FLAG_HIDDEN);

    relearning_btn = lv_btn_create(page);
    lv_obj_add_style(relearning_btn, &ui_button_styles()->style, 0);
    lv_obj_add_style(relearning_btn, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(relearning_btn, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(relearning_btn, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);
    lv_obj_set_size(relearning_btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(relearning_btn, LV_ALIGN_CENTER, 0, 75);
    lv_obj_add_flag(relearning_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(relearning_btn, ui_sensor_monitor_relearn_btn_event, LV_EVENT_CLICKED, page);
    lv_obj_t *relearning_btn_lab = lv_label_create(relearning_btn);
    lv_obj_set_style_text_font(relearning_btn_lab, &font_cn_gb2_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(relearning_btn_lab, "Relearn");
    lv_obj_set_style_text_color(relearning_btn_lab, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
    lv_obj_center(relearning_btn_lab);

    ir_learning_settings_close_btn = lv_btn_create(page);
    lv_obj_set_size(ir_learning_settings_close_btn, 24, 24);
    lv_obj_add_style(ir_learning_settings_close_btn, &ui_button_styles()->style, 0);
    lv_obj_add_style(ir_learning_settings_close_btn, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(ir_learning_settings_close_btn, &ui_button_styles()->style_focus, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(ir_learning_settings_close_btn, &ui_button_styles()->style_focus, LV_STATE_FOCUSED);
    lv_obj_align(ir_learning_settings_close_btn, LV_ALIGN_TOP_LEFT, 0, -8);
    lv_obj_add_flag(ir_learning_settings_close_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *ir_learning_settings_close_btn_text = lv_label_create(ir_learning_settings_close_btn);
    lv_label_set_text_static(ir_learning_settings_close_btn_text, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(ir_learning_settings_close_btn_text, lv_color_make(158, 158, 158), LV_STATE_DEFAULT);
    lv_obj_center(ir_learning_settings_close_btn_text);
    lv_obj_add_event_cb(ir_learning_settings_close_btn, ir_learning_settings_close_event, LV_EVENT_CLICKED, page);

    temp_sensor_panel = lv_obj_create(page);
    lv_obj_set_size(temp_sensor_panel, 150, 50);
    lv_obj_clear_flag(temp_sensor_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(temp_sensor_panel, LV_ALIGN_TOP_LEFT, 35, -8);
    lv_obj_set_style_radius(temp_sensor_panel, 10, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(temp_sensor_panel, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(temp_sensor_panel, LV_OPA_30, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(temp_sensor_panel, 10, LV_STATE_DEFAULT);
    lv_obj_t *temp_sensor_image = lv_img_create(temp_sensor_panel);
    lv_obj_align(temp_sensor_image, LV_ALIGN_CENTER, -62, 0);
    lv_img_set_src(temp_sensor_image, sensor_monitor_img_src_list[0].img);
    temp_value_label = lv_label_create(temp_sensor_panel);
    lv_label_set_text(temp_value_label, "");
    lv_obj_set_style_text_color(temp_value_label, lv_color_make(40, 40, 40), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(temp_value_label, &font_en_22, LV_STATE_DEFAULT);
    lv_obj_align(temp_value_label, LV_ALIGN_CENTER, -37, 1);
    lv_obj_t *degree_image = lv_img_create(temp_sensor_panel);
    lv_obj_align(degree_image, LV_ALIGN_CENTER, -14, 0);
    lv_img_set_src(degree_image, sensor_monitor_img_src_list[3].img);

    lv_obj_t *hum_sensor_image = lv_img_create(temp_sensor_panel);
    lv_obj_align(hum_sensor_image, LV_ALIGN_CENTER, 12, 0);
    lv_img_set_src(hum_sensor_image, sensor_monitor_img_src_list[1].img);
    hum_value_label = lv_label_create(temp_sensor_panel);
    lv_label_set_text(hum_value_label, "");
    lv_obj_set_style_text_color(hum_value_label, lv_color_make(40, 40, 40), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(hum_value_label, &font_en_22, LV_STATE_DEFAULT);
    lv_obj_align(hum_value_label, LV_ALIGN_CENTER, 36, 1);
    lv_obj_t *percent_image = lv_img_create(temp_sensor_panel);
    lv_obj_align(percent_image, LV_ALIGN_CENTER, 59, 0);
    lv_img_set_src(percent_image, sensor_monitor_img_src_list[4].img);

    radar_panel = lv_obj_create(page);
    lv_obj_set_size(radar_panel, 95, 50);
    lv_obj_clear_flag(radar_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(radar_panel, LV_ALIGN_TOP_LEFT, 195, -8);
    lv_obj_set_style_radius(radar_panel, 10, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(radar_panel, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(radar_panel, LV_OPA_30, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(radar_panel, 10, LV_STATE_DEFAULT);
    radar_btn = lv_btn_create(radar_panel);
    lv_obj_set_size(radar_btn, 46, 18);
    lv_obj_set_style_radius(radar_btn, 9, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(radar_btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(radar_btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    if ((RADAR_SWITCH_STATE & xEventGroupGetBits(sensor_monitor_event_grp))) {
        lv_obj_set_style_border_color(radar_btn, lv_color_hex(0xCE244F), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_border_color(radar_btn, lv_color_hex(0x9E9E9E), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    lv_obj_set_style_border_opa(radar_btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(radar_btn, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(radar_btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(radar_btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(radar_btn, LV_ALIGN_CENTER, 19, -8);
    lv_obj_add_event_cb(radar_btn, ui_radar_btn_event, LV_EVENT_CLICKED, radar_panel);
    radar_btn_lab = lv_label_create(radar_btn);
    lv_obj_set_width(radar_btn_lab, 24);
    lv_obj_set_height(radar_btn_lab, 12);
    if ((RADAR_SWITCH_STATE & xEventGroupGetBits(sensor_monitor_event_grp))) {
        lv_obj_set_x(radar_btn_lab, 8);
        lv_label_set_text(radar_btn_lab, "ON");
        lv_obj_set_style_bg_color(radar_btn_lab, lv_color_hex(0xCE244F), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_x(radar_btn_lab, -8);
        lv_label_set_text(radar_btn_lab, "OFF");
        lv_obj_set_style_bg_color(radar_btn_lab, lv_color_hex(0x9E9E9E), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    lv_obj_set_y(radar_btn_lab, 0);
    lv_obj_set_align(radar_btn_lab, LV_ALIGN_CENTER);
    lv_obj_set_style_text_color(radar_btn_lab, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(radar_btn_lab, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(radar_btn_lab, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(radar_btn_lab, &font_en_bold_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(radar_btn_lab, 7, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_opa(radar_btn_lab, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    radar_image = lv_img_create(radar_panel);
    lv_obj_align(radar_image, LV_ALIGN_CENTER, -26, 0);
    if ((RADAR_SWITCH_STATE & xEventGroupGetBits(sensor_monitor_event_grp)) && (RADAR_STATE & xEventGroupGetBits(sensor_monitor_event_grp))) {
        lv_img_set_src(radar_image, air_ctrl_btn_src_list[0].img_on);
    } else {
        lv_img_set_src(radar_image, air_ctrl_btn_src_list[0].img_off);
    }
    lv_obj_t *radar_label = lv_label_create(radar_panel);
    lv_label_set_text_static(radar_label, air_ctrl_btn_src_list[0].name);
    lv_obj_set_style_text_color(radar_label, lv_color_make(40, 40, 40), LV_STATE_DEFAULT);
    lv_obj_align(radar_label, LV_ALIGN_CENTER, 19, 9);
    lv_obj_set_style_text_font(radar_label, &font_cn_gb2_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    air_ctrl_panel = lv_obj_create(page);
    lv_obj_set_size(air_ctrl_panel, 255, 120);
    lv_obj_clear_flag(air_ctrl_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(air_ctrl_panel, LV_ALIGN_TOP_LEFT, 35, 52);
    lv_obj_set_style_radius(air_ctrl_panel, 10, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(air_ctrl_panel, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(air_ctrl_panel, LV_OPA_30, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(air_ctrl_panel, 10, LV_STATE_DEFAULT);
    btn_ir_setting = lv_btn_create(air_ctrl_panel);
    lv_obj_set_size(btn_ir_setting, 24, 24);
    lv_obj_add_style(btn_ir_setting, &ui_button_styles()->style, 0);
    lv_obj_add_style(btn_ir_setting, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(btn_ir_setting, &ui_button_styles()->style_focus, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_ir_setting, &ui_button_styles()->style_focus, LV_STATE_FOCUSED);
    // lv_obj_add_flag(btn_ir_setting, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(btn_ir_setting, LV_ALIGN_TOP_RIGHT, 5, 76);
    lv_obj_t *btn_ir_setting_lab = lv_label_create(btn_ir_setting);
    lv_label_set_text_static(btn_ir_setting_lab, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(btn_ir_setting_lab, lv_color_make(158, 158, 158), LV_STATE_DEFAULT);
    lv_obj_center(btn_ir_setting_lab);
    lv_obj_add_event_cb(btn_ir_setting, ui_sensor_monitor_btn_ir_setting_event, LV_EVENT_CLICKED, page);

    ir_learning_tips_lab = lv_label_create(air_ctrl_panel);
    lv_obj_set_width(ir_learning_tips_lab, 240);
    lv_obj_set_height(ir_learning_tips_lab, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ir_learning_tips_lab, 0);
    lv_obj_set_y(ir_learning_tips_lab, -23);
    lv_obj_set_align(ir_learning_tips_lab, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ir_learning_tips_lab, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_color(ir_learning_tips_lab, lv_color_make(40, 40, 40), LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ir_learning_tips_lab, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ir_learning_tips_lab, &font_cn_gb2_16, LV_PART_MAIN | LV_STATE_DEFAULT);


    if (IR_LEARNING_STATE & xEventGroupGetBits(sensor_monitor_event_grp)) {
        lv_obj_clear_flag(btn_ir_setting, LV_OBJ_FLAG_HIDDEN);
        if (SR_LANG_EN == param->sr_lang) {
            lv_label_set_text(ir_learning_tips_lab,
                              "Click the buton or say\n 'Turn on/off the Air'\n to control your air.");
        } else {
            lv_label_set_text(ir_learning_tips_lab,
                              "Click the buton or say\n '打开/关闭空调'\n to control your air.");
        }
    } else {
        lv_obj_add_flag(btn_ir_setting, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ir_learning_tips_lab,
                          "To control your AC, IR learning is required. Click the button below to begin the learning.");
    }

    ir_learning_btn = lv_btn_create(air_ctrl_panel);
    lv_obj_add_style(ir_learning_btn, &ui_button_styles()->style, 0);
    lv_obj_add_style(ir_learning_btn, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(ir_learning_btn, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(ir_learning_btn, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);
    lv_obj_set_size(ir_learning_btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(ir_learning_btn, LV_ALIGN_CENTER, 0, 37);
    lv_obj_add_event_cb(ir_learning_btn, ui_sensor_monitor_btn_ir_learning_event, LV_EVENT_CLICKED, page);
    lv_obj_t *ir_learning_btn_lab = lv_label_create(ir_learning_btn);
    lv_label_set_text(ir_learning_btn_lab, "OK Let's Go");
    lv_obj_set_style_text_color(ir_learning_btn_lab, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
    lv_obj_center(ir_learning_btn_lab);
    if (IR_LEARNING_STATE & xEventGroupGetBits(sensor_monitor_event_grp)) {
        lv_obj_add_flag(ir_learning_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(ir_learning_btn, LV_OBJ_FLAG_HIDDEN);
    }

    ir_learning_prompt_words = lv_label_create(air_ctrl_panel);
    lv_obj_set_width(ir_learning_prompt_words, 240);
    lv_obj_set_height(ir_learning_prompt_words, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ir_learning_prompt_words, 5);
    lv_obj_set_y(ir_learning_prompt_words, -10);
    lv_obj_set_align(ir_learning_prompt_words, LV_ALIGN_CENTER);
    lv_obj_add_flag(ir_learning_prompt_words, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_color(ir_learning_prompt_words, lv_color_make(40, 40, 40), LV_STATE_DEFAULT);
    lv_label_set_text(ir_learning_prompt_words,
                      "1. Point your controller towards the IR module.\n2. Press the power button of the controller.");
    lv_obj_set_style_text_align(ir_learning_prompt_words, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ir_learning_prompt_words, &font_cn_gb2_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ir_learning_state_lab = lv_label_create(air_ctrl_panel);
    lv_obj_set_width(ir_learning_state_lab, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ir_learning_state_lab, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ir_learning_state_lab, 0);
    lv_obj_set_y(ir_learning_state_lab, 40);
    lv_obj_set_align(ir_learning_state_lab, LV_ALIGN_CENTER);
    lv_obj_add_flag(ir_learning_state_lab, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(ir_learning_state_lab, "Press the button");
    lv_obj_set_style_text_font(ir_learning_state_lab, &font_cn_gb2_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ir_learning_state_lab, lv_color_make(206, 36, 79), LV_STATE_DEFAULT);

    ac_switch_btn = lv_btn_create(air_ctrl_panel);
    lv_obj_add_style(ac_switch_btn, &ui_button_styles()->style, 0);
    lv_obj_add_style(ac_switch_btn, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(ac_switch_btn, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(ac_switch_btn, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);
    lv_obj_set_size(ac_switch_btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(ac_switch_btn, LV_ALIGN_CENTER, 0, 32);
    lv_obj_add_event_cb(ac_switch_btn, ui_sensor_monitor_btn_ac_switch_event, LV_EVENT_CLICKED, page);
    if (IR_LEARNING_STATE & xEventGroupGetBits(sensor_monitor_event_grp)) {
        lv_obj_clear_flag(ac_switch_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ac_switch_btn, LV_OBJ_FLAG_HIDDEN);
    }
    ac_switch_btn_lab = lv_label_create(ac_switch_btn);
    if (AIR_POWER_STATE & xEventGroupGetBits(sensor_monitor_event_grp)) {
        lv_label_set_text(ac_switch_btn_lab, "Turn off the air");
    } else {
        lv_label_set_text(ac_switch_btn_lab, "Turn on the air");
    }
    lv_obj_set_style_text_color(ac_switch_btn_lab, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
    lv_obj_center(ac_switch_btn_lab);

#if !CONFIG_BSP_BOARD_ESP32_S3_BOX_Lite
    bsp_btn_register_callback(BSP_BUTTON_MAIN, BUTTON_PRESS_UP, btn_return_down_cb, (void *)btn_return);
#endif

    esp_err_t ret = bsp_board_get_sensor_handle()->get_humiture(&temperature, &humidity);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read AHT21: %d", ret);
        xEventGroupClearBits(sensor_monitor_event_grp, SENSOR_BASE_CONNECT_STATE);
        lv_obj_add_flag(btn_return, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(temp_sensor_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(radar_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(air_ctrl_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(relearning_lab, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(temp_value_label, "0");
        lv_label_set_text(hum_value_label, "0");
        esp_sensor_base_img = lv_img_create(page);
        lv_obj_align(esp_sensor_base_img, LV_ALIGN_CENTER, 0, -60);
        lv_img_set_src(esp_sensor_base_img, sensor_monitor_img_src_list[2].img);

        tips_lab = lv_label_create(page);
        lv_label_set_text(tips_lab, "This function needs the\n sensor accessory. \nPlease mount the esp-box onto it.");
        lv_obj_set_style_text_color(tips_lab, lv_color_make(40, 40, 40), LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(tips_lab, &font_cn_gb2_16, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(tips_lab, LV_ALIGN_CENTER, 0, 15);
        lv_obj_set_style_text_align(tips_lab, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

        btn_next = lv_btn_create(page);
        lv_obj_add_style(btn_next, &ui_button_styles()->style, 0);
        lv_obj_add_style(btn_next, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
        lv_obj_add_style(btn_next, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
        lv_obj_add_style(btn_next, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);
        lv_obj_set_size(btn_next, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_align(btn_next, LV_ALIGN_CENTER, 0, 70);
        lv_obj_t *btn_next_lab = lv_label_create(btn_next);
        lv_label_set_text(btn_next_lab, "OK Let's Go");
        lv_obj_set_style_text_color(btn_next_lab, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
        lv_obj_center(btn_next_lab);
        lv_obj_add_event_cb(btn_next, ui_next_btn_event, LV_EVENT_CLICKED, page);

        lv_group_add_obj(ui_get_btn_op_group(), btn_next);
    }
    timer_handle = lv_timer_create(update_timer_cb, UPDATE_TIME_PERIOD, NULL);

    if (ui_get_btn_op_group()) {
        lv_group_add_obj(ui_get_btn_op_group(), btn_return);
        lv_group_add_obj(ui_get_btn_op_group(), btn_ir_setting);
        lv_group_add_obj(ui_get_btn_op_group(), radar_btn);
        lv_group_add_obj(ui_get_btn_op_group(), ir_learning_btn_lab);
        lv_group_add_obj(ui_get_btn_op_group(), ac_switch_btn);
        lv_group_add_obj(ui_get_btn_op_group(), air_switch_reversal_btn);
        lv_group_add_obj(ui_get_btn_op_group(), relearning_btn);
        lv_group_add_obj(ui_get_btn_op_group(), ir_learning_settings_close_btn);
    }
}
