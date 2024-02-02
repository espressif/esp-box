/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <inttypes.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "esp_private/usb_phy.h"
#include "esp_log.h"
#include "bsp/esp-bsp.h"
#include "bsp_board.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "usb_headset.h"
#include "fft_convert.h"

const static char *TAG = "usb_headset";

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTOTYPES
//--------------------------------------------------------------------+
// Volume control range
enum {
    VOLUME_CTRL_0_DB = 0,
    VOLUME_CTRL_10_DB = 2560,
    VOLUME_CTRL_20_DB = 5120,
    VOLUME_CTRL_30_DB = 7680,
    VOLUME_CTRL_40_DB = 10240,
    VOLUME_CTRL_50_DB = 12800,
    VOLUME_CTRL_60_DB = 15360,
    VOLUME_CTRL_70_DB = 17920,
    VOLUME_CTRL_80_DB = 20480,
    VOLUME_CTRL_90_DB = 23040,
    VOLUME_CTRL_100_DB = 25600,
    VOLUME_CTRL_SILENCE = 0x8000,
};
// List of supported sample rates
// we just support one sample rate, due to the limitation of the codec
const uint32_t sample_rates[] = {DEFAULT_SAMPLE_RATE};
static uint32_t s_sample_rate = DEFAULT_SAMPLE_RATE;
#define N_SAMPLE_RATES  TU_ARRAY_SIZE(sample_rates)

// Audio controls, current states
static int8_t spk_mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];       // +1 for master channel 0
static int16_t spk_volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];    // +1 for master channel 0
static audio_control_range_2_n_t(1) range_vol = {
    .wNumSubRanges = tu_htole16(1),
    .subrange[0] = { .bMin = tu_htole16(-VOLUME_CTRL_50_DB), tu_htole16(VOLUME_CTRL_0_DB), tu_htole16(256) }
};

// Buffer for microphone data
static int16_t s_mic_buf1[CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ / 2] = {0};
static int16_t s_mic_buf2[CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ / 2] = {0};
static int16_t *s_mic_write_buf = s_mic_buf1;
static int16_t *s_mic_read_buf = s_mic_buf2;
volatile static size_t s_mic_read_buf_len = 0;
static TaskHandle_t mic_task_handle;
// Buffer for speaker data
static int16_t s_spk_buf[CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ / 2] = {0};
volatile static size_t s_spk_buf_len = 0;
static TaskHandle_t spk_task_handle;
// Speaker and microphone status
volatile static bool s_spk_active;
volatile static bool s_mic_active;
// Resolution per format, Note: due to the limitation of the codec, we currently just support one resolution
const uint8_t spk_resolutions_per_format[CFG_TUD_AUDIO_FUNC_1_N_FORMATS] = {CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_RX
                                                                           };
const uint8_t mic_resolutions_per_format[CFG_TUD_AUDIO_FUNC_1_N_FORMATS] = {CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_TX
                                                                           };
// Current resolution, update on format change
static uint8_t s_spk_resolution = spk_resolutions_per_format[0];
static uint8_t s_mic_resolution = mic_resolutions_per_format[0];
static size_t s_spk_bytes_ms = 0;
static size_t s_mic_bytes_ms = 0;
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
#define UAC_ENTER_CRITICAL()    portENTER_CRITICAL(&s_mux)
#define UAC_EXIT_CRITICAL()     portEXIT_CRITICAL(&s_mux)

// for debug purpose
#include "usb_headset_debug.h"

static void usb_phy_init(void)
{
    // Configure USB PHY
    usb_phy_handle_t phy_hdl;
    usb_phy_config_t phy_conf = {
        .controller = USB_PHY_CTRL_OTG,
        .otg_mode = USB_OTG_MODE_DEVICE,
    };
    phy_conf.target = USB_PHY_TARGET_INT;
    usb_new_phy(&phy_conf, &phy_hdl);
}

static void usb_task(void *pvParam)
{
    (void) pvParam;
    usb_phy_init();
    if (tud_init(BOARD_TUD_RHPORT) == false) {
        ESP_LOGE(TAG, "Failed to initialize TinyUSB");
        goto error;
    }

    do {
        // TinyUSB Core task
        tud_task();
    } while (true);

error:
    vTaskDelete(NULL);
}

