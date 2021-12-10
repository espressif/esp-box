/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include "esp_log.h"
#include "bsp_board.h"
#include "bsp_i2c.h"
#include "bsp_i2s.h"
#include "bsp_codec.h"
#include "esp32_s3_box.h"
#include "esp32_s3_box_lite.h"

static const char *TAG = "bsp boards";

static const boards_info_t g_boards_info[] = {
    {BOARD_S3_BOX,      "S3_BOX",      bsp_board_s3_box_init,      bsp_board_s3_box_power_ctrl, bsp_board_s3_box_get_res_desc},
    {BOARD_S3_BOX_LITE, "S3_BOX_LITE", bsp_board_s3_box_lite_init, bsp_board_s3_box_lite_power_ctrl, bsp_board_s3_box_lite_get_res_desc},
};
static boards_info_t *g_board = NULL;

static esp_err_t bsp_board_detect()
{
    esp_err_t ret = ESP_OK;
    for (size_t i = 0; i < sizeof(g_boards_info) / sizeof(boards_info_t); i++) {
        const board_res_desc_t *brd = g_boards_info[i].board_get_res_desc();
        /*!< Initialize I2C bus, used for TP ,audio codec and IMU */
        bsp_i2c_init(I2C_NUM_0, 400 * 1000, brd->GPIO_I2C_SCL, brd->GPIO_I2C_SDA);
        uint32_t codecs = 0;
        bsp_codec_detect(&codecs);

        if ((CODEC_DEV_ES7210 | CODEC_DEV_ES8311) == codecs) {
            g_board = (boards_info_t *)&g_boards_info[BOARD_S3_BOX];
        } else if ((CODEC_DEV_ES7243 | CODEC_DEV_ES8156) == codecs) {
            g_board = (boards_info_t *)&g_boards_info[BOARD_S3_BOX_LITE];
        } else {
            ESP_LOGE(TAG, "Can't Detect a correct board");
            bsp_i2c_deinit();
            ret = ESP_ERR_NOT_FOUND;
        }
        if (g_board) {
            ESP_LOGI(TAG, "Detected board: [%s]", g_board->name);
            break;
        }
    }
    return ret;
}

const boards_info_t *bsp_board_get_info(void)
{
    return g_board;
}

const board_res_desc_t *bsp_board_get_description(void)
{
    return g_board->board_get_res_desc();
}

esp_err_t bsp_board_init(void)
{
    if (ESP_OK == bsp_board_detect()) {
        return g_board->board_init();
    }
    return ESP_FAIL;
}

esp_err_t bsp_board_power_ctrl(power_module_t module, bool on)
{
    if (g_board) {
        return g_board->board_power_ctrl(module, on);
    }
    return ESP_FAIL;
}
