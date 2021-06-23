#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "dirent.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#ifndef CONFIG_IDF_TARGET_ESP32S2
#include "driver/sdmmc_host.h"
#endif
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "sdcard_init.h"

#define SD_CARD_TAG                 "SD_CARD_UTIL"
#define SD_CARD_INTR_GPIO           34
#define SD_CARD_PWR_CTRL            13

#if defined CONFIG_ESP32_KORVO_V1_1_BOARD || defined CONFIG_ESP32_S3_KORVO_V2_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V1_0_BOARD || defined CONFIG_ESP_KORVO_MIX_A_V1_0_BOARD
#define PIN_NUM_MISO 21
#define PIN_NUM_MOSI 18
#define PIN_NUM_CLK  5
#define PIN_NUM_CS   23
#endif


#define SDMMC_SLOT_CONFIG_DEFAULT_MIX() {\
    .clk = 38, \
    .cmd = 39, \
    .d0 = 42, \
    .d1 = 4, \
    .d2 = 12, \
    .d3 = 1, \
    .d4 = 33, \
    .d5 = 34, \
    .d6 = 35, \
    .d7 = 36, \
    .gpio_cd = SDMMC_SLOT_NO_CD, \
    .gpio_wp = SDMMC_SLOT_NO_WP, \
    .width   = SDMMC_SLOT_WIDTH_DEFAULT, \
    .flags = 0, \
}

#define SDMMC_SLOT_CONFIG_DEFAULT_KORVO_V2() {\
    .clk = 18, \
    .cmd = 17, \
    .d0 = 16, \
    .d1 = 4, \
    .d2 = 12, \
    .d3 = 15, \
    .d4 = 33, \
    .d5 = 34, \
    .d6 = 35, \
    .d7 = 36, \
    .gpio_cd = SDMMC_SLOT_NO_CD, \
    .gpio_wp = SDMMC_SLOT_NO_WP, \
    .width   = SDMMC_SLOT_WIDTH_DEFAULT, \
    .flags = 0, \
}

int sd_card_mount(const char* basePath)
{
#if defined CONFIG_ESP_LYRAT_V4_3_BOARD || defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

#ifdef CONFIG_ESP_LYRAT_V4_3_BOARD
    // To use 1-line SD mode, uncomment the following line:
    host.flags = SDMMC_HOST_FLAG_1BIT;
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.gpio_cd = SD_CARD_INTR_GPIO;

#elif defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
    gpio_config_t sdcard_pwr_pin_cfg = {
        .pin_bit_mask = 1UL << SD_CARD_PWR_CTRL,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&sdcard_pwr_pin_cfg);
    gpio_set_level(SD_CARD_PWR_CTRL, 0);

    host.flags = SDMMC_HOST_FLAG_1BIT;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.gpio_cd = SD_CARD_INTR_GPIO;
    slot_config.width = 1;

#endif

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5
    };

    sdmmc_card_t *card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(basePath, &host, &slot_config, &mount_config, &card);

    switch (ret) {
    case ESP_OK:
        // Card has been initialized, print its properties
        sdmmc_card_print_info(stdout, card);
        ESP_LOGI(SD_CARD_TAG, "CID name %s!\n", card->cid.name);
        return 1;
        break;
    case ESP_ERR_INVALID_STATE:
        ESP_LOGE(SD_CARD_TAG, "File system already mounted");
        break;
    case ESP_FAIL:
        ESP_LOGE(SD_CARD_TAG, "Failed to mount filesystem. If you want the card to be formatted, set format_if_mount_failed = true.");
        break;
    default:
        ESP_LOGE(SD_CARD_TAG, "Failed to initialize the card (%d). Make sure SD card lines have pull-up resistors in place.", ret);
        break;
    }
    return -1;