static void usb_headset_spk(void *pvParam)
{
    while (1) {
        SYSVIEW_SPK_WAIT_EVENT_START();
        if (s_spk_active == false) {
            ulTaskNotifyTake(pdFAIL, portMAX_DELAY);
            continue;
        }
        // clear the notification
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (s_spk_buf_len == 0) {
            continue;
        }
        // playback the data from the ring buffer chunk by chunk
        SYSVIEW_SPK_WAIT_EVENT_END();
        SYSVIEW_SPK_SEND_EVENT_START();
        size_t bytes_written = 0;
        bsp_i2s_write(s_spk_buf, s_spk_buf_len, &bytes_written, 0);
        for (int i = 0; i < bytes_written / 2; i ++) {
            rb_write(s_spk_buf + i, 2);
        }
        s_spk_buf_len = 0;
        SYSVIEW_SPK_SEND_EVENT_END();
    }
}

static void usb_headset_mic(void *pvParam)
{
    while (1) {
        if (s_mic_active == false) {
            ulTaskNotifyTake(pdFAIL, portMAX_DELAY);
            continue;
        }
        // clear the notification
        ulTaskNotifyTake(pdTRUE, 0);
        // read data from the microphone chunk by chunk
        SYSVIEW_MIC_READ_EVENT_START();
        size_t bytes_read = 0;
        size_t bytes_require = s_mic_bytes_ms * (CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_MS - 1);
        esp_err_t ret = bsp_i2s_read(s_mic_write_buf, bytes_require, &bytes_read, 0);
        if (ret != ESP_OK) {
            ESP_LOGD(TAG, "Failed to read data from I2S, ret = %d", ret);
            SYSVIEW_MIC_READ_EVENT_END();
            continue;
        }
        // swap the buffer
        int16_t *tmp_buf = s_mic_read_buf;
        UAC_ENTER_CRITICAL();
        s_mic_read_buf = s_mic_write_buf;
        s_mic_read_buf_len = bytes_read;
        s_mic_write_buf = tmp_buf;
        UAC_EXIT_CRITICAL();
        SYSVIEW_MIC_READ_EVENT_END();
    }
}

esp_err_t usb_headset_init(void)
{
    s_spk_bytes_ms = s_sample_rate / 1000 * s_spk_resolution * DEFAULT_PLAYER_CHANNEL / 8;
    s_mic_bytes_ms = s_sample_rate / 1000 * s_mic_resolution * DEFAULT_RECORDER_CHANNEL / 8;
    // we give the higher priority to playback task, to avoid the data pending in the ring buffer
    BaseType_t ret_val = xTaskCreate(usb_headset_spk, "usb_headset_spk", 4 * 1024, NULL, 8, &spk_task_handle);
    if (ret_val != pdPASS) {
        ESP_LOGE(TAG, "Failed to create usb_headset_spk task");
        return ESP_FAIL;
    }
    // we give the lower priority to record task, to avoid the data pending in the ring buffer
    ret_val = xTaskCreate(usb_headset_mic, "usb_headset_mic", 4 * 1024, NULL, 8, &mic_task_handle);
    if (ret_val != pdPASS) {
        ESP_LOGE(TAG, "Failed to create usb_headset_mic task");
        return ESP_FAIL;
    }
    ret_val = xTaskCreatePinnedToCore(usb_task, "usb_task", 4 * 1024, NULL, 10, NULL, 1);
    if (ret_val != pdPASS) {
        ESP_LOGE(TAG, "Failed to create usb_task");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "TinyUSB initialized");
    return ESP_OK;
}

// Invoked when device is mounted
void tud_mount_cb(void)
{
    s_spk_active = false;
    s_mic_active = false;
    ESP_LOGI(TAG, "USB mounted");
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
    ESP_LOGI(TAG, "USB unmounted");
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
    s_spk_active = false;
    s_mic_active = false;
    (void)remote_wakeup_en;
    ESP_LOGI(TAG, "USB suspended");
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
    ESP_LOGI(TAG, "USB resumed");
}

