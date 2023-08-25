/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_log.h"
#include "esp_check.h"
#include "esp_pm.h"

#include "bsp/esp-bsp.h"
#include "bsp_board.h"

#include "iot_button.h"
#include "esp_lvgl_port.h"

#ifdef CONFIG_NEED_SENSOR
#include "aht20.h"
#include "at581x.h"
#endif

#define BSP_I2C_EXPAND_NUM              ((1 == BSP_I2C_NUM) ? (0):(1))
#define BSP_I2C_EXPAND_CLK_SPEED_HZ     CONFIG_BSP_I2C_CLK_SPEED_HZ

#define ES8311_SAMPLE_RATE              (16000)
#define ES8311_DEFAULT_VOLUME           (60)

#define ES7210_SAMPLE_RATE              (16000)
#define ES7210_BIT_WIDTH                (16)
#define ES7210_ADC_VOLUME               (24.0)
#define ES7210_CHANNEL                  (2)

#define RADAE_POWER_DELAY               (60 * 2) // 1min
#define RADAE_FUNC_STOP                 (RADAE_POWER_DELAY + 1)

static uint8_t bsp_home_button_get(void *param);

static const pmod_pins_t g_pmod[2] = {
    {
        {BSP_PMOD2_IO5, BSP_PMOD2_IO6, BSP_PMOD2_IO7, BSP_PMOD2_IO8},
        {BSP_PMOD2_IO1, BSP_PMOD2_IO2, BSP_PMOD2_IO3, BSP_PMOD2_IO4},
    },
    {
        {BSP_PMOD1_IO5, BSP_PMOD1_IO6, BSP_PMOD1_IO7, BSP_PMOD1_IO8},
        {BSP_PMOD1_IO1, BSP_PMOD1_IO2, BSP_PMOD1_IO3, BSP_PMOD1_IO4},
    },
};

static const board_res_desc_t g_board_s3_box_res = {

    .FUNC_SDMMC_EN =   (1),
    .SDMMC_BUS_WIDTH = (4),
    .GPIO_SDMMC_CLK =  (BSP_PMOD2_IO3),
    .GPIO_SDMMC_CMD =  (BSP_PMOD2_IO2),
    .GPIO_SDMMC_D0 =   (BSP_PMOD2_IO8),
    .GPIO_SDMMC_D1 =   (BSP_PMOD2_IO4),
    .GPIO_SDMMC_D2 =   (BSP_PMOD2_IO1),
    .GPIO_SDMMC_D3 =   (BSP_PMOD2_IO5),
    .GPIO_SDMMC_DET =  (GPIO_NUM_NC),

    .FUNC_SDSPI_EN =       (0),
    .SDSPI_HOST =          (SPI2_HOST),
    .GPIO_SDSPI_CS =       (GPIO_NUM_NC),
    .GPIO_SDSPI_SCLK =     (GPIO_NUM_NC),
    .GPIO_SDSPI_MISO =     (GPIO_NUM_NC),
    .GPIO_SDSPI_MOSI =     (GPIO_NUM_NC),

    .FUNC_SPI_EN =         (0),
    .GPIO_SPI_CS =         (GPIO_NUM_NC),
    .GPIO_SPI_MISO =       (GPIO_NUM_NC),
    .GPIO_SPI_MOSI =       (GPIO_NUM_NC),
    .GPIO_SPI_SCLK =       (GPIO_NUM_NC),

    .FUNC_RMT_EN =         (0),
    .GPIO_RMT_IR =         (GPIO_NUM_NC),
    .GPIO_RMT_LED =        (GPIO_NUM_39),

    .PMOD1 = &g_pmod[0],
    .PMOD2 = &g_pmod[1],
};

static const button_config_t BOARD_BTN_ID_config[BOARD_BTN_ID_NUM] = {
    {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config.active_level = false,
        .gpio_button_config.gpio_num = BOARD_BTN_ID_BOOT,
    },
    {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config.active_level = false,
        .gpio_button_config.gpio_num = BOARD_BTN_ID_MUTE,
    },
    {
        .type = BUTTON_TYPE_CUSTOM,
        .custom_button_config.priv = NULL,
        .custom_button_config.button_custom_get_key_value = bsp_home_button_get,
        .custom_button_config.active_level = true,
    },
};

static esp_pm_lock_handle_t g_pm_apb_lock = NULL;
static esp_pm_lock_handle_t g_pm_light_lock = NULL;
static esp_pm_lock_handle_t g_pm_cpu_lock = NULL;

