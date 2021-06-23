/**
 * @file bsp_board_esp32_c3_devkit.h
 * @version 0.1
 * @date 2021-06-25
 * 
 * @copyright Copyright 2021 Espressif Systems (Shanghai) Co. Ltd.
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */
#pragma once

#include "driver/gpio.h"

/**
 * @brief ESP32-S2-HMI-DevKit LCD GPIO defination and config
 * 
 */
#define FUNC_LCD_EN     (1)
#define LCD_BUS_WIDTH   (1)
#define LCD_WIDTH       (320)
#define LCD_HEIGHT      (240)
#define LCD_HOST        (SPI2_HOST)
#define LCD_DISP_IC_ST  (1)
#define LCD_FREQ        (20 * 1000 * 1000)
#define LCD_HOST        (SPI2_HOST)

#define LCD_SWAP_XY     (true)
#define LCD_MIRROR_X    (false)
#define LCD_MIRROR_Y    (true)
#define LCD_COLOR_INV   (true)

#define GPIO_LCD_BL     (GPIO_NUM_8)
#define GPIO_LCD_BL_ON  (1)
#define GPIO_LCD_CS     (GPIO_NUM_5)
#define GPIO_LCD_RST    (GPIO_NUM_9)
#define GPIO_LCD_DC     (GPIO_NUM_4)
#define GPIO_LCD_WR     (GPIO_NUM_6)
#define GPIO_LCD_CLK    (GPIO_LCD_WR)
#define GPIO_LCD_DIN    (GPIO_NUM_7)
#define GPIO_LCD_DOUT   (GPIO_NUM_2)

/**
 * @brief ESP32-S2-HMI-DevKit I2C GPIO defineation
 * 
 */
#define FUNC_I2C_EN     (1)
#define GPIO_I2C_SCL    (GPIO_NUM_0)
#define GPIO_I2C_SDA    (GPIO_NUM_1)

/**
 * @brief ESP32-S2-HMI-DevKit SDMMC GPIO defination
 * 
 */
#define FUNC_SDMMC_EN   (0)

/**
 * @brief ESP32-S2-HMI-DevKit SDSPI GPIO definationv
 * 
 */
#define FUNC_SDCARD_EN      (1)
#define FUNC_SDSPI_EN       (1)
#define GPIO_SDSPI_CS       (GPIO_NUM_4)
#define GPIO_SDSPI_SCLK     (GPIO_NUM_6)
#define GPIO_SDSPI_MISO     (GPIO_NUM_2)
#define GPIO_SDSPI_MOSI     (GPIO_NUM_7)

/**
 * @brief ESP32-S2-HMI-DevKit SPI GPIO defination
 * 
 */
#define FUNC_SPI_EN         (1)
#define GPIO_SPI_CS         (GPIO_NUM_4)
#define GPIO_SPI_MISO       (GPIO_NUM_2)
#define GPIO_SPI_MOSI       (GPIO_NUM_7)
#define GPIO_SPI_SCLK       (GPIO_NUM_6)

/**
 * @brief ESP32-S2-HMI-DevKit RMT GPIO defination
 * 
 */
#define FUNC_RMT_EN         (1)
#define GPIO_RMT_IR         (GPIO_NUM_NC)
#define GPIO_RMT_LED        (GPIO_NUM_10)

/**
 * @brief ESP32-S2-HMI-DevKit I2S GPIO defination
 * 
 */
#define FUNC_I2S_EN         (0)
#define GPIO_I2S_LRCK       (GPIO_NUM_NC)
#define GPIO_I2S_MCLK       (GPIO_NUM_NC)
#define GPIO_I2S_SCLK       (GPIO_NUM_NC)
#define GPIO_I2S_SDIN       (GPIO_NUM_NC)
#define GPIO_I2S_DOUT       (GPIO_NUM_NC)

/**
 * @brief ESP32-S3-HMI-DevKit power control IO
 * 
 */
#define FUNC_PWR_CTRL       (0)

/**
 * @brief ESP32-S3-HMI-DevKit camrea control IO
 * 
 */
#define FUNC_CAM_EN         (0)