// Helper for clock get requests
static bool tud_audio_clock_get_request(uint8_t rhport, audio_control_request_t const *request)
{
    TU_ASSERT(request->bEntityID == UAC2_ENTITY_CLOCK);

    if (request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ) {
        if (request->bRequest == AUDIO_CS_REQ_CUR) {
            TU_LOG1("Clock get current freq %lu\r\n", s_sample_rate);

            audio_control_cur_4_t curf = { (int32_t) tu_htole32(s_sample_rate) };
            return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &curf, sizeof(curf));
        } else if (request->bRequest == AUDIO_CS_REQ_RANGE) {
            audio_control_range_4_n_t(N_SAMPLE_RATES) rangef = {
                .wNumSubRanges = tu_htole16(N_SAMPLE_RATES)
            };
            TU_LOG1("Clock get %d freq ranges\r\n", N_SAMPLE_RATES);
            for (uint8_t i = 0; i < N_SAMPLE_RATES; i++) {
                rangef.subrange[i].bMin = (int32_t) sample_rates[i];
                rangef.subrange[i].bMax = (int32_t) sample_rates[i];
                rangef.subrange[i].bRes = 0;
                TU_LOG1("Range %d (%d, %d, %d)\r\n", i, (int)rangef.subrange[i].bMin, (int)rangef.subrange[i].bMax, (int)rangef.subrange[i].bRes);
            }

            return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &rangef, sizeof(rangef));
        }
    } else if (request->bControlSelector == AUDIO_CS_CTRL_CLK_VALID &&
               request->bRequest == AUDIO_CS_REQ_CUR) {
        audio_control_cur_1_t cur_valid = { .bCur = 1 };
        TU_LOG1("Clock get is valid %u\r\n", cur_valid.bCur);
        return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &cur_valid, sizeof(cur_valid));
    }
    TU_LOG1("Clock get request not supported, entity = %u, selector = %u, request = %u\r\n",
            request->bEntityID, request->bControlSelector, request->bRequest);
    return false;
}

// Helper for clock set requests
static bool tud_audio_clock_set_request(uint8_t rhport, audio_control_request_t const *request, uint8_t const *buf)
{
    (void)rhport;

    TU_ASSERT(request->bEntityID == UAC2_ENTITY_CLOCK);
    TU_VERIFY(request->bRequest == AUDIO_CS_REQ_CUR);

    if (request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ) {
        TU_VERIFY(request->wLength == sizeof(audio_control_cur_4_t));

        uint32_t target_sample_rate = (uint32_t) ((audio_control_cur_4_t const *)buf)->bCur;
        TU_LOG1("Clock set current freq: %ld\r\n", target_sample_rate);
        if (target_sample_rate != s_sample_rate) {
            bool target_exists = false;
            for (int i = 0; i < N_SAMPLE_RATES; i++) {
                if (target_sample_rate == sample_rates[i]) {
                    target_exists = true;
                    break;
                }
            }

            if (target_exists == false) {
                TU_LOG1("Unsupported sample rate %ld", target_sample_rate);
                return false;
            }
            // Currently the bsp_codec_set_fs() can not support different sample rate for mic and speaker
            // the dynamic sample rate change is not supported
            // TODO: bsp_codec_set_fs(target_sample_rate, s_spk_resolution, DEFAULT_PLAYER_CHANNEL);
            s_sample_rate = target_sample_rate;
            s_spk_bytes_ms = s_sample_rate / 1000 * s_spk_resolution * DEFAULT_PLAYER_CHANNEL / 8;
            s_mic_bytes_ms = s_sample_rate / 1000 * s_mic_resolution * DEFAULT_RECORDER_CHANNEL / 8;
            TU_LOG1("Mic/Speaker frequency %" PRIu32 ", resolution %d, ch %d", target_sample_rate, s_spk_resolution, DEFAULT_PLAYER_CHANNEL);
            return true;
        }
        return true;
    } else {
        TU_LOG1("Clock set request not supported, entity = %u, selector = %u, request = %u\r\n",
                request->bEntityID, request->bControlSelector, request->bRequest);
        return false;
    }
}

