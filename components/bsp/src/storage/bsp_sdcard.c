/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "bsp_board.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#if (SOC_SDMMC_HOST_SUPPORTED)
#include "driver/sdmmc_host.h"
#endif

#define DEFAULT_FD_NUM      2
#define DEFAULT_MOUNT_POINT "/sdcard"

static sdmmc_card_t *card;
static const char *TAG = "bsp_sdcard";

esp_err_t bsp_sdcard_init(char *mount_point, size_t max_files)
{
    if (NULL != card) {
        return ESP_ERR_INVALID_STATE;
    }
    const board_res_desc_t *brd = bsp_board_get_description();

    /* Check if SD crad is supported */
    if (!brd->FUNC_SDMMC_EN && !brd->FUNC_SDSPI_EN) {
        ESP_LOGE(TAG, "SDMMC and SDSPI not supported on this board!");
        return ESP_ERR_NOT_SUPPORTED;
    }

    esp_err_t ret_val = ESP_OK;

    /**
     * @brief Options for mounting the filesystem.
     *   If format_if_mount_failed is set to true, SD card will be partitioned and
     *   formatted in case when mounting fails.
     *
     */
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = max_files,
        .allocation_unit_size = 16 * 1024
    };

    /**
     * @brief Use settings defined above to initialize SD card and mount FAT filesystem.
     *   Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
     *   Please check its source code and implement error recovery when developing
     *   production applications.
     *
     */
    sdmmc_host_t host = {0};
    if (brd->FUNC_SDMMC_EN) {
        sdmmc_host_t h = SDMMC_HOST_DEFAULT();
        memcpy(&host, &h, sizeof(sdmmc_host_t));
    } else {
        sdmmc_host_t h = SDSPI_HOST_DEFAULT();
        memcpy(&host, &h, sizeof(sdmmc_host_t));
        spi_bus_config_t bus_cfg = {
            .mosi_io_num = brd->GPIO_SDSPI_MOSI,
            .miso_io_num = brd->GPIO_SDSPI_MISO,
            .sclk_io_num = brd->GPIO_SDSPI_SCLK,
            .quadwp_io_num = GPIO_NUM_NC,
            .quadhd_io_num = GPIO_NUM_NC,
            .max_transfer_sz = 4000,
        };
        ret_val = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CH_AUTO);
        if (ret_val != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize bus.");
            return ret_val;
        }
    }

    /**
     * @brief This initializes the slot without card detect (CD) and write protect (WP) signals.
     *   Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
     *
     */
    if (brd->FUNC_SDMMC_EN) {
        sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
        /* Config SD data width. 0, 4 or 8. Currently for SD card, 8 bit is not supported. */
        slot_config.width = brd->SDMMC_BUS_WIDTH;

        /**
         * @brief On chips where the GPIOs used for SD card can be configured, set them in
         *   the slot_config structure.
         *
         */
#if SOC_SDMMC_USE_GPIO_MATRIX
        slot_config.clk = brd->GPIO_SDMMC_CLK;
        slot_config.cmd = brd->GPIO_SDMMC_CMD;
        slot_config.d0 = brd->GPIO_SDMMC_D0;
        slot_config.d1 = brd->GPIO_SDMMC_D1;
        slot_config.d2 = brd->GPIO_SDMMC_D2;
        slot_config.d3 = brd->GPIO_SDMMC_D3;
#endif
        slot_config.cd = brd->GPIO_SDMMC_DET;
        slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
        ret_val = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);
    } else {
        sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
        slot_config.gpio_cs = brd->GPIO_SDSPI_CS;
        slot_config.host_id = host.slot;
        ret_val = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);
    }

    /* Check for SDMMC mount result. */
    if (ret_val != ESP_OK) {
        if (ret_val == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret_val));
        }
        return ret_val;
    }

    /* Card has been initialized, print its properties. */
    sdmmc_card_print_info(stdout, card);

    return ret_val;
}

esp_err_t bsp_sdcard_init_default(void)
{
    return bsp_sdcard_init(DEFAULT_MOUNT_POINT, DEFAULT_FD_NUM);
}

esp_err_t bsp_sdcard_deinit(char *mount_point)
{
    if (NULL == mount_point) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Unmount an SD card from the FAT filesystem and release resources acquired */
    esp_err_t ret_val = esp_vfs_fat_sdcard_unmount(mount_point, card);

    /* Make SD/MMC card information structure pointer NULL */
    card = NULL;

    return ret_val;
}

esp_err_t bsp_sdcard_deinit_default(void)
{
    return bsp_sdcard_deinit(DEFAULT_MOUNT_POINT);
}