static bool sys_sleep_entered = false;
static bottom_id_t sys_bottom_id;

static float sys_temperature_value;
static uint8_t sys_humidity_value;
static uint16_t power_off_delay;
#ifdef CONFIG_NEED_SENSOR
static aht20_dev_handle_t aht20 = NULL;
#endif

static esp_codec_dev_handle_t play_dev_handle;
static esp_codec_dev_handle_t record_dev_handle;

static button_handle_t *g_btn_handle = NULL;
static bsp_codec_config_t g_codec_handle;

static const boards_info_t g_boards_info = {
    .id =           BOARD_S3_BOX,
#ifdef CONFIG_BSP_ESP32_S3_BOX_3
    .name =         "S3_BOX_3",
#else
    .name =         "S3_BOX",
#endif
    .board_desc =   &g_board_s3_box_res
};

static const char *TAG = "board";

static uint8_t bsp_home_button_get(void *param)
{
#if CONFIG_BSP_TOUCH_BUTTON
    return bsp_button_get(BSP_BUTTON_MAIN);
#else
    return false;
#endif
}

esp_err_t bsp_btn_init(void)
{
    ESP_ERROR_CHECK((NULL != g_btn_handle));

    g_btn_handle = calloc(sizeof(button_handle_t), BOARD_BTN_ID_NUM);
    assert((g_btn_handle) && "memory is insufficient for button");

    /* Init buttons */
    for (int i = 0; i < BOARD_BTN_ID_NUM; i++) {
        g_btn_handle[i] = iot_button_create(&BOARD_BTN_ID_config[i]);
        assert(g_btn_handle[i]);
    }
    return ESP_OK;
}

esp_err_t bsp_btn_register_callback(bsp_button_id_t btn, button_event_t event, button_cb_t callback, void *user_data)
{
    assert((g_btn_handle) && "button not initialized");
    assert((btn < BOARD_BTN_ID_NUM) && "button id incorrect");

    if (NULL == callback) {
        return iot_button_unregister_cb(g_btn_handle[btn], event);
    }

    return iot_button_register_cb(g_btn_handle[btn], event, callback, user_data);
}

esp_err_t bsp_btn_rm_all_callback(bsp_button_id_t btn)
{
    assert((g_btn_handle) && "button not initialized");
    assert((btn < BOARD_BTN_ID_NUM) && "button id incorrect");

    for (size_t event = 0; event < BUTTON_EVENT_MAX; event++) {
        iot_button_unregister_cb(g_btn_handle[btn], event);
    }
    return ESP_OK;
}

esp_err_t bsp_btn_rm_event_callback(bsp_button_id_t btn, size_t event)
{
    assert((g_btn_handle) && "button not initialized");
    assert((btn < BOARD_BTN_ID_NUM) && "button id incorrect");

    iot_button_unregister_cb(g_btn_handle[btn], event);
    return ESP_OK;
}

static esp_err_t bsp_i2s_read(void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_read(record_dev_handle, audio_buffer, len);
    *bytes_read = len;
    return ret;
}

static esp_err_t bsp_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_write(play_dev_handle, audio_buffer, len);
    *bytes_written = len;
    return ret;
}

static esp_err_t bsp_codec_es8311_set(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    esp_err_t ret = ESP_OK;

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = rate,
        .channel = ch,
        .bits_per_sample = bits_cfg,
    };

    ret = esp_codec_dev_close(play_dev_handle);
    ret = esp_codec_dev_close(record_dev_handle);

    ret = esp_codec_dev_open(play_dev_handle, &fs);
    ret = esp_codec_dev_open(record_dev_handle, &fs);
    return ret;
}

static esp_err_t bsp_codec_volume_set(int volume, int *volume_set)
{
    esp_err_t ret = ESP_OK;
    float v = volume;
    ret = esp_codec_dev_set_out_vol(play_dev_handle, (int)v);
    return ret;
}

static esp_err_t bsp_codec_mute_set(bool enable)
{
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_set_out_mute(play_dev_handle, enable);
    return ret;
}

static esp_err_t bsp_codec_es7210_set()
{
    esp_err_t ret = ESP_OK;

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = ES7210_SAMPLE_RATE,
        .channel = ES7210_CHANNEL,
        .bits_per_sample = ES7210_BIT_WIDTH,
    };

    assert(record_dev_handle);

    if (play_dev_handle) {
        ret = esp_codec_dev_close(play_dev_handle);
    }

    if (record_dev_handle) {
        ret = esp_codec_dev_close(record_dev_handle);
    }
    ret = esp_codec_dev_open(record_dev_handle, &fs);
    esp_codec_dev_set_in_gain(record_dev_handle, ES7210_ADC_VOLUME);
    return ret;
}