// Helper for feature unit get requests
static bool tud_audio_feature_unit_get_request(uint8_t rhport, audio_control_request_t const *request)
{
    TU_ASSERT(request->bEntityID == UAC2_ENTITY_SPK_FEATURE_UNIT);

    if (request->bControlSelector == AUDIO_FU_CTRL_MUTE && request->bRequest == AUDIO_CS_REQ_CUR) {
        audio_control_cur_1_t mute1 = { .bCur = spk_mute[request->bChannelNumber] };
        TU_LOG1("Get channel %u spk_mute %d\r\n", request->bChannelNumber, mute1.bCur);
        return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &mute1, sizeof(mute1));
    } else if (UAC2_ENTITY_SPK_FEATURE_UNIT && request->bControlSelector == AUDIO_FU_CTRL_VOLUME) {
        if (request->bRequest == AUDIO_CS_REQ_RANGE) {
            TU_LOG1("Get channel %u spk_volume range (%d, %d, %u) dB\r\n", request->bChannelNumber,
                    range_vol.subrange[0].bMin / 256, range_vol.subrange[0].bMax / 256, range_vol.subrange[0].bRes / 256);
            return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &range_vol, sizeof(range_vol));
        } else if (request->bRequest == AUDIO_CS_REQ_CUR) {
            audio_control_cur_2_t cur_vol = { .bCur = tu_htole16(spk_volume[request->bChannelNumber]) };
            TU_LOG1("Get channel %u spk_volume %d dB\r\n", request->bChannelNumber, cur_vol.bCur / 256);
            return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &cur_vol, sizeof(cur_vol));
        }
    }
    TU_LOG1("Feature unit get request not supported, entity = %u, selector = %u, request = %u\r\n",
            request->bEntityID, request->bControlSelector, request->bRequest);

    return false;
}

// Helper for feature unit set requests
static bool tud_audio_feature_unit_set_request(uint8_t rhport, audio_control_request_t const *request, uint8_t const *buf)
{
    (void)rhport;

    TU_ASSERT(request->bEntityID == UAC2_ENTITY_SPK_FEATURE_UNIT);
    TU_VERIFY(request->bRequest == AUDIO_CS_REQ_CUR);

    if (request->bControlSelector == AUDIO_FU_CTRL_MUTE) {
        TU_VERIFY(request->wLength == sizeof(audio_control_cur_1_t));
        spk_mute[request->bChannelNumber] = ((audio_control_cur_1_t const *)buf)->bCur;
        TU_LOG1("Set speaker channel %d Mute: %d\r\n", request->bChannelNumber, spk_mute[request->bChannelNumber]);
        bsp_codec_mute_set(spk_mute[request->bChannelNumber]);
        return true;
    } else if (request->bControlSelector == AUDIO_FU_CTRL_VOLUME) {
        TU_VERIFY(request->wLength == sizeof(audio_control_cur_2_t));
        spk_volume[request->bChannelNumber] = ((audio_control_cur_2_t const *)buf)->bCur;
        int spk_volume_db = spk_volume[request->bChannelNumber] / 256; // Convert to dB
        int volume = (spk_volume_db + 50) * 2; // Map to range 0 to 100
        TU_LOG1("Set speaker channel %d volume: %d dB (%d)\r\n", request->bChannelNumber, spk_volume_db, volume);
        bsp_codec_volume_set(volume, NULL);
        return true;
    } else {
        TU_LOG1("Feature unit set request not supported, entity = %u, selector = %u, request = %u\r\n",
                request->bEntityID, request->bControlSelector, request->bRequest);
        return false;
    }
}

//--------------------------------------------------------------------+
// Application Callback API Implementations
//--------------------------------------------------------------------+

// Invoked when audio class specific get request received for an entity
bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
    audio_control_request_t const *request = (audio_control_request_t const *)p_request;

    if (request->bEntityID == UAC2_ENTITY_CLOCK) {
        return tud_audio_clock_get_request(rhport, request);
    }
    if (request->bEntityID == UAC2_ENTITY_SPK_FEATURE_UNIT) {
        return tud_audio_feature_unit_get_request(rhport, request);
    } else {
        TU_LOG1("Get request not handled, entity = %d, selector = %d, request = %d\r\n",
                request->bEntityID, request->bControlSelector, request->bRequest);
    }
    return false;
}

// Invoked when audio class specific set request received for an entity
bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request, uint8_t *buf)
{
    audio_control_request_t const *request = (audio_control_request_t const *)p_request;

    if (request->bEntityID == UAC2_ENTITY_SPK_FEATURE_UNIT) {
        return tud_audio_feature_unit_set_request(rhport, request, buf);
    }
    if (request->bEntityID == UAC2_ENTITY_CLOCK) {
        return tud_audio_clock_set_request(rhport, request, buf);
    }
    TU_LOG1("Set request not handled, entity = %d, selector = %d, request = %d\r\n",
            request->bEntityID, request->bControlSelector, request->bRequest);

    return false;
}