#elif defined CONFIG_ESP32_KORVO_V1_1_BOARD || defined CONFIG_ESP_KORVO_MIX_A_V1_0_BOARD

    ESP_LOGI("APP_TAG", "Initializing SD card");
    ESP_LOGI("APP_TAG", "Using SPI peripheral");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
    slot_config.gpio_miso = PIN_NUM_MISO;
    slot_config.gpio_mosi = PIN_NUM_MOSI;
    slot_config.gpio_sck  = PIN_NUM_CLK;
    slot_config.gpio_cs   = PIN_NUM_CS;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE("APP_TAG", "Failed to mount filesystem. "
                     "If you want the card to be formatted, set format_if_mount_failed = true.");

        } else {
            ESP_LOGE("APP_TAG", "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
            return -1;
        }

    }
    sdmmc_card_print_info(stdout, card);
    return 1;


#elif defined CONFIG_ESP32_S3_KORVO_V1_0_BOARD
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t* card;
    ESP_LOGI("APP_TAG", "Using SDMMC peripheral");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    // To use 1-line SD mode, uncomment the following line:
    slot_config.width = 1;
    gpio_set_pull_mode(7, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
    gpio_set_pull_mode(6, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
    gpio_set_pull_mode(5, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes

    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE("APP_TAG", "Failed to mount filesystem. "
                     "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE("APP_TAG", "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return -1;
    }

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
    return 1;
#elif defined CONFIG_ESP_KORVO_MIX_B_V1_0_BOARD || defined CONFIG_ESP_KORVO_MIX_B_V2_0_BOARD
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t* card;
    ESP_LOGI("APP_TAG", "MIX Using SDMMC peripheral");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT_MIX();

    // To use 1-line SD mode, uncomment the following line:
    slot_config.width = 1;
    gpio_set_pull_mode(39, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
    gpio_set_pull_mode(42, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
    gpio_set_pull_mode(1, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes

    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE("APP_TAG", "Failed to mount filesystem. "
                     "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE("APP_TAG", "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return 0;
    }

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
    return 1;
#elif defined CONFIG_ESP32_S3_KORVO_V2_0_BOARD
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t* card;
    ESP_LOGI("APP_TAG", "Using SDMMC peripheral");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT_KORVO_V2();

    // To use 1-line SD mode, uncomment the following line:
    slot_config.width = 1;
    gpio_set_pull_mode(17, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
    gpio_set_pull_mode(16, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
    gpio_set_pull_mode(15, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes

    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE("APP_TAG", "Failed to mount filesystem. "
                     "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE("APP_TAG", "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return -1;
    }

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
    return 1;

#elif defined CONFIG_ESP32_S3_KORVO_V3_0_BOARD
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t* card;
    ESP_LOGI("APP_TAG", "Using SDMMC peripheral");
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    // sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT_KORVO_V2();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    // To use 1-line SD mode, uncomment the following line:
    slot_config.width = 1;

    slot_config.clk = GPIO_NUM_18;
    slot_config.cmd = GPIO_NUM_17;
    slot_config.d0 = GPIO_NUM_16;
    slot_config.d1 = -1;
    slot_config.d2 = -1;
    slot_config.d3 = GPIO_NUM_15;

    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE("APP_TAG", "Failed to mount filesystem. "
                     "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE("APP_TAG", "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return -1;
    }
    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
    return 1;
#elif defined CONFIG_ESP32_S3_CUBE_V2_0_BOARD
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t* card;
    ESP_LOGI("APP_TAG", "Using SDMMC peripheral");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    // sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT_KORVO_V2();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    // To use 1-line SD mode, uncomment the following line:
    slot_config.width = 1;

    slot_config.clk = GPIO_NUM_10;
    slot_config.cmd = GPIO_NUM_9;
    slot_config.d0 = GPIO_NUM_11;
    slot_config.d1 = -1;
    slot_config.d2 = -1;
    slot_config.d3 = -1;

    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE("APP_TAG", "Failed to mount filesystem. "
                     "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE("APP_TAG", "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return -1;
    }
    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
    return 1;
#else
    esp_err_t ret = 0;
    ESP_LOGW("APP_TAG", "SD card is not currently supported");
#endif

}


int FatfsComboWrite(const void* buffer, int size, int count, FILE* stream)
{
    int res = 0;
    res = fwrite(buffer, size, count, stream);
    res |= fflush(stream);        // required by stdio, this will empty any buffers which newlib holds
    res |= fsync(fileno(stream)); // this will tell the filesystem driver to write data to disk

    return res;
}

