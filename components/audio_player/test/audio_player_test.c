// Copyright 2020 Espressif Systems (Shanghai) Co. Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "esp_log.h"
#include "unity.h"
#include "audio_player.h"
#include "driver/gpio.h"
#include "test_utils.h"

static const char *TAG = "AUDIO PLAYER TEST";

static esp_err_t audio_mute_function(AUDIO_PLAYER_MUTE_SETTING setting) {
    ESP_LOGI(TAG, "mute setting %d", setting);
    return ESP_OK;
}

TEST_CASE("audio player can be newed and deleted", "[audio player]")
{
    esp_err_t ret = audio_player_new(I2S_NUM_0, audio_mute_function);
    TEST_ASSERT_TRUE(ret == ESP_OK);

    ret = audio_player_delete();
    TEST_ASSERT_TRUE(ret == ESP_OK);

    audio_player_state_t state = audio_player_get_state();
    TEST_ASSERT_TRUE(state == AUDIO_PLAYER_STATE_SHUTDOWN);
}

#define I2S_CONFIG_DEFAULT() { \
    .mode                   = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX, \
    .sample_rate            = sample_rate, \
    .bits_per_sample        = I2S_BITS_PER_SAMPLE_16BIT, \
    .channel_format         = I2S_CHANNEL_FMT_RIGHT_LEFT, \
    .communication_format   = I2S_COMM_FORMAT_STAND_I2S, \
    .intr_alloc_flags       = ESP_INTR_FLAG_LEVEL1, \
    .dma_buf_count          = 6, \
    .dma_buf_len            = 160, \
    .use_apll               = false, \
    .tx_desc_auto_clear     = true, \
    .fixed_mclk             = 0, \
    .mclk_multiple          = I2S_MCLK_MULTIPLE_DEFAULT, \
    .bits_per_chan          = I2S_BITS_PER_CHAN_16BIT, \
}

static esp_err_t i2s_init(i2s_port_t i2s_num, uint32_t sample_rate)
{
    esp_err_t ret_val = ESP_OK;
    i2s_config_t i2s_config = I2S_CONFIG_DEFAULT();

    // esp-box pin values, we don't care about audio output for the test however, only
    // that the i2s hardware is operating as expected, clocking data out and in
    i2s_pin_config_t pin_config = {
        .bck_io_num = GPIO_NUM_17,
        .ws_io_num = GPIO_NUM_47,
        .data_out_num = GPIO_NUM_15,
        .data_in_num = GPIO_NUM_16,
        .mck_io_num = GPIO_NUM_2,
    };

    ret_val |= i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
    ret_val |= i2s_set_pin(i2s_num, &pin_config);
    ret_val |= i2s_zero_dma_buffer(i2s_num);

    return ret_val;
}

static audio_player_callback_event_t expected_event;
static QueueHandle_t event_queue;

static void audio_player_callback(audio_player_cb_ctx_t *ctx)
{
    TEST_ASSERT_TRUE(ctx->audio_event == expected_event);

    // wake up the test so it can continue to the next step
    TEST_ASSERT_TRUE(xQueueSend(event_queue, &(ctx->audio_event), 0) == pdPASS);
}

TEST_CASE("audio player states and callbacks are correct", "[audio player]")
{
    audio_player_callback_event_t event;

    esp_err_t ret = i2s_init(I2S_NUM_0, 44100);
    TEST_ASSERT_TRUE(ret == ESP_OK);

    ret = audio_player_new(I2S_NUM_0, audio_mute_function);
    TEST_ASSERT_TRUE(ret == ESP_OK);

    event_queue = xQueueCreate(1, sizeof(audio_player_callback_event_t));
    TEST_ASSERT_NOT_NULL(event_queue);

    ret = audio_player_callback_register(audio_player_callback, NULL);
    TEST_ASSERT_TRUE(ret == ESP_OK);

    audio_player_state_t state = audio_player_get_state();
    TEST_ASSERT_TRUE(state == AUDIO_PLAYER_STATE_IDLE);

    extern const char mp3_start[] asm("_binary_gs_16b_1c_44100hz_mp3_start");
    extern const char mp3_end[]   asm("_binary_gs_16b_1c_44100hz_mp3_end");

    // -1 due to the size being 1 byte too large, I think because end is the byte
    // immediately after the last byte in the memory but I'm not sure - cmm 2022-08-20
    size_t mp3_size = (mp3_end - mp3_start) - 1;
    ESP_LOGI(TAG, "mp3_size %zu bytes", mp3_size);

    FILE *fp = fmemopen((void*)mp3_start, mp3_size, "rb");
    TEST_ASSERT_TRUE(fp != NULL);



    ///////////////
    expected_event = AUDIO_PLAYER_CALLBACK_EVENT_PLAYING;
    ret = audio_player_play(fp);
    TEST_ASSERT_TRUE(ret == ESP_OK);

    // wait for playing event to arrive
    TEST_ASSERT_TRUE(xQueueReceive(event_queue, &event, pdMS_TO_TICKS(100)) == pdPASS);

    // confirm state is playing
    state = audio_player_get_state();
    TEST_ASSERT_TRUE(state == AUDIO_PLAYER_STATE_PLAYING);



    ///////////////
    expected_event = AUDIO_PLAYER_CALLBACK_EVENT_PAUSE;
    ret = audio_player_pause();
    TEST_ASSERT_TRUE(ret == ESP_OK);

    // wait for paused event to arrive
    TEST_ASSERT_TRUE(xQueueReceive(event_queue, &event, pdMS_TO_TICKS(100)) == pdPASS);

    state = audio_player_get_state();
    TEST_ASSERT_TRUE(state == AUDIO_PLAYER_STATE_PAUSE);



    ////////////////
    expected_event = AUDIO_PLAYER_CALLBACK_EVENT_PLAYING;
    ret = audio_player_resume();
    TEST_ASSERT_TRUE(ret == ESP_OK);

    // wait for paused event to arrive
    TEST_ASSERT_TRUE(xQueueReceive(event_queue, &event, pdMS_TO_TICKS(100)) == pdPASS);



    ///////////////
    expected_event = AUDIO_PLAYER_CALLBACK_EVENT_IDLE;

    // the track is 16 seconds long so lets wait a bit here
    int sleep_seconds = 16;
    ESP_LOGI(TAG, "sleeping for %d seconds", sleep_seconds);
    vTaskDelay(pdMS_TO_TICKS(sleep_seconds * 1000));

    // wait for idle event to arrive
    TEST_ASSERT_TRUE(xQueueReceive(event_queue, &event, pdMS_TO_TICKS(100)) == pdPASS);

    state = audio_player_get_state();
    TEST_ASSERT_TRUE(state == AUDIO_PLAYER_STATE_IDLE);



    ///////////////
    expected_event = AUDIO_PLAYER_CALLBACK_EVENT_SHUTDOWN;
    ret = audio_player_delete();
    TEST_ASSERT_TRUE(ret == ESP_OK);

    // wait for idle event to arrive
    TEST_ASSERT_TRUE(xQueueReceive(event_queue, &event, pdMS_TO_TICKS(100)) == pdPASS);

    state = audio_player_get_state();
    TEST_ASSERT_TRUE(state == AUDIO_PLAYER_STATE_SHUTDOWN);

    vQueueDelete(event_queue);

    i2s_driver_uninstall(I2S_NUM_0);

    ESP_LOGI(TAG, "NOTE: a memory leak will be reported the first time this test runs.\n");
    ESP_LOGI(TAG, "esp-idf v4.4.1 and v4.4.2 both leak memory between i2s_driver_install() and i2s_driver_uninstall()\n");
}
