/**
 * @file esp32_s2_hmi_devkit.c
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
#include "esp_log.h"
#include "tca9554.h"

#if (!CONFIG_IDF_TARGET_ESP32S2)
    /* Check for IDF target. This dev board uses ESP32-S2 */
    #error "Please set idf target to `ESP32-S2`"
#endif

#define PWR_PA_BIT          (1 << 3)
#define PWR_PA_ON_LEVEL     (1)
#define PWR_ALL_BIT         (1 << 4)
#define PWR_ALL_ON_LEVEL    (0)
#define PWR_LCD_BL          (1 << 7)
#define PWR_LCD_ON_LEVEL    (1)

static const char *TAG = "board";

esp_err_t bsp_board_init(void)
{
    /* Init I2C bus */
    ESP_ERROR_CHECK(bsp_i2c_init(I2C_NUM_0, 400 * 1000));

    /* Open all periphral power */
    ESP_ERROR_CHECK(bsp_board_power_ctrl(POWER_MODULE_ALL, true));
    ESP_ERROR_CHECK(bsp_board_power_ctrl(POWER_MODULE_LCD, true));

    return ESP_OK;
}

esp_err_t bsp_board_power_ctrl(power_module_t module, bool on)
{
    /* Config power control IO */
    static esp_err_t bsp_io_config_state = ESP_FAIL;
    if (ESP_OK != bsp_io_config_state) {
        bsp_io_config_state = ESP_OK;
        bsp_io_config_state |= tca9554_init();
        bsp_io_config_state |= tca9554_set_configuration(0b00000101);
        bsp_io_config_state |= tca9554_write_output_pins(0b11000111);
    }

    /* Checko IO config result */
    if (ESP_OK != bsp_io_config_state) {
        ESP_LOGE(TAG, "Failed initialize power control IO");
        return bsp_io_config_state;
    }

    /* Get output level of IO expander */
    uint8_t io_level;
    esp_err_t ret_val = tca9554_read_output_pins(&io_level);
    if (ESP_OK != ret_val) {
        ESP_LOGE(TAG, "Fail read IO expander's output level");
        return ret_val;
    } else {
        ESP_LOGD(TAG, "Output IO level read: %02X", io_level);
    }

    /* Control independent power domain */
    switch (module) {
    case POWER_MODULE_LCD:
        if (on ^ PWR_LCD_ON_LEVEL) {
            io_level &= ~PWR_LCD_BL;
        } else {
            io_level |= PWR_LCD_BL;
        }
        break;
    case POWER_MODULE_AUDIO:
        if (on ^ PWR_PA_ON_LEVEL) {
            io_level &= ~PWR_PA_BIT;
        } else {
            io_level |= PWR_PA_BIT;
        }
        break;
    case POWER_MODULE_ALL:
        if (on ^ PWR_ALL_ON_LEVEL) {
            io_level &= ~PWR_ALL_BIT;
        } else {
            io_level |= PWR_ALL_BIT;
        }
        break;
    default:
        return ESP_ERR_INVALID_ARG;
    }

    /* Write expected output level to IO expander */
    ESP_LOGD(TAG, "Output IO level write : %02X", io_level);
    ret_val = tca9554_write_output_pins(io_level);

    return ret_val;
}
