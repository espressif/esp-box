/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/i2s_std.h"
#include "iot_button.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_I2C_EXPAND_SCL      (GPIO_NUM_40)
#define BSP_I2C_EXPAND_SDA      (GPIO_NUM_41)

#define BSP_RADAR_OUT_IO        (GPIO_NUM_21)
#define BSP_IR_CTRL_GPIO        (GPIO_NUM_44)
#define BSP_IR_TX_GPIO          (GPIO_NUM_39)
#define BSP_IR_RX_GPIO          (GPIO_NUM_38)

typedef enum {
    BOARD_S3_BOX,
    BOARD_S3_BOX_LITE,
} boards_id_t;

typedef enum{
    BOTTOM_ID_SENSOR,
    BOTTOM_ID_UNKNOW,
    BOTTOM_ID_LOST,
}bottom_id_t;

typedef enum {
#if CONFIG_BSP_BOARD_ESP32_S3_BOX
    BOARD_BTN_ID_BOOT = 0,
    BOARD_BTN_ID_MUTE,
    BOARD_BTN_ID_HOME,
    BOARD_BTN_ID_NUM
#elif CONFIG_BSP_BOARD_ESP32_S3_BOX_Lite
    BOARD_BTN_ID_BOOT = 0,
    BOARD_BTN_ID_NUM
#endif
} bsp_button_id_t;

typedef struct {
    gpio_num_t row1[4]; //The first row
    gpio_num_t row2[4];
} pmod_pins_t;

typedef struct {
    /**
     * @brief ESP-Dev-Board SDMMC GPIO defination
     *
     * @note Only avaliable when PMOD connected
     */
    bool FUNC_SDMMC_EN   ;
    int SDMMC_BUS_WIDTH ;
    int GPIO_SDMMC_CLK  ;
    int GPIO_SDMMC_CMD  ;
    int GPIO_SDMMC_D0   ;
    int GPIO_SDMMC_D1   ;
    int GPIO_SDMMC_D2   ;
    int GPIO_SDMMC_D3   ;
    int GPIO_SDMMC_DET  ;

    /**
     * @brief ESP-Dev-Board SDSPI GPIO definationv
     *
     */
    bool FUNC_SDSPI_EN       ;
    int SDSPI_HOST          ;
    int GPIO_SDSPI_CS       ;
    int GPIO_SDSPI_SCLK     ;
    int GPIO_SDSPI_MISO     ;
    int GPIO_SDSPI_MOSI     ;

    /**
     * @brief ESP-Dev-Board SPI GPIO defination
     *
     */
    bool FUNC_SPI_EN         ;
    int GPIO_SPI_CS         ;
    int GPIO_SPI_MISO       ;
    int GPIO_SPI_MOSI       ;
    int GPIO_SPI_SCLK       ;

    /**
     * @brief ESP-Dev-Board RMT GPIO defination
     *
     */
    bool FUNC_RMT_EN         ;
    int GPIO_RMT_IR         ;
    int GPIO_RMT_LED        ;

    const pmod_pins_t *PMOD1;
    const pmod_pins_t *PMOD2;

} board_res_desc_t;

typedef esp_err_t (*bsp_codec_reconfig_fn)();

typedef esp_err_t (*bsp_codec_mute_fn)(bool enable);

typedef esp_err_t (*bsp_codec_volume_fn)(int volume, int *volume_set);

typedef esp_err_t (*bsp_i2s_reconfig_clk_fn)(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch);

typedef esp_err_t (*bsp_i2s_read_fn)(void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms);

typedef esp_err_t (*bsp_i2s_write_fn)(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms);

typedef struct {
    bsp_codec_mute_fn mute_set_fn;
    bsp_codec_volume_fn volume_set_fn;
    bsp_codec_reconfig_fn codec_reconfig_fn;

    bsp_i2s_read_fn i2s_read_fn;
    bsp_i2s_write_fn i2s_write_fn;
    bsp_i2s_reconfig_clk_fn i2s_reconfig_clk_fn;
} bsp_codec_config_t;

typedef struct {
    boards_id_t id;
    const char *name;
    /**
     * @brief Get board description
     */
    const board_res_desc_t *board_desc;

} boards_info_t;

/**
 * @brief Special config for dev board
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_board_init(void);

/**
 * @brief Get the board information
 *
 * @return pointer of boards_info_t
 */
const boards_info_t *bsp_board_get_info(void);

/**
 * @brief Get board description
 *
 * @return pointer of board_res_desc_t
 */
const board_res_desc_t *bsp_board_get_description(void);

/**
 * @brief Get the codec operation function
 *
 * @return pointer of bsp_codec_config
 */
bsp_codec_config_t *bsp_board_get_codec_handle(void);

/**
 * @brief Call default button init code
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_btn_init(void);

/**
 * @brief Register the button event callback function.
 *
 * @param btn A button handle to register
 * @param event Button event
 * @param callback Callback function.
 * @param user_data user data
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG   Arguments is invalid.
 */
esp_err_t bsp_btn_register_callback(bsp_button_id_t btn, button_event_t event, button_cb_t callback, void *user_data);

/**
 * @brief Unregister the button event callback function.
 *
 * @param btn A button handle to unregister
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG   Arguments is invalid.
 */
esp_err_t bsp_btn_rm_all_callback(bsp_button_id_t btn);

esp_err_t bsp_btn_rm_event_callback(bsp_button_id_t btn, size_t event);
/**
 * @brief stop codec to enter sleep mode
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_codec_dev_stop(void);

/**
 * @brief resume codec to enter normal mode
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_codec_dev_resume(void);

/**
 * @brief esp_pm_lock_acquire
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_pm_exit_sleep();

/**
 * @brief esp_pm_lock_release
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_pm_enter_sleep();

/**
 * @brief init pm module
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_pm_init();

/**
 * @brief get sleep mode
 *
 * @return
 *    - true: sleep mode
 *    - false: nornal mode
 */
bool bsp_get_system_sleep_mode();

/**
 * @brief get bottom status
 *
 * @return
 *    - BOTTOM_ID_SENSOR: sensor bottom connected
 *    - BOTTOM_ID_LOST: sensor bottom lost
 *    - BOTTOM_ID_UNKNOW: unknow
 */
bottom_id_t bsp_get_bottom_id();

/**
 * @brief get radar status
 *
 * @return
 *    - true: active
 *    - false: passive
 */
bool bsp_get_system_radar_status();

/**
 * @brief set radar status
 *
 * @param enable
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG   Arguments is invalid.
 */
void bsp_set_system_radar_status(bool enable);

/**
 * @brief get sleep mode
 *
 * @return
 *    - ESP_OK: read successfully
 *    - ESP_FAIL: read failed
 */
esp_err_t bsp_read_temp_humidity(float *temperature_s, uint8_t *humidity_s);

#ifdef __cplusplus
}
#endif