bool tud_audio_set_itf_close_EP_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
    (void)rhport;

    uint8_t const itf = tu_u16_low(tu_le16toh(p_request->wIndex));
    uint8_t const alt = tu_u16_low(tu_le16toh(p_request->wValue));

    if (ITF_NUM_AUDIO_STREAMING_SPK == itf && alt == 0) {
        TU_LOG2("Speaker interface closed");
        s_spk_active = false;
    } else if (ITF_NUM_AUDIO_STREAMING_MIC == itf && alt == 0) {
        TU_LOG2("Microphone interface closed");
        s_mic_active = false;
    }

    return true;
}

bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
    (void)rhport;
    uint8_t const itf = tu_u16_low(tu_le16toh(p_request->wIndex));
    uint8_t const alt = tu_u16_low(tu_le16toh(p_request->wValue));

    TU_LOG2("Set interface %d alt %d\r\n", itf, alt);
    if (ITF_NUM_AUDIO_STREAMING_SPK == itf && alt != 0) {
        // due to the limitation of the codec, we just support one resolution
        // TODO: configuration of the resolution of the speaker
        uint8_t spk_resolution = spk_resolutions_per_format[alt - 1];
        s_spk_resolution = spk_resolution;
        s_spk_bytes_ms = s_sample_rate / 1000 * s_spk_resolution * DEFAULT_PLAYER_CHANNEL / 8;
        s_spk_active = true;
        s_spk_buf_len = 0;
        xTaskNotifyGive(spk_task_handle);
        TU_LOG1("Speaker interface %d-%d opened\n", itf, alt);
    } else if (ITF_NUM_AUDIO_STREAMING_MIC == itf && alt != 0) {
        // due to the limitation of the codec, we just support one resolution
        // TODO: configuration of the resolution of the microphone
        uint8_t mic_resolution = mic_resolutions_per_format[alt - 1];
        s_mic_resolution = mic_resolution;
        s_mic_bytes_ms = s_sample_rate / 1000 * s_mic_resolution * DEFAULT_RECORDER_CHANNEL / 8;
        s_mic_active = true;
        s_mic_read_buf_len = 0;
        xTaskNotifyGive(mic_task_handle);
        TU_LOG1("Microphone interface %d-%d opened\n", itf, alt);
    }

    return true;
}

bool tud_audio_rx_done_post_read_cb(uint8_t rhport, uint16_t n_bytes_received, uint8_t func_id, uint8_t ep_out, uint8_t cur_alt_setting)
{
    (void)rhport;
    (void)func_id;
    (void)ep_out;
    (void)cur_alt_setting;

    int bytes_remained = tud_audio_available();
    size_t bytes_require = (CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_MS - 1) * s_spk_bytes_ms;
    if (bytes_remained < bytes_require) {
        return true;
    }
    // read data chunk by chunk
    size_t bytes_frame = s_spk_resolution * DEFAULT_PLAYER_CHANNEL / 8;
    bytes_require = bytes_remained - bytes_remained % bytes_frame;
    s_spk_buf_len = tud_audio_read(s_spk_buf, bytes_require);
    xTaskNotifyGive(spk_task_handle);
    return true;
}

bool tud_audio_tx_done_pre_load_cb(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
    (void)rhport;
    (void)itf;
    (void)ep_in;
    (void)cur_alt_setting;

    size_t bytes_require = (CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_MS - 1) * s_mic_bytes_ms;
    tu_fifo_t *sw_in_fifo = tud_audio_get_ep_in_ff();
    uint16_t fifo_remained = tu_fifo_remaining(sw_in_fifo);
    if (fifo_remained < bytes_require) {
        return true;
    }
    // load data chunk by chunk
    UAC_ENTER_CRITICAL();
    if (s_mic_read_buf_len > 0) {
        tud_audio_write(s_mic_read_buf, s_mic_read_buf_len);
        s_mic_read_buf_len = 0;
    }
    UAC_EXIT_CRITICAL();
    return true;
}
