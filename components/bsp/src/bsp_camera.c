/**
 * @file bsp_camera.c
 * @brief 
 * @version 0.1
 * @date 2021-07-07
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

#include "bsp_board.h"
#include "esp_camera.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "camera";

static camera_config_t camera_config = {
    .pin_pwdn = GPIO_CAM_PWDN,
    .pin_reset = GPIO_CAM_RST,
    .pin_xclk = GPIO_CAM_XCLK,
    .pin_sscb_sda = GPIO_CAM_SDA,
    .pin_sscb_scl = GPIO_CAM_SCL,

    .pin_d7 = GPIO_CAM_Y9,
    .pin_d6 = GPIO_CAM_Y8,
    .pin_d5 = GPIO_CAM_Y7,
    .pin_d4 = GPIO_CAM_Y6,
    .pin_d3 = GPIO_CAM_Y5,
    .pin_d2 = GPIO_CAM_Y4,
    .pin_d1 = GPIO_CAM_Y3,
    .pin_d0 = GPIO_CAM_Y2,
    .pin_vsync = GPIO_CAM_VSYNC,
    .pin_href = GPIO_CAM_HSYNC,
    .pin_pclk = GPIO_CAM_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_RGB565, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QVGA,    //QQVGA-UXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 12, //0-63 lower number means higher quality
    .fb_count = 1       //if more than one, i2s runs in continuous mode. Use only with JPEG
};


esp_err_t bsp_camera_init(void)
{
    esp_err_t ret_val = ESP_OK;
    ret_val |=  esp_camera_init(&camera_config);
    ret_val |= esp_camera_sensor_get()->set_vflip(esp_camera_sensor_get(), true);

    if (ESP_OK != ret_val) {
        ESP_LOGE(TAG, "Failed initialize camera : 0x%02X", ret_val);
    }

    return ret_val;
}

