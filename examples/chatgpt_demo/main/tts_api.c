/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <string.h>
#include"tts_api.h"
#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "app_audio.h"
#include "app_ui_ctrl.h"
#include "audio_player.h"
#include "esp_crt_bundle.h"
#include "inttypes.h"

#define VOICE_ID CONFIG_VOICE_ID
#define VOLUME CONFIG_VOLUME_LEVEL

static const char *TAG = "TTS-Api";

/* Define a function to handle HTTP events during an HTTP request */

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
        file_total_len = 0;
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=(%"PRIu32" + %d) [%d]", file_total_len, evt->data_len, MAX_FILE_SIZE);
        if ((file_total_len + evt->data_len) < MAX_FILE_SIZE) {
            memcpy(audio_rx_buffer + file_total_len, (char *)evt->data, evt->data_len);
            file_total_len += evt->data_len;
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH:%"PRIu32", %"PRIu32" K", file_total_len, file_total_len / 1024);
        audio_player_play(audio_rx_buffer, file_total_len);
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}

/* Decode 2 Hex */

char dec2hex(short int c)
{
    if (0 <= c && c <= 9) {
        return c + '0';
    } else if (10 <= c && c <= 15) {
        return c + 'A' - 10;
    } else {
        return -1;
    }
}

/* Encode URL for playing sound */

void url_encode(const char *url, char *encode_out)
{
    int i = 0;
    int len = strlen(url);
    int res_len = 0;

    assert(encode_out);

    for (i = 0; i < len; ++i) {
        char c = url[i];
        char n = url[i + 1];
        if (c == '\\' && n == 'n') {
            i += 1;
            continue;
        } else if (('0' <= c && c <= '9') || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '/' || c == '.') {
            encode_out[res_len++] = c;
        } else {
            int j = (short int)c;
            if (j < 0) {
                j += 256;
            }
            int i1, i0;
            i1 = j / 16;
            i0 = j - i1 * 16;
            encode_out[res_len++] = '%';
            encode_out[res_len++] = dec2hex(i1);
            encode_out[res_len++] = dec2hex(i0);
        }
    }
    encode_out[res_len] = '\0';
}

/* Create Text to Speech request */

esp_err_t text_to_speech_request(const char *message, AUDIO_CODECS_FORMAT code_format)
{
    size_t message_len = strlen(message);
    char *encoded_message;
    char *codec_format_str;
    encoded_message = heap_caps_malloc((3 * message_len + 1), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    url_encode(message, encoded_message);

    if (AUDIO_CODECS_MP3 == code_format) {
        codec_format_str = "MP3";
    } else {
        codec_format_str = "WAV";
    }
    int url_size = snprintf(NULL, 0, "https://dds.dui.ai/runtime/v1/synthesize?voiceId=%s&text=%s&speed=1&volume=%d&audiotype=%s", \
                            VOICE_ID, \
                            encoded_message, \
                            VOLUME, \
                            codec_format_str);
// Allocate memory for the URL buffer
    char *url = heap_caps_malloc((url_size + 1), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (url == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for URL");
        return ESP_ERR_NO_MEM;
    }
    snprintf(url, url_size + 1, "https://dds.dui.ai/runtime/v1/synthesize?voiceId=%s&text=%s&speed=1&volume=%d&audiotype=%s", \
             VOICE_ID, \
             encoded_message, \
             VOLUME, \
             codec_format_str);
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .event_handler = http_event_handler,
        .buffer_size = 128000,
        .buffer_size_tx = 4000,
        .timeout_ms = 40000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    uint32_t starttime = esp_log_timestamp();
    ESP_LOGE(TAG, "[Start] create_TTS_request, timestamp:%"PRIu32, starttime);
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    ESP_LOGE(TAG, "[End] create_TTS_request, + offset:%"PRIu32, esp_log_timestamp() - starttime);

    heap_caps_free(url);
    heap_caps_free(encoded_message);
    esp_http_client_cleanup(client);
    return err;
}
