/**
 * @file esp32_s3_box.c
 * @brief 
 * @version 0.1
 * @date 2021-07-28
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

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "bsp_board.h"
#include "bsp_i2c.h"

#define GPIO_MUTE_NUM   GPIO_NUM_1
#define GPIO_MUTE_LEVEL 1

static const char *TAG = "board";

__attribute__((weak)) void mute_btn_handler(void *arg)
{
    if (GPIO_MUTE_LEVEL == gpio_get_level(GPIO_MUTE_NUM)) {
        esp_rom_printf(DRAM_STR("Mute Off"));
    } else {
        esp_rom_printf(DRAM_STR("Mute On"));
    }
}

esp_err_t bsp_board_init(void)
{
    /*!< Mute_Button on ESP32-S3-Box */
    gpio_config_t io_conf_key;
    io_conf_key.intr_type = GPIO_INTR_ANYEDGE;
    io_conf_key.mode = GPIO_MODE_INPUT;
    io_conf_key.pin_bit_mask = 1ULL << GPIO_MUTE_NUM;
    io_conf_key.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf_key.pull_up_en = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf_key));

    /* Install GPIO ISR service to enable GPIO ISR callback */
    gpio_install_isr_service(0);
    ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_MUTE_NUM, mute_btn_handler, NULL));

    /*!< Initialize I2C bus, used for TP ,audio codec and IMU */
    bsp_i2c_init(I2C_NUM_0, 400 * 1000);

    return ESP_OK;
}

esp_err_t bsp_board_power_ctrl(power_module_t module, bool on)
{
    /* Config power control IO */
    static esp_err_t bsp_io_config_state = ESP_FAIL;
    if (ESP_OK != bsp_io_config_state) {
        gpio_config_t io_conf;
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = 1ULL << GPIO_PWR_CTRL;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        bsp_io_config_state = gpio_config(&io_conf);
    }

    /* Checko IO config result */
    if (ESP_OK != bsp_io_config_state) {
        ESP_LOGE(TAG, "Failed initialize power control IO");
        return bsp_io_config_state;
    }

    /* Control independent power domain */
    switch (module) {
    case POWER_MODULE_LCD:
        gpio_set_level(GPIO_LCD_BL, on ? (GPIO_LCD_BL_ON) : (!GPIO_LCD_BL_ON));
        break;
    case POWER_MODULE_AUDIO:
    case POWER_MODULE_ALL:
        gpio_set_level(GPIO_PWR_CTRL, on ? (GPIO_PWR_ON_LEVEL) : (!GPIO_PWR_ON_LEVEL));
        break;
    default:
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}
