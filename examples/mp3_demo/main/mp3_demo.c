/**
 * @file
 * @version 0.1
 * @date 2021-11-11
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

#include "audio_player.h"
#include "bsp_board.h"
#include "bsp_lcd.h"
#include "bsp_storage.h"
#include "bsp_i2s.h"
#include "bsp_codec.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lv_port.h"
#include "lvgl.h"
#include "ui_audio.h"
#include "file_iterator.h"

static const char *TAG = "mp3_demo";

static esp_err_t audio_mute_function(AUDIO_PLAYER_MUTE_SETTING setting) {
    // Volume saved when muting and restored when unmuting. Restoring volume is necessary
    // as es8311_set_voice_mute(true) results in voice volume (REG32) being set to zero.
    static uint8_t last_volume;

    uint8_t volume = get_sys_volume();
    if(volume != 0) {
        last_volume = volume;
    }
    
    ESP_RETURN_ON_ERROR(bsp_codec_set_mute(setting == AUDIO_PLAYER_MUTE ? true : false), TAG, "set voice mute");

    // restore the voice volume upon unmuting
    if(setting == AUDIO_PLAYER_UNMUTE) {
        bsp_codec_set_voice_volume(last_volume);
    }
    ESP_LOGI(TAG, "mute setting %d, volume:%d", setting, last_volume);

    return ESP_OK;
}

void app_main(void)
{
    ESP_ERROR_CHECK(bsp_board_init());
    ESP_ERROR_CHECK(bsp_board_power_ctrl(POWER_MODULE_AUDIO, true));

    ESP_ERROR_CHECK(bsp_spiffs_init("storage", "/spiffs", 2));
    ESP_ERROR_CHECK(lv_port_init());
    bsp_lcd_set_backlight(true);

    file_iterator_instance_t *file_iterator = file_iterator_new("/spiffs");
    assert(file_iterator != NULL);

    /**
     * @brief Initialize I2S and audio codec
     *
     * @note `MP3GetLastFrameInfo` is used to fill the `MP3FrameInfo`, which includes `samprate`,
     *       and the sampling rate is updated during playback using this value.
     */
    esp_err_t ret = bsp_codec_init(AUDIO_HAL_44K_SAMPLES);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "bsp_codec_init failed with %d", ret);
        return;
    }

    bsp_i2s_init(I2S_NUM_0, 44100);
    // TODO: original code isn't checking return value here, this is failing with a value of 0x103 which is
    // who the heck knows what error
//    ESP_RETURN_ON_ERROR(ret, TAG, "bsp_i2s_init");

    audio_player_config_t config = { .port = I2S_NUM_0,
                                     .mute_fn = audio_mute_function,
                                     .priority = 1 };

    ESP_ERROR_CHECK(audio_player_new(config));

    ui_audio_start(file_iterator);

    do {
        lv_task_handler();
    } while (vTaskDelay(1), true);
}
