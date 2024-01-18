/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "app_button.h"

button_handle_t btns[16] = {0};
uint32_t g_pressed_button_value = 0;
rc_channel_state_t channel_state = {0};

void vibration_motor_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

static void vibration_motor_open(void)
{
    uint8_t vibration_feedback_state = get_vibration_feedback_state();
    if (vibration_feedback_state) {
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
    }
}

static void vibration_motor_close(void)
{
    uint8_t vibration_feedback_state = get_vibration_feedback_state();
    if (vibration_feedback_state) {
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
    }
}

static void up_button_press_down_cb(void *arg, void *data)
{
    ESP_LOGI(BUTTON_TAG, "up button.");
    g_pressed_button_value |= UP_BUTTON_MASK;
    vibration_motor_open();
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_upBtn, lv_color_hex(0xF2A860), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void up_button_press_up_cb(void *arg, void *data)
{
    vibration_motor_close();
    g_pressed_button_value &= ~UP_BUTTON_MASK;
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_upBtn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void left_button_press_down_cb(void *arg, void *data)
{
    ESP_LOGI(BUTTON_TAG, "left button.");
    g_pressed_button_value |= LEFT_BUTTON_MASK;
    vibration_motor_open();
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_leftBtn, lv_color_hex(0xF2A860), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void left_button_press_up_cb(void *arg, void *data)
{
    vibration_motor_close();
    g_pressed_button_value &= ~LEFT_BUTTON_MASK;
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_leftBtn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void down_button_press_down_cb(void *arg, void *data)
{
    ESP_LOGI(BUTTON_TAG, "down button.");
    g_pressed_button_value |= DOWN_BUTTON_MASK;
    vibration_motor_open();
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_downBtn, lv_color_hex(0xF2A860), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void down_button_press_up_cb(void *arg, void *data)
{
    vibration_motor_close();
    g_pressed_button_value &= ~DOWN_BUTTON_MASK;
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_downBtn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void right_button_press_down_cb(void *arg, void *data)
{
    ESP_LOGI(BUTTON_TAG, "right button.");
    g_pressed_button_value |= RIGHT_BUTTON_MASK;
    vibration_motor_open();
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_rightBtn, lv_color_hex(0xF2A860), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void right_button_press_up_cb(void *arg, void *data)
{
    vibration_motor_close();
    g_pressed_button_value &= ~RIGHT_BUTTON_MASK;
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_rightBtn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void lb_button_press_down_cb(void *arg, void *data)
{
    ESP_LOGI(BUTTON_TAG, "LB button.");
    g_pressed_button_value |= LB_BUTTON_MASK;
    channel_state.channel_3_status = !channel_state.channel_3_status;
    vibration_motor_open();
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_lbBtn, lv_color_hex(0xF2A860), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void lb_button_press_up_cb(void *arg, void *data)
{
    vibration_motor_close();
    g_pressed_button_value &= ~LB_BUTTON_MASK;
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_lbBtn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void lt_button_press_down_cb(void *arg, void *data)
{
    ESP_LOGI(BUTTON_TAG, "LT button.");
    g_pressed_button_value |= LT_BUTTON_MASK;
    vibration_motor_open();
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_ltBtn, lv_color_hex(0xF2A860), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void lt_button_press_up_cb(void *arg, void *data)
{
    vibration_motor_close();
    g_pressed_button_value &= ~LT_BUTTON_MASK;
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_ltBtn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void select_button_press_down_cb(void *arg, void *data)
{
    ESP_LOGI(BUTTON_TAG, "select button.");
    g_pressed_button_value |= SELECT_BUTTON_MASK;
    channel_state.channel_1_status = !channel_state.channel_1_status;
    vibration_motor_open();
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_selectBtn, lv_color_hex(0xF2A860), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void select_button_press_up_cb(void *arg, void *data)
{
    vibration_motor_close();
    g_pressed_button_value &= ~SELECT_BUTTON_MASK;
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_selectBtn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void x_button_press_down_cb(void *arg, void *data)
{
    ESP_LOGI(BUTTON_TAG, "X button.");
    g_pressed_button_value |= X_BUTTON_MASK;
    vibration_motor_open();
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_xBtn, lv_color_hex(0xF2A860), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void x_button_press_up_cb(void *arg, void *data)
{
    vibration_motor_close();
    g_pressed_button_value &= ~X_BUTTON_MASK;
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_xBtn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void y_button_press_down_cb(void *arg, void *data)
{
    ESP_LOGI(BUTTON_TAG, "Y button.");
    g_pressed_button_value |= Y_BUTTON_MASK;
    vibration_motor_open();
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_yBtn, lv_color_hex(0xF2A860), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();

}

static void y_button_press_up_cb(void *arg, void *data)
{
    vibration_motor_close();
    g_pressed_button_value &= ~Y_BUTTON_MASK;
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_yBtn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void a_button_press_down_cb(void *arg, void *data)
{
    ESP_LOGI(BUTTON_TAG, "A button.");
    g_pressed_button_value |= A_BUTTON_MASK;
    channel_state.channel_2_status = 1;
    vibration_motor_open();
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_aBtn, lv_color_hex(0xF2A860), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void a_button_press_up_cb(void *arg, void *data)
{
    vibration_motor_close();
    g_pressed_button_value &= ~A_BUTTON_MASK;
    channel_state.channel_2_status = 0;
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_aBtn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void b_button_press_down_cb(void *arg, void *data)
{
    ESP_LOGI(BUTTON_TAG, "B button.");
    g_pressed_button_value |= B_BUTTON_MASK;
    vibration_motor_open();
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_bBtn, lv_color_hex(0xF2A860), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void b_button_press_up_cb(void *arg, void *data)
{
    vibration_motor_close();
    g_pressed_button_value &= ~B_BUTTON_MASK;
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_bBtn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void rb_button_press_down_cb(void *arg, void *data)
{
    ESP_LOGI(BUTTON_TAG, "RB button.");
    g_pressed_button_value |= RB_BUTTON_MASK;
    channel_state.channel_4_status = !channel_state.channel_4_status;
    vibration_motor_open();
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_rbBtn, lv_color_hex(0xF2A860), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void rb_button_press_up_cb(void *arg, void *data)
{
    vibration_motor_close();
    g_pressed_button_value &= ~RB_BUTTON_MASK;
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_rbBtn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void rt_button_press_down_cb(void *arg, void *data)
{
    ESP_LOGI(BUTTON_TAG, "RT button.");
    g_pressed_button_value |= RT_BUTTON_MASK;
    vibration_motor_open();
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_rtBtn, lv_color_hex(0xF2A860), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void rt_button_press_up_cb(void *arg, void *data)
{
    vibration_motor_close();
    g_pressed_button_value &= ~RT_BUTTON_MASK;
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_rtBtn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void start_button_press_down_cb(void *arg, void *data)
{
    ESP_LOGI(BUTTON_TAG, "start button.");
    g_pressed_button_value |= START_BUTTON_MASK;
    vibration_motor_open();
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_startBtn, lv_color_hex(0xF2A860), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void start_button_press_up_cb(void *arg, void *data)
{
    vibration_motor_close();
    g_pressed_button_value &= ~START_BUTTON_MASK;
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_startBtn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void left_rocker_button_press_down_cb(void *arg, void *data)
{
    ESP_LOGI(BUTTON_TAG, "left rocker button.");
    g_pressed_button_value |= LEFT_ROCKER_BUTTON_MASK;
    vibration_motor_open();
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_leftRockerBtn, lv_color_hex(0xF2A860), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void left_rocker_button_press_up_cb(void *arg, void *data)
{
    vibration_motor_close();
    g_pressed_button_value &= ~LEFT_ROCKER_BUTTON_MASK;
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_leftRockerBtn, lv_color_hex(0xA8A8A8), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void right_rocker_button_press_down_cb(void *arg, void *data)
{
    ESP_LOGI(BUTTON_TAG, "right rocker button.");
    g_pressed_button_value |= RIGHT_ROCKER_BUTTON_MASK;
    vibration_motor_open();
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_rightRockerBtn, lv_color_hex(0xF2A860), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void right_rocker_button_press_up_cb(void *arg, void *data)
{
    vibration_motor_close();
    g_pressed_button_value &= ~RIGHT_ROCKER_BUTTON_MASK;
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(ui_rightRockerBtn, lv_color_hex(0xA8A8A8), LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

button_cb_t button_press_down_cb[16] = {
    up_button_press_down_cb,
    left_button_press_down_cb,
    down_button_press_down_cb,
    right_button_press_down_cb,
    lb_button_press_down_cb,
    lt_button_press_down_cb,
    select_button_press_down_cb,
    left_rocker_button_press_down_cb,
    y_button_press_down_cb,
    x_button_press_down_cb,
    a_button_press_down_cb,
    b_button_press_down_cb,
    rb_button_press_down_cb,
    rt_button_press_down_cb,
    start_button_press_down_cb,
    right_rocker_button_press_down_cb,
};

button_cb_t button_press_up_cb[16] = {
    up_button_press_up_cb,
    left_button_press_up_cb,
    down_button_press_up_cb,
    right_button_press_up_cb,
    lb_button_press_up_cb,
    lt_button_press_up_cb,
    select_button_press_up_cb,
    left_rocker_button_press_up_cb,
    y_button_press_up_cb,
    x_button_press_up_cb,
    a_button_press_up_cb,
    b_button_press_up_cb,
    rb_button_press_up_cb,
    rt_button_press_up_cb,
    start_button_press_up_cb,
    right_rocker_button_press_up_cb,
};

static void configure_74hc165_pin(void)
{
    ESP_LOGI(BUTTON_TAG, "Configure 74hc165d GPIO!");
    gpio_reset_pin(HC165D_PL);
    gpio_reset_pin(HC165D_CE);
    gpio_reset_pin(HC165D_SCL);
    gpio_reset_pin(HC165D_DATA);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(HC165D_PL, GPIO_MODE_OUTPUT);
    gpio_set_direction(HC165D_CE, GPIO_MODE_OUTPUT);
    gpio_set_direction(HC165D_SCL, GPIO_MODE_OUTPUT);
    gpio_set_direction(HC165D_DATA, GPIO_MODE_INPUT);
    gpio_set_level(HC165D_PL, 1);
    gpio_set_level(HC165D_CE, 0);
    gpio_set_level(HC165D_SCL, 1);
}

static uint16_t read_74hc165d_data(void)
{
    uint16_t data = 0;
    uint8_t i = 0;
    uint8_t q7_bit = 0;
    gpio_set_level(HC165D_PL, 0);
    gpio_set_level(HC165D_PL, 1);
    q7_bit = gpio_get_level(HC165D_DATA);
    if (q7_bit == 1) {
        data |= 0x01;
    }
    for (i = 0; i < 15; i++) {
        data = data << 1;
        gpio_set_level(HC165D_SCL, 0);
        gpio_set_level(HC165D_SCL, 1);
        q7_bit = gpio_get_level(HC165D_DATA);
        if (q7_bit == 1) {
            data |= 0x01;
        }
    }
    return data;
}

static uint8_t get_button_value(void *btn_index)
{
    int button_index = (int)btn_index;
    uint16_t data = read_74hc165d_data();
    uint16_t mask = 1 << button_index;
    uint8_t button_value = (data & mask) >> button_index;
    return button_value;
}

void box_rc_button_init(void)
{
    configure_74hc165_pin();
    button_config_t button_cfg = {
        .type = BUTTON_TYPE_CUSTOM,
        .long_press_time = 1000,
        .short_press_time = 180,
        .custom_button_config = {
            .button_custom_get_key_value = get_button_value,
        },
    };

    for (int i = 0; i < 16; i++) {
        if (btns[i] == NULL) {
            button_cfg.custom_button_config.priv = (void *)i;
            if (i == 7 || i == 15) {
                button_cfg.custom_button_config.active_level = 1;
            } else {
                button_cfg.custom_button_config.active_level = 0;
            }
            btns[i] = iot_button_create(&button_cfg);
            iot_button_register_cb(btns[i], BUTTON_PRESS_DOWN, button_press_down_cb[i], NULL);
            iot_button_register_cb(btns[i], BUTTON_PRESS_UP, button_press_up_cb[i], NULL);
        }
    }
}

void box_rc_button_delete(void)
{
    for (int i = 0; i < 16; i++) {
        iot_button_delete(btns[i]);
        btns[i] = NULL;
        ESP_LOGI(BUTTON_TAG, "button %d deleted.\n", i);
    }
}

uint32_t get_pressed_button_value(void)
{
    return g_pressed_button_value;
}

rc_channel_state_t get_rc_button_state(void)
{
    return channel_state;
}
