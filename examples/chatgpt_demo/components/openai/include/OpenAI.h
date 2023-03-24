/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include <stdint.h>
#include "esp_err.h"

#define OPENAI_API_KEY_MAX_LEN 256
#define MAX_RESPONSE_TOKEN (CONFIG_MAX_TOKEN)
#define MAX_HTTP_RECV_BUFFER (1 * 1024 * 1024 + 1024)
#define MESSAGE_CONTENT_SIZE (CONFIG_MESSAGE_CONTENT_SIZE)

/**
 * @brief region of server
 */
typedef enum {
    REGION_OpenAI,
    REGION_Espressif,
    REGION_Max,
} region_t;

/**
 * @brief audio file format supported
 */
typedef enum {
    FORMAT_M4A,
    FORMAT_MP3,
    FORMAT_WEBM,
    FORMAT_MP4,
    FORMAT_MPGA,
    FORMAT_WAV,
    FORMAT_MPEG,
} format_t;

/**
 * @brief The audio file format to use.
 */
extern format_t file_type;

/**
 * @brief OpenAI API Authentication key.
 */
extern char OPENAI_API_KEY[OPENAI_API_KEY_MAX_LEN];

/**
 * @brief Set the OpenAI API key to use for OpenAI services.
 *
 * @param key The API key to set. Must be less than OPENAI_API_KEY_MAX_LEN characters long.
 */
void set_api_key(const char *key);

/**
 * @brief Set the region to use for AI services.
 *
 * @param server The server region to set.
 */
void set_server(const region_t server);

/**
 * @brief Set the audio file format to use.
 *
 * @param type The audio file format to set.
 */
void set_audio_type(const format_t type);

/**
 * @brief Get the string representation of the audio file format.
 *
 * @param file_type The audio file format.
 * @return The string representation of the audio file format.
 */
char *get_file_format(format_t file_type);

/**
 * @brief Get the message content for the last response from ChatGPT.
 *
 * @return The message content for the last response.
 */
char *get_message_content_for_chatgpt();

/**
 * @brief Get the message content for the last response from Whisper AI.
 *
 * @return The message content for the last response.
 */
char *get_message_content_for_whisper();

/**
 * @brief Create a ChatGPT request for the given content.
 *
 * @param content The content of the request.
 * @return ESP_OK on success, or an error code otherwise.
 */
esp_err_t create_chatgpt_request(const char *content);

/**
 * @brief Create a Whisper AI request from the recorded audio.
 *
 * @param audio The recorded audio to use for the request.
 * @param audio_len The length of the recorded audio.
 * @return ESP_OK on success, or an error code otherwise.
 */
esp_err_t create_whisper_request_from_record(uint8_t *audio, int audio_len);
