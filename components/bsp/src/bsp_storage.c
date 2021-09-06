/**
 * @file bsp_storage.c
 * @brief 
 * @version 0.1
 * @date 2021-06-25
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
#include "bsp_storage.h"
#include "esp_err.h"
#include "esp_spiffs.h"
#include "esp_vfs_fat.h"
#include "esp_vfs_semihost.h"
#include "sdmmc_cmd.h"

/* Add SDMMC driver header if SoC supports SDMMC */
#if SOC_SDMMC_HOST_SUPPORTED
#include "driver/sdmmc_host.h"
#endif /* SOC_SDMMC_HOST_SUPPORTED */

static const char *TAG = "bsp_storage";
const char sd_mount_point[] = "/sdcard";
const char spiffs_mount_point[] = "/spiffs";
const char host_mount_point[] = "/host";

/* SD/MMC card information structure */
static sdmmc_card_t *card;

static esp_err_t bsp_sdcard_init_default(void)
{
	esp_err_t ret_val = ESP_OK;

	/* Check if SDMMC is supported on board. */
	if (!FUNC_SDMMC_EN && !FUNC_SDSPI_EN) {
		ESP_LOGE(TAG, "SDMMC and SDSPI not supported on this board!");
		return ESP_ERR_NOT_SUPPORTED;
	}

	/**
	 * @brief Options for mounting the filesystem.
	 *   If format_if_mount_failed is set to true, SD card will be partitioned and
	 *   formatted in case when mounting fails.
	 * 
	 */
	esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 2,
        .allocation_unit_size = 16 * 1024
    };

	/**
	 * @brief Use settings defined above to initialize SD card and mount FAT filesystem.
	 *   Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
	 *   Please check its source code and implement error recovery when developing
	 *   production applications.
	 * 
	 */
    sdmmc_host_t host = 
#if FUNC_SDMMC_EN
    SDMMC_HOST_DEFAULT();
#else
    SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = GPIO_SDSPI_MOSI,
        .miso_io_num = GPIO_SDSPI_MISO,
        .sclk_io_num = GPIO_SDSPI_SCLK,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = 4000,
    };
    ret_val = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret_val != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return ret_val;
    }
#endif

	/**
	 * @brief This initializes the slot without card detect (CD) and write protect (WP) signals.
	 *   Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
	 * 
	 */
#if FUNC_SDMMC_EN
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
#else
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
#endif

#if FUNC_SDMMC_EN
    /* Config SD data width. 0, 4 or 8. Currently for SD card, 8 bit is not supported. */
    slot_config.width = SDMMC_BUS_WIDTH;

	/**
	 * @brief On chips where the GPIOs used for SD card can be configured, set them in
	 *   the slot_config structure.
	 * 
	 */
#if SOC_SDMMC_USE_GPIO_MATRIX
    slot_config.clk = GPIO_SDMMC_CLK;
    slot_config.cmd = GPIO_SDMMC_CMD;
    slot_config.d0 = GPIO_SDMMC_D0;
    slot_config.d1 = GPIO_SDMMC_D1;
    slot_config.d2 = GPIO_SDMMC_D2;
    slot_config.d3 = GPIO_SDMMC_D3;
#endif
    slot_config.cd = GPIO_SDMMC_DET;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
#else
    slot_config.gpio_cs = GPIO_SDSPI_CS;
    slot_config.host_id = host.slot;
#endif
	/**
	 * @brief Enable internal pullups on enabled pins. The internal pullups
	 *   are insufficient however, please make sure 10k external pullups are
	 *   connected on the bus. This is for debug / example purpose only.
	 */

	/* get FAT filesystem on SD card registered in VFS. */
    ret_val = 
#if FUNC_SDMMC_EN
    esp_vfs_fat_sdmmc_mount(sd_mount_point, &host, &slot_config, &mount_config, &card);
#else
    esp_vfs_fat_sdspi_mount(sd_mount_point, &host, &slot_config, &mount_config, &card);
#endif

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

static esp_err_t bsp_sdcard_deinit(void)
{
    /* Unmount an SD card from the FAT filesystem and release resources acquired */
    esp_err_t ret_val = esp_vfs_fat_sdcard_unmount(sd_mount_point, card);

    /* Make SD/MMC card information structure pointer NULL */
    card = NULL;

	return ret_val;
}

static esp_err_t bsp_spiffs_init_default(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = spiffs_mount_point,
        .partition_label = NULL,
        .max_files = 2,
        .format_if_mount_failed = false,
    };

    esp_err_t ret_val = esp_vfs_spiffs_register(&conf);

    if (ESP_OK != ret_val) {
        if (ESP_FAIL == ret_val) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ESP_ERR_NOT_FOUND == ret_val) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret_val));
        }
    }

    return ret_val;
}

static esp_err_t bsp_spiffs_init(void *cfg)
{
    char **cfg_str = (char **) cfg;

    ESP_LOGI(TAG, "Partation Name : %s", cfg_str[0]);
    ESP_LOGI(TAG, "Mount Point : %s", cfg_str[1]);

    esp_vfs_spiffs_conf_t conf = {
        .base_path = cfg_str[1],
        .partition_label = cfg_str[0],
        .max_files = 2,
        .format_if_mount_failed = false,
    };

    esp_err_t ret_val = esp_vfs_spiffs_register(&conf);

    if (ESP_OK != ret_val) {
        if (ESP_FAIL == ret_val) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ESP_ERR_NOT_FOUND == ret_val) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret_val));
        }
    }

    return ret_val;
}

esp_err_t bsp_storage_init_default(bsp_storage_dev_t dev)
{
    switch (dev) {
    case BSP_STORAGE_SD_CARD:
        return bsp_sdcard_init_default();
        break;
    case BSP_STORAGE_SPIFFS:
    return bsp_spiffs_init_default();
        break;
    case BSP_STORAGE_SEMIHOST:
        return esp_vfs_semihost_register(host_mount_point, NULL);
        break;
    default:
        break;
    }

    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bsp_storage_init(bsp_storage_dev_t dev, void *conf)
{
    switch (dev) {
    case BSP_STORAGE_SPIFFS:
    return bsp_spiffs_init(conf);
        break;
    default:
        break;
    }

    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bsp_storage_deinit_default(bsp_storage_dev_t dev)
{
    switch (dev) {
    case BSP_STORAGE_SD_CARD:
        return bsp_sdcard_deinit();
        break;
    case BSP_STORAGE_SPIFFS:
        return esp_vfs_spiffs_unregister(NULL);
        break;
    case BSP_STORAGE_SEMIHOST:
        return esp_vfs_semihost_unregister(host_mount_point);
        break;
    default:
        break;
    }

    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bsp_storage_get_mount_point(bsp_storage_dev_t dev, char **p_mont_point)
{
    switch (dev) {
    case BSP_STORAGE_SD_CARD:
        if (NULL == card) {
            ESP_LOGE(TAG, "Card not initialized!");
            return ESP_ERR_INVALID_STATE;
        }
        *p_mont_point = (char *) sd_mount_point;
        break;
    case BSP_STORAGE_SPIFFS:
        *p_mont_point = (char *) spiffs_mount_point;
        break;
    case BSP_STORAGE_SEMIHOST:
        *p_mont_point = (char *) host_mount_point;
        break;
    default:
        break;
    }

	return ESP_OK;
}
