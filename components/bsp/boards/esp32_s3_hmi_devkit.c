/**
 * @file esp32_s3_hmi_devkit.c
 * @brief 
 * @version 0.1
 * @date 2021-07-05
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

#include <stdbool.h>
#include "bsp_board.h"
#include "bsp_i2c.h"
#include "bsp_storage.h"
#include "bsp_tp.h"
#include "driver/gpio.h"
#include "esp_log.h"

#if (!CONFIG_IDF_TARGET_ESP32S3)
    /* Check for IDF target. This dev board uses ESP32-S3 */
    #error "Please set idf target to `ESP32-S3`"
#endif

static const char *TAG = "board";

esp_err_t bsp_board_init(void)
{
    /* Open all periphral power */
    ESP_ERROR_CHECK(bsp_board_power_ctrl(POWER_MODULE_ALL, true));

    /* Wait for power stable */
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Init I2C bus */
    ESP_ERROR_CHECK(bsp_i2c_init(I2C_NUM_0, 400 * 1000));

    /* Init input device */
    ESP_ERROR_CHECK(bsp_tp_init());

    /* Init SDMMC card */
    ESP_ERROR_CHECK(bsp_storage_init(BSP_STORAGE_SD_CARD));

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
    case POWER_MODULE_AUDIO:
        // gpio_set_level(GPIO_NUM_46, on ? (GPIO_PWR_ON_LEVEL) : (!GPIO_PWR_ON_LEVEL));
        break;
    case POWER_MODULE_ALL:
        gpio_set_level(GPIO_PWR_CTRL, on ? (GPIO_PWR_ON_LEVEL) : (!GPIO_PWR_ON_LEVEL));
        break;
    default:
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}
