/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once
#include "esp_err.h"
#include "stdbool.h"

#define MAX_FILE_SIZE       (1*1024*1024)
#define FILE_SIZE (256000)

/**
 * @brief Audio format supported.
 */
typedef enum {
    AUDIO_CODECS_MP3,
    AUDIO_CODECS_WAV,
} AUDIO_CODECS_FORMAT;

/**
 * @brief Length of file.
 */
extern uint32_t file_total_len;

/**
 * @brief Create a Text to Speech request from the reposne from ChatGPT Api.
 *
 * @param message The message used for the request
 * @param code_format The audio format.
 * @return ESP_OK on success, or an error code otherwise.
 */
esp_err_t text_to_speech_request(const char *message, AUDIO_CODECS_FORMAT code_format);
