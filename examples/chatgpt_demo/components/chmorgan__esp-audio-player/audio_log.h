#pragma once

#include "esp_log.h"

#if CONFIG_AUDIO_PLAYER_LOG_LEVEL >= 1
#define LOGI_1(FMT, ...) \
    ESP_LOGI(TAG, "[1] " FMT, ##__VA_ARGS__)
#else
#define LOGI_1(FMT, ...) { (void)TAG; }
#endif

#if CONFIG_AUDIO_PLAYER_LOG_LEVEL >= 2
#define LOGI_2(FMT, ...) \
    ESP_LOGI(TAG, "[2] " FMT, ##__VA_ARGS__)
#else
#define LOGI_2(FMT, ...) { (void)TAG;}
#endif

#if CONFIG_AUDIO_PLAYER_LOG_LEVEL >= 3
#define LOGI_3(FMT, ...) \
    ESP_LOGI(TAG, "[3] " FMT, ##__VA_ARGS__)
#define COMPILE_3(x) x
#else
#define LOGI_3(FMT, ...) { (void)TAG; }
#define COMPILE_3(x) {}
#endif
