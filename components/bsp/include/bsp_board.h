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

#include "bsp/esp-bsp.h"
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
    BOTTOM_ID_SENSOR,   /*!< bottom is ESP32-S3-BOX-3-SENSOR */
    BOTTOM_ID_UNKNOW,   /*!< bottom isn't ESP32-S3-BOX-3-SENSOR */
    BOTTOM_ID_LOST,     /*!< bottom ESP32-S3-BOX-3-SENSOR is connect when poweron, and lost */
} bottom_id_t;

typedef struct {
    gpio_num_t row1[4]; /*!< first row */
    gpio_num_t row2[4]; /*!< second row */
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

/**
 * @brief Get sleep mode
 *
 * @return
 *    - true: sleep mode
 *    - false: nornal mode
 */
typedef bool (*bsp_sys_get_sleep_mode)();

/**
 * @brief Get radar status
 *
 * @return
 *    - true: active
 *    - false: passive
 */
typedef bool (*bsp_bottom_get_radar_status)();

/**
 * @brief Set radar onoff
 *
 * @param enable: Enable or disable radar module
 */
typedef void (*bsp_bottom_set_radar_enable)(bool enable);

/**
 * @brief Get bottom status
 *
 * @return
 *    - BOTTOM_ID_SENSOR: sensor bottom connected
 *    - BOTTOM_ID_LOST: sensor bottom lost
 *    - BOTTOM_ID_UNKNOW: unknow
 */
typedef bottom_id_t (*bsp_sys_get_bottom_id)();

/**
 * @brief Get temp and humidity data
 *
 * @param temperature: Output temperature
 * @param humidity: Output humidity
 *
 * @return
 *    - ESP_OK: read successfully
 *    - ESP_FAIL: read failed
 */
typedef esp_err_t (*bsp_bottom_get_humiture)(float *temperature, uint8_t *humidity);

/**
 * @brief Player set mute.
 *
 * @param enable: true or false
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_codec_mute_set(bool enable);

/**
 * @brief Player set volume.
 *
 * @param volume: volume set
 * @param volume_set: volume set response
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_codec_volume_set(int volume, int *volume_set);

/**
 * @brief Stop I2S function.
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_codec_dev_stop(void);

/**
 * @brief Resume I2S function.
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_codec_dev_resume(void);

/**
 * @brief Set I2S format to codec.
 *
 * @param rate: Sample rate of sample
 * @param bits_cfg: Bit lengths of one channel data
 * @param ch: Channels of sample
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_codec_set_fs(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch);

/**
 * @brief Read data from recoder.
 *
 * @param audio_buffer: The pointer of receiving data buffer
 * @param len: Max data buffer length
 * @param bytes_read: Byte number that actually be read, can be NULL if not needed
 * @param timeout_ms: Max block time
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_i2s_read(void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms);

/**
 * @brief Write data to player.
 *
 * @param audio_buffer: The pointer of sent data buffer
 * @param len: Max data buffer length
 * @param bytes_written: Byte number that actually be sent, can be NULL if not needed
 * @param timeout_ms: Max block time
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms);

typedef struct {
    bsp_sys_get_sleep_mode get_sleep_mode;
    bsp_sys_get_bottom_id get_bottom_id;

    bsp_bottom_set_radar_enable set_radar_enable;
    bsp_bottom_get_radar_status get_radar_status;
    bsp_bottom_get_humiture get_humiture;
} bsp_bottom_property_t;

typedef struct {
    const char *name;
    const board_res_desc_t *board_desc; /*!< Get board description */
} boards_info_t;

/**
 * @brief Special config for board
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
 * @brief Get the sensor operation function
 *
 * @return pointer of bsp_bottom_property_t
 */
bsp_bottom_property_t *bsp_board_get_sensor_handle(void);

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
 * @param btn: A button handle to register
 * @param event: Button event
 * @param callback: Callback function.
 * @param user_data: user data
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG   Arguments is invalid.
 */
esp_err_t bsp_btn_register_callback(bsp_button_t btn, button_event_t event, button_cb_t callback, void *user_data);

/**
 * @brief Unregister the button event callback function.
 *
 * @param btn: A button handle to unregister
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG   Arguments is invalid.
 */
esp_err_t bsp_btn_rm_all_callback(bsp_button_t btn);

/**
 * @brief Unregister the button event callback function.
 *
 * @param btn: A button handle to unregister
 * @param event: Unregister event
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG   Arguments is invalid.
 */
esp_err_t bsp_btn_rm_event_callback(bsp_button_t btn, size_t event);

#ifdef __cplusplus
}
#endif