static void bsp_codec_init()
{
    play_dev_handle = bsp_audio_codec_speaker_init();
    assert((play_dev_handle) && "play_dev_handle not initialized");

    record_dev_handle = bsp_audio_codec_microphone_init();
    assert((record_dev_handle) && "record_dev_handle not initialized");

    bsp_codec_es7210_set();
    bsp_codec_es8311_set(ES7210_SAMPLE_RATE, ES7210_BIT_WIDTH, ES7210_CHANNEL);

    bsp_codec_config_t *codec_config = bsp_board_get_codec_handle();
    codec_config->volume_set_fn = bsp_codec_volume_set;
    codec_config->mute_set_fn = bsp_codec_mute_set;
    codec_config->codec_reconfig_fn = bsp_codec_es7210_set;
    codec_config->i2s_read_fn = bsp_i2s_read;
    codec_config->i2s_write_fn = bsp_i2s_write;
    codec_config->i2s_reconfig_clk_fn = bsp_codec_es8311_set;
}

__attribute__((weak)) void mute_btn_handler(void *handle, void *arg)
{
    button_event_t event = (button_event_t)arg;

    if (BUTTON_PRESS_DOWN == event) {
        esp_rom_printf(DRAM_STR("Mute On\r\n"));
    } else {
        esp_rom_printf(DRAM_STR("Mute Off\r\n"));
    }
}

esp_err_t bsp_board_s3_box_init(void)
{
    bsp_btn_init();
    bsp_btn_register_callback(BOARD_BTN_ID_MUTE, BUTTON_PRESS_DOWN, mute_btn_handler, (void *)BUTTON_PRESS_DOWN);
    bsp_btn_register_callback(BOARD_BTN_ID_MUTE, BUTTON_PRESS_UP, mute_btn_handler, (void *)BUTTON_PRESS_UP);

    /**
     * @brief Initialize I2S and audio codec
     *
     * @note Actually the sampling rate can be reconfigured.
     *       `MP3GetLastFrameInfo` can fill the `MP3FrameInfo`, which includes `samprate`.
     *       So theoretically, the sampling rate can be dynamically changed according to the MP3 frame information.
     */
    bsp_codec_init();

    return ESP_OK;
}

const boards_info_t *bsp_board_get_info(void)
{
    return &g_boards_info;
}

const board_res_desc_t *bsp_board_get_description(void)
{
    return g_boards_info.board_desc;
}

bsp_codec_config_t *bsp_board_get_codec_handle(void)
{
    return &g_codec_handle;
}

esp_err_t bsp_codec_dev_stop(void)
{
    esp_err_t ret = ESP_OK;

    if (play_dev_handle) {
        ret = esp_codec_dev_close(play_dev_handle);
    }

    if (record_dev_handle) {
        ret = esp_codec_dev_close(record_dev_handle);
    }
    return ret;
}

bool bsp_i2c_device_probe(i2c_port_t i2c_num, uint8_t addr)
{
    bool probe_result = false;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, addr | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    if (i2c_master_cmd_begin(i2c_num, cmd, 2000) == ESP_OK) {
        probe_result = true;
    }
    i2c_cmd_link_delete(cmd);
    return probe_result;
}

bool bsp_get_system_sleep_mode()
{
    return sys_sleep_entered;
}

bottom_id_t bsp_get_bottom_id()
{
    return sys_bottom_id;
}

bool bsp_get_system_radar_status()
{
    if (BOTTOM_ID_SENSOR == sys_bottom_id) {
        return ((power_off_delay > RADAE_POWER_DELAY / 2) ? (true) : (false));
    } else {
        return false;
    }
}

void bsp_set_system_radar_status(bool enable)
{
    if (enable) {
        power_off_delay = RADAE_POWER_DELAY;
    } else {
        power_off_delay = RADAE_FUNC_STOP;
    }
}

esp_err_t bsp_read_temp_humidity(float *temperature_s, uint8_t *humidity_s)
{
    if (BOTTOM_ID_SENSOR == sys_bottom_id) {
        *temperature_s = sys_temperature_value;
        *humidity_s = sys_humidity_value;
        return ESP_OK;
    } else {
        return ESP_FAIL;
    }
}

#ifdef CONFIG_NEED_SENSOR
static void low_power_monitor_task(void *arg)
{
    static uint8_t gpio_level_prev = 1;
    uint32_t temperature_raw, humidity_raw;
    uint8_t gpio_level;

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.pin_bit_mask = (1ULL << BSP_RADAR_OUT_IO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    vTaskDelay(pdMS_TO_TICKS(1500));

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));

        if (sys_bottom_id != BOTTOM_ID_UNKNOW) {
            if (bsp_i2c_device_probe(BSP_I2C_EXPAND_NUM, AT581X_ADDRRES_0)) {
                if (sys_bottom_id == BOTTOM_ID_LOST) {
                    ESP_LOGE(TAG, "Sensor bottom connected");
                    sys_bottom_id = BOTTOM_ID_SENSOR;
                }
            } else {
                if (sys_bottom_id == BOTTOM_ID_SENSOR) {
                    ESP_LOGE(TAG, "Sensor bottom lost");
                    sys_bottom_id = BOTTOM_ID_LOST;
                }
            }
        }

        if (BOTTOM_ID_SENSOR == sys_bottom_id) {
            gpio_level = gpio_get_level(BSP_RADAR_OUT_IO);
        } else {
            gpio_level = 1;
        }

        if (gpio_level_prev ^ gpio_level) {
            gpio_level_prev = gpio_level;
            if (gpio_level && (RADAE_FUNC_STOP != power_off_delay)) {
                power_off_delay = RADAE_POWER_DELAY;
                ESP_LOGI(TAG, "Radar: %s", "active");
            }
        }

        if ((gpio_level) && (true == sys_sleep_entered)) {
            bsp_pm_exit_sleep();

            ESP_LOGI(TAG, "power on");
            bsp_touch_exit_sleep();
            bsp_display_exit_sleep();
            bsp_display_backlight_on();

            lvgl_port_resume();
            iot_button_resume();
            bsp_codec_dev_resume();
            sys_sleep_entered = false;
        } else if ((1 == power_off_delay) && (BOTTOM_ID_SENSOR == sys_bottom_id)) {
            ESP_LOGI(TAG, "power off");
            sys_sleep_entered = true;

            bsp_touch_enter_sleep();
            bsp_display_enter_sleep();
            bsp_display_backlight_off();

            lvgl_port_stop();
            iot_button_stop();
            bsp_codec_dev_stop();
            bsp_pm_enter_sleep();
        }

        if ((RADAE_FUNC_STOP != power_off_delay) && power_off_delay \
                && (BOTTOM_ID_SENSOR == sys_bottom_id)) {
            power_off_delay--;
        }

        if (BOTTOM_ID_SENSOR == sys_bottom_id) {
            aht20_read_temperature_humidity(aht20, \
                                            &temperature_raw, &sys_temperature_value, \
                                            &humidity_raw, &sys_humidity_value);
        }
    }
}
#endif

esp_err_t bsp_init_temp_humudity()
{
    esp_err_t ret = ESP_OK;
#ifdef CONFIG_NEED_SENSOR
    aht20_i2c_config_t i2c_conf = {
        .i2c_port = BSP_I2C_EXPAND_NUM,
        .i2c_addr = AHT20_ADDRRES_0,
    };

    ret |= aht20_new_sensor(&i2c_conf, &aht20);
    ret |= aht20_init_sensor(aht20);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Temp & humidity init ok");
    }
#endif
    return ret;
}

esp_err_t bsp_init_radar()
{
    esp_err_t ret = ESP_OK;
#ifdef CONFIG_NEED_SENSOR
    at581x_dev_handle_t at581x;

    at581x_i2c_config_t i2c_conf = {
        .i2c_port = BSP_I2C_EXPAND_NUM,
        .i2c_addr = AT581X_ADDRRES_0,
    };

    ret |= at581x_new_sensor(&i2c_conf, &at581x);
    ret |= at581x_init_sensor(at581x);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Rader init ok");
    }
#endif
    return ret;
}

esp_err_t bsp_codec_dev_resume(void)
{
    esp_err_t ret = ESP_OK;

    if (play_dev_handle) {
        ret = bsp_codec_es7210_set();
    }
    if (record_dev_handle) {
        ret = bsp_codec_es8311_set(ES7210_SAMPLE_RATE, ES7210_BIT_WIDTH, ES7210_CHANNEL);
    }
    return ret;
}

esp_err_t bsp_pm_exit_sleep()
{
    esp_err_t ret = ESP_OK;

    ret |= esp_pm_lock_acquire(g_pm_apb_lock);
    ret |= esp_pm_lock_acquire(g_pm_light_lock);
    ret |= esp_pm_lock_acquire(g_pm_cpu_lock);
    return ret;
}

esp_err_t bsp_pm_enter_sleep()
{
    esp_err_t ret = ESP_OK;

    ret |= esp_pm_lock_release(g_pm_apb_lock);
    ret |= esp_pm_lock_release(g_pm_light_lock);
    ret |= esp_pm_lock_release(g_pm_cpu_lock);
    return ret;
}

esp_err_t bsp_pm_init()
{
    esp_err_t ret = ESP_OK;

#if CONFIG_PM_ENABLE
    esp_pm_config_t pm_config = {
        .max_freq_mhz = CONFIG_EXAMPLE_MAX_CPU_FREQ_MHZ,
        .min_freq_mhz = CONFIG_EXAMPLE_MIN_CPU_FREQ_MHZ,
#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
        .light_sleep_enable = true
#endif
    };
    ESP_ERROR_CHECK( esp_pm_configure(&pm_config));
#endif

    if (g_pm_apb_lock == NULL) {
        ESP_RETURN_ON_ERROR(esp_pm_lock_create(ESP_PM_APB_FREQ_MAX, 0, "l_apb", &g_pm_apb_lock),
                            TAG, "create l_apb pm lock failed");
    }

    if (g_pm_light_lock == NULL) {
        ESP_RETURN_ON_ERROR(esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "l_ls", &g_pm_light_lock),
                            TAG, "create l_ls pm lock failed");
    }
    if (g_pm_cpu_lock == NULL) {
        ESP_RETURN_ON_ERROR(esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "l_cpu", &g_pm_cpu_lock),
                            TAG, "create l_cpu pm lock failed");
    }

    bsp_pm_exit_sleep();
#ifdef CONFIG_NEED_SENSOR
    ret = xTaskCreatePinnedToCore(&low_power_monitor_task, "Lowpower Task", 4 * 1024, NULL, 5, NULL, 1);
    ESP_RETURN_ON_ERROR(pdPASS != ret, TAG,  "create Lowpower task failed");
#endif
    return ret;
}

esp_err_t bsp_i2c_expand_init(void)
{
    static bool i2c_initialized = false;

    /* I2C was initialized before */
    if (i2c_initialized) {
        return ESP_OK;
    }

    const i2c_config_t i2c_expand_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BSP_I2C_EXPAND_SDA,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_io_num = BSP_I2C_EXPAND_SCL,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = BSP_I2C_EXPAND_CLK_SPEED_HZ
    };
    ESP_RETURN_ON_ERROR(i2c_param_config(BSP_I2C_EXPAND_NUM, &i2c_expand_conf),
                        TAG, "create expand i2c failed");
    ESP_RETURN_ON_ERROR(i2c_driver_install(BSP_I2C_EXPAND_NUM, i2c_expand_conf.mode, 0, 0, 0),
                        TAG, "install expand i2c failed");

    i2c_initialized = true;

    ESP_LOGI(TAG, "I2C num: %d, [%d, %d], speed:%d",
             BSP_I2C_EXPAND_NUM, BSP_I2C_EXPAND_SCL, BSP_I2C_EXPAND_SDA, BSP_I2C_EXPAND_CLK_SPEED_HZ);

    return ESP_OK;
}

esp_err_t bsp_i2c_expand_deinit(void)
{
    ESP_RETURN_ON_ERROR(i2c_driver_delete(BSP_I2C_EXPAND_NUM),
                        TAG, "uninstall expand i2c failed");
    return ESP_OK;
}

esp_err_t bsp_board_init(void)
{
    esp_err_t ret = ESP_OK;

    ret |= bsp_board_s3_box_init();
#ifdef CONFIG_NEED_SENSOR
    ret |= bsp_pm_init();
    bsp_i2c_expand_init();

    if (bsp_i2c_device_probe(BSP_I2C_EXPAND_NUM, AT581X_ADDRRES_0)) {
        ESP_LOGW(TAG, "Sensor bottom connected");
        ret |= bsp_init_radar();
        ret |= bsp_init_temp_humudity();
        sys_bottom_id = BOTTOM_ID_SENSOR;
    } else {
        ESP_LOGW(TAG, "Sensor bottom lost");
        sys_bottom_id = BOTTOM_ID_UNKNOW;
        bsp_i2c_expand_deinit();
    }
#endif
    return ret;
}
