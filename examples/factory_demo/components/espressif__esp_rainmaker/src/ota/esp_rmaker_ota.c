// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
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

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <freertos/task.h>
#include <esp_efuse.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_https_ota.h>
#include <esp_wifi_types.h>
#include <esp_wifi.h>
#include <nvs.h>
#include <json_parser.h>
#if CONFIG_BT_ENABLED
#include <esp_bt.h>
#endif /* CONFIG_BT_ENABLED */

#include <esp_rmaker_utils.h>
#include <esp_rmaker_common_events.h>
#include <esp_rmaker_utils.h>
#include "esp_rmaker_internal.h"
#include "esp_rmaker_ota_internal.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
// Features supported in 4.4+

#ifdef CONFIG_ESP_RMAKER_USE_CERT_BUNDLE
#define ESP_RMAKER_USE_CERT_BUNDLE
#include <esp_crt_bundle.h>
#endif

#else

#ifdef CONFIG_ESP_RMAKER_USE_CERT_BUNDLE
#warning "Certificate Bundle not supported below IDF v4.4. Using provided certificate instead."
#endif

#endif /* !IDF4.4 */
static const char *TAG = "esp_rmaker_ota";
static TimerHandle_t s_ota_rollback_timer;

#define OTA_REBOOT_TIMER_SEC    10
#define DEF_HTTP_TX_BUFFER_SIZE    1024
#define DEF_HTTP_RX_BUFFER_SIZE    CONFIG_ESP_RMAKER_OTA_HTTP_RX_BUFFER_SIZE
#define RMAKER_OTA_ROLLBACK_WAIT_PERIOD    CONFIG_ESP_RMAKER_OTA_ROLLBACK_WAIT_PERIOD
extern const char esp_rmaker_ota_def_cert[] asm("_binary_rmaker_ota_server_crt_start");
const char *ESP_RMAKER_OTA_DEFAULT_SERVER_CERT = esp_rmaker_ota_def_cert;
ESP_EVENT_DEFINE_BASE(RMAKER_OTA_EVENT);

typedef enum {
    OTA_OK = 0,
    OTA_ERR,
    OTA_DELAYED
} esp_rmaker_ota_action_t;

char *esp_rmaker_ota_status_to_string(ota_status_t status)
{
    switch (status) {
        case OTA_STATUS_IN_PROGRESS:
            return "in-progress";
        case OTA_STATUS_SUCCESS:
            return "success";
        case OTA_STATUS_FAILED:
            return "failed";
        case OTA_STATUS_DELAYED:
            return "delayed";
        case OTA_STATUS_REJECTED:
            return "rejected";
        default:
            return "invalid";
    }
    return "invalid";
}

esp_rmaker_ota_event_t esp_rmaker_ota_status_to_event(ota_status_t status)
{
    switch (status) {
        case OTA_STATUS_IN_PROGRESS:
            return RMAKER_OTA_EVENT_IN_PROGRESS;
        case OTA_STATUS_SUCCESS:
            return RMAKER_OTA_EVENT_SUCCESSFUL;
        case OTA_STATUS_FAILED:
            return RMAKER_OTA_EVENT_FAILED;
        case OTA_STATUS_DELAYED:
            return RMAKER_OTA_EVENT_DELAYED;
        case OTA_STATUS_REJECTED:
            return RMAKER_OTA_EVENT_REJECTED;
        default:
            ESP_LOGD(TAG, "No Rmaker OTA Event for given status: %d: %s",
                    status, esp_rmaker_ota_status_to_string(status));
    }
    return RMAKER_OTA_EVENT_INVALID;
}

static inline esp_err_t esp_rmaker_ota_post_event(esp_rmaker_event_t event_id, void* data, size_t data_size)
{
    return esp_event_post(RMAKER_OTA_EVENT, event_id, data, data_size, portMAX_DELAY);
}

esp_err_t esp_rmaker_ota_report_status(esp_rmaker_ota_handle_t ota_handle, ota_status_t status, char *additional_info)
{
    ESP_LOGI(TAG, "Reporting %s: %s", esp_rmaker_ota_status_to_string(status), additional_info);

    if (!ota_handle) {
        return ESP_FAIL;
    }
    esp_rmaker_ota_t *ota = (esp_rmaker_ota_t *)ota_handle;
    esp_err_t err = ESP_FAIL;
    if (ota->type == OTA_USING_PARAMS) {
        err = esp_rmaker_ota_report_status_using_params(ota_handle, status, additional_info);
    } else if (ota->type == OTA_USING_TOPICS) {
        err = esp_rmaker_ota_report_status_using_topics(ota_handle, status, additional_info);
    }
    if (err == ESP_OK) {
        esp_rmaker_ota_t *ota = (esp_rmaker_ota_t *)ota_handle;
        ota->last_reported_status = status;
    }
    esp_rmaker_ota_post_event(esp_rmaker_ota_status_to_event(status), additional_info, strlen(additional_info) + 1);
    return err;
}

void esp_rmaker_ota_common_cb(void *priv)
{
    if (!priv) {
        return;
    }
    esp_rmaker_ota_t *ota = (esp_rmaker_ota_t *)priv;
    if (!ota->url) {
        goto ota_finish;
    }
    esp_rmaker_ota_data_t ota_data = {
        .url = ota->url,
        .filesize = ota->filesize,
        .fw_version = ota->fw_version,
        .ota_job_id = (char *)ota->transient_priv,
        .server_cert = ota->server_cert,
        .priv = ota->priv,
        .metadata = ota->metadata
    };
    ota->ota_cb((esp_rmaker_ota_handle_t) ota, &ota_data);
ota_finish:
    if (ota->type == OTA_USING_PARAMS) {
        esp_rmaker_ota_finish_using_params(ota);
    } else if (ota->type == OTA_USING_TOPICS) {
        esp_rmaker_ota_finish_using_topics(ota);
    }
}

static esp_err_t validate_image_header(esp_rmaker_ota_handle_t ota_handle,
        esp_app_desc_t *new_app_info)
{
    if (new_app_info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        ESP_LOGD(TAG, "Running firmware version: %s", running_app_info.version);
    }

#ifndef CONFIG_ESP_RMAKER_SKIP_PROJECT_NAME_CHECK
    if (memcmp(new_app_info->project_name, running_app_info.project_name, sizeof(new_app_info->project_name)) != 0) {
        ESP_LOGW(TAG, "OTA Image built for Project: %s. Expected: %s",
                new_app_info->project_name, running_app_info.project_name);
        esp_rmaker_ota_report_status(ota_handle, OTA_STATUS_REJECTED, "Project Name mismatch");
        return ESP_FAIL;
    }
#endif

#ifndef CONFIG_ESP_RMAKER_SKIP_VERSION_CHECK
    if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0) {
        ESP_LOGW(TAG, "Current running version is same as the new. We will not continue the update.");
        esp_rmaker_ota_report_status(ota_handle, OTA_STATUS_REJECTED, "Same version received");
        return ESP_FAIL;
    }
#endif

#ifndef CONFIG_ESP_RMAKER_SKIP_SECURE_VERSION_CHECK
    if (esp_efuse_check_secure_version(new_app_info->secure_version) == false) {
        ESP_LOGW(TAG, "New secure version is lower than stored in efuse. We will not continue the update.");
        esp_rmaker_ota_report_status(ota_handle, OTA_STATUS_REJECTED, "Lower secure version received");
        return ESP_FAIL;
    }
#endif

    return ESP_OK;
}

#ifdef CONFIG_ESP_RMAKER_OTA_TIME_SUPPORT

/* Retry delay for cases wherein time info itself is not available */
#define OTA_FETCH_RETRY_DELAY   30
#define MINUTES_IN_DAY          (24 * 60)
#define OTA_DELAY_TIME_BUFFER   5

/* Check if time data is available in the metadata. Format
 * {"download_window":{"end":1155,"start":1080},"validity":{"end":1665426600,"start":1665081000}}
 */
esp_rmaker_ota_action_t esp_rmaker_ota_handle_time(jparse_ctx_t *jptr, esp_rmaker_ota_handle_t ota_handle, esp_rmaker_ota_data_t *ota_data)
{
    bool time_info = false;
    int start_min = -1, end_min = -1, start_date = -1, end_date = -1;
    if (json_obj_get_object(jptr, "download_window") == 0) {
        /* Download window means specific time of day. Eg, Between 02:00am and 05:00am only */
        time_info = true;
        json_obj_get_int(jptr, "start", &start_min);
        json_obj_get_int(jptr, "end", &end_min);
        json_obj_leave_object(jptr);
        ESP_LOGI(TAG, "Download Window : %d %d", start_min, end_min);
    }
    if (json_obj_get_object(jptr, "validity") == 0) {
        /* Validity indicates start and end epoch time, typicaly useful if OTA is to be performed between some dates */
        time_info = true;
        json_obj_get_int(jptr, "start", &start_date);
        json_obj_get_int(jptr, "end", &end_date);
        json_obj_leave_object(jptr);
        ESP_LOGI(TAG, "Validity : %d %d", start_date, end_date);
    }
    if (time_info) {
        /* If time info is present, but time is not yet synchronised, we will re-fetch OTA after some time */
        if (esp_rmaker_time_check() != true) {
            esp_rmaker_ota_report_status(ota_handle, OTA_STATUS_DELAYED, "No time information available yet.");
            esp_rmaker_ota_fetch_with_delay(OTA_FETCH_RETRY_DELAY);
            return OTA_DELAYED;
        }
        time_t current_timestamp = 0;
        struct tm current_time = {0};
        time(&current_timestamp);
        localtime_r(&current_timestamp, &current_time);

        /* Check for date validity first */
        if ((start_date != -1) && (current_timestamp < start_date)) {
            int delay_time = start_date - current_timestamp;
            /* The delay logic here can include the start_min and end_min as well, but it makes the logic quite complex,
             * just for a minor optimisation.
             */
            ESP_LOGI(TAG, "Delaying OTA by %d seconds (%d min) as it is not valid yet.", delay_time, delay_time / 60);
            esp_rmaker_ota_report_status(ota_handle, OTA_STATUS_DELAYED, "Not within valid window.");
            esp_rmaker_ota_fetch_with_delay(delay_time + OTA_DELAY_TIME_BUFFER);
            return OTA_DELAYED;
        } else if ((end_date != -1) && (current_timestamp > end_date)) {
            ESP_LOGE(TAG, "OTA download window lapsed");
            esp_rmaker_ota_report_status(ota_handle, OTA_STATUS_FAILED, "OTA download window lapsed.");
            return OTA_ERR;
        }

        /* Check for download window */
        if (start_min != -1) {
            /* end_min is required if start_min is provided */
            if (end_min == -1) {
                ESP_LOGE(TAG, "Download window should have an end time if start time is specified.");
                esp_rmaker_ota_report_status(ota_handle, OTA_STATUS_FAILED, "Invalid download window specified.");
                return OTA_ERR;
            }
            int cur_min = current_time.tm_hour * 60 + current_time.tm_min;
            if (start_min > end_min) {
                /* This means that the window is across midnight (Eg. 23:00 to 02:00 i.e. 1380 to 120).
                 * We are just moving the window here such that start_min becomes 0 and the comparisons are simplified.
                 * For this example, diff_min will be  1440 - 1380 = 60.
                 * Effective end_min: 180
                 * If cur_time is 18:00, effective cur_time = 1080 + 60 = 1140
                 * If cur_time is 23:30, effective cur_time = 1410 + 60 = 1470 ( > MINUTES_IN_DAY)
                 *          So, cur_time = 1470 - 1440 = 30
                 * */
                int diff_min = MINUTES_IN_DAY - start_min;
                start_min = 0;
                end_min += diff_min;
                cur_min += diff_min;
                if (cur_min >= MINUTES_IN_DAY) {
                    cur_min -= MINUTES_IN_DAY;
                }
            }
            /* Current time is within OTA download window */
            if ((cur_min >= start_min) && (cur_min <= end_min)) {
                ESP_LOGI(TAG, "OTA received within download window.");
                return OTA_OK;
            } else {
                /* Delay the OTA if it is not in the download window. Even if it later goes outside the valid date range,
                 * that will be handled in subsequent ota fetch. Reporting failure here itself would mark the OTA job
                 * as failed and the node will no more get the OTA even if it tries to fetch it again due to a reboot or
                 * other action within the download window.
                 */
                int delay_min = start_min - cur_min;
                if (delay_min < 0) {
                    delay_min += MINUTES_IN_DAY;
                }
                ESP_LOGI(TAG, "Delaying OTA by %d seconds (%d min) as it is not within download window.", delay_min * 60, delay_min);
                esp_rmaker_ota_report_status(ota_handle, OTA_STATUS_DELAYED, "Not within download window.");
                esp_rmaker_ota_fetch_with_delay(delay_min * 60 + OTA_DELAY_TIME_BUFFER);
                return OTA_DELAYED;
            }
        } else {
            ESP_LOGI(TAG, "OTA received within validity period.");
        }
    }
    return OTA_OK;
}

#endif /* CONFIG_ESP_RMAKER_OTA_TIME_SUPPORT */

esp_rmaker_ota_action_t esp_rmaker_ota_handle_metadata(esp_rmaker_ota_handle_t ota_handle, esp_rmaker_ota_data_t *ota_data)
{
    if (!ota_data->metadata) {
        return ESP_OK;
    }
    esp_rmaker_ota_action_t ota_action = OTA_OK;
    jparse_ctx_t jctx;
    if (json_parse_start(&jctx, ota_data->metadata, strlen(ota_data->metadata)) == 0) {
#ifdef CONFIG_ESP_RMAKER_OTA_TIME_SUPPORT
        /* Handle OTA timing data, if any */
        ota_action = esp_rmaker_ota_handle_time(&jctx, ota_handle, ota_data);
#endif /* CONFIG_ESP_RMAKER_OTA_TIME_SUPPORT */
        json_parse_end(&jctx);
    }
    return ota_action;
}

esp_err_t esp_rmaker_ota_default_cb(esp_rmaker_ota_handle_t ota_handle, esp_rmaker_ota_data_t *ota_data)
{
    if (!ota_data->url) {
        return ESP_FAIL;
    }
    /* Handle OTA metadata, if any */
    if (ota_data->metadata) {
        if (esp_rmaker_ota_handle_metadata(ota_handle, ota_data) != OTA_OK) {
            ESP_LOGW(TAG, "Cannot proceed with the OTA as per the metadata received.");
            return ESP_FAIL;
        }
    }
    esp_rmaker_ota_post_event(RMAKER_OTA_EVENT_STARTING, NULL, 0);
    int buffer_size_tx = DEF_HTTP_TX_BUFFER_SIZE;
    /* In case received url is longer, we will increase the tx buffer size
     * to accomodate the longer url and other headers.
     */
    if (strlen(ota_data->url) > buffer_size_tx) {
        buffer_size_tx = strlen(ota_data->url) + 128;
    }
    esp_err_t ota_finish_err = ESP_OK;
    esp_http_client_config_t config = {
        .url = ota_data->url,
#ifdef ESP_RMAKER_USE_CERT_BUNDLE
        .crt_bundle_attach = esp_crt_bundle_attach,
#else
        .cert_pem = ota_data->server_cert,
#endif
        .timeout_ms = 5000,
        .buffer_size = DEF_HTTP_RX_BUFFER_SIZE,
        .buffer_size_tx = buffer_size_tx,
        .keep_alive_enable = true
    };
#ifdef CONFIG_ESP_RMAKER_SKIP_COMMON_NAME_CHECK
    config.skip_cert_common_name_check = true;
#endif

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };

    if (ota_data->filesize) {
        ESP_LOGD(TAG, "Received file size: %d", ota_data->filesize);
    }

    esp_rmaker_ota_report_status(ota_handle, OTA_STATUS_IN_PROGRESS, "Starting OTA Upgrade");

    /* Using a warning just to highlight the message */
    ESP_LOGW(TAG, "Starting OTA. This may take time.");
    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed");
        esp_rmaker_ota_report_status(ota_handle, OTA_STATUS_FAILED, "ESP HTTPS OTA Begin failed");
        return ESP_FAIL;
    }

/* Get the current Wi-Fi power save type. In case OTA fails and we need this
 * to restore power saving.
 */
    wifi_ps_type_t ps_type;
    esp_wifi_get_ps(&ps_type);
/* Disable Wi-Fi power save to speed up OTA, iff BT is controller is idle/disabled.
 * Co-ex requirement, device panics otherwise.*/
#if CONFIG_BT_ENABLED
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE) {
        esp_wifi_set_ps(WIFI_PS_NONE);
    }
#else
    esp_wifi_set_ps(WIFI_PS_NONE);
#endif /* CONFIG_BT_ENABLED */

    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_read_img_desc failed");
        esp_rmaker_ota_report_status(ota_handle, OTA_STATUS_FAILED, "Failed to read image decription");
        goto ota_end;
    }
    err = validate_image_header(ota_handle, &app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "image header verification failed");
        goto ota_end;
    }

    esp_rmaker_ota_report_status(ota_handle, OTA_STATUS_IN_PROGRESS, "Downloading Firmware Image");
    int count = 0;
    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err == ESP_ERR_INVALID_VERSION) {
            esp_rmaker_ota_report_status(ota_handle, OTA_STATUS_REJECTED, "Chip revision mismatch");
            goto ota_end;
        }
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
        /* esp_https_ota_perform returns after every read operation which gives user the ability to
         * monitor the status of OTA upgrade by calling esp_https_ota_get_image_len_read, which gives length of image
         * data read so far.
         * We are using a counter just to reduce the number of prints
         */

        count++;
        if (count == 50) {
            ESP_LOGI(TAG, "Image bytes read: %d", esp_https_ota_get_image_len_read(https_ota_handle));
            count = 0;
        }
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed %s", esp_err_to_name(err));
        char description[40];
        snprintf(description, sizeof(description), "OTA failed: Error %s", esp_err_to_name(err));
        esp_rmaker_ota_report_status(ota_handle, OTA_STATUS_FAILED, description);
    }

    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
        // the OTA image was not completely received and user can customise the response to this situation.
        ESP_LOGE(TAG, "Complete data was not received.");
    } else {
        esp_rmaker_ota_report_status(ota_handle, OTA_STATUS_IN_PROGRESS, "Firmware Image download complete");
    }

ota_end:
#ifdef CONFIG_BT_ENABLED
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE) {
        esp_wifi_set_ps(ps_type);
    }
#else
    esp_wifi_set_ps(ps_type);
#endif /* CONFIG_BT_ENABLED */
    ota_finish_err = esp_https_ota_finish(https_ota_handle);
    if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
        nvs_handle handle;
        esp_err_t err = nvs_open_from_partition(ESP_RMAKER_NVS_PART_NAME, RMAKER_OTA_NVS_NAMESPACE, NVS_READWRITE, &handle);
        if (err == ESP_OK) {
            uint8_t ota_update = 1;
            nvs_set_blob(handle, RMAKER_OTA_UPDATE_FLAG_NVS_NAME, &ota_update, sizeof(ota_update));
            nvs_close(handle);
        }
        /* Success will be reported after a reboot since Rollback is enabled */
        esp_rmaker_ota_report_status(ota_handle, OTA_STATUS_IN_PROGRESS, "Rebooting into new firmware");
#else
        esp_rmaker_ota_report_status(ota_handle, OTA_STATUS_SUCCESS, "OTA Upgrade finished successfully");
#endif
#ifndef CONFIG_ESP_RMAKER_OTA_DISABLE_AUTO_REBOOT
        ESP_LOGI(TAG, "OTA upgrade successful. Rebooting in %d seconds...", OTA_REBOOT_TIMER_SEC);
        esp_rmaker_reboot(OTA_REBOOT_TIMER_SEC);
#else
        ESP_LOGI(TAG, "OTA upgrade successful. Auto reboot is disabled. Requesting a Reboot via Event handler.");
        esp_rmaker_ota_post_event(RMAKER_OTA_EVENT_REQ_FOR_REBOOT, NULL, 0);
#endif
        return ESP_OK;
    } else {
        if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
            esp_rmaker_ota_report_status(ota_handle, OTA_STATUS_FAILED, "Image validation failed");
        } else {
            /* Not reporting status here, because relevant error will already be reported
             * in some earlier step
             */
            ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed %d", ota_finish_err);
        }
    }
    return ESP_FAIL;
}

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    esp_rmaker_ota_t *ota = (esp_rmaker_ota_t *)arg;
    esp_event_handler_unregister(RMAKER_COMMON_EVENT, RMAKER_MQTT_EVENT_CONNECTED, &event_handler);
    esp_rmaker_ota_report_status((esp_rmaker_ota_handle_t )ota, OTA_STATUS_SUCCESS, "OTA Upgrade finished and verified successfully");
    esp_ota_mark_app_valid_cancel_rollback();
    ota->ota_in_progress = false;
    if (s_ota_rollback_timer) {
        xTimerStop(s_ota_rollback_timer, portMAX_DELAY);
        xTimerDelete(s_ota_rollback_timer, portMAX_DELAY);
        s_ota_rollback_timer = NULL;
    }
    if (ota->type == OTA_USING_TOPICS) {
        if (esp_rmaker_ota_fetch_with_delay(RMAKER_OTA_FETCH_DELAY) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to create OTA Fetch timer.");
        }
    }
}

static void esp_ota_rollback(TimerHandle_t handle)
{
    ESP_LOGE(TAG, "Could not verify firmware even after %d seconds since boot-up. Rolling back.",
            RMAKER_OTA_ROLLBACK_WAIT_PERIOD);
    esp_ota_mark_app_invalid_rollback_and_reboot();
}

static esp_err_t esp_ota_check_for_mqtt(esp_rmaker_ota_t *ota)
{
    s_ota_rollback_timer = xTimerCreate("ota_rollback_tm", (RMAKER_OTA_ROLLBACK_WAIT_PERIOD * 1000) / portTICK_PERIOD_MS,
                            pdTRUE, NULL, esp_ota_rollback);
    if (s_ota_rollback_timer) {
        xTimerStart(s_ota_rollback_timer, 0);
    } else {
        ESP_LOGW(TAG, "Could not create rollback timer. Will require manual reboot if firmware verification fails");
    }

    return esp_event_handler_register(RMAKER_COMMON_EVENT, RMAKER_MQTT_EVENT_CONNECTED, &event_handler, ota);
}

static void esp_rmaker_ota_manage_rollback(esp_rmaker_ota_config_t *ota_config, esp_rmaker_ota_t *ota)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        ESP_LOGI(TAG, "OTA state = %d", ota_state);
        /* Not checking for CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE here because the firmware may have
         * it disabled, but bootloader may have it enabled, in which case, we will have to
         * handle this state.
         */
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "First Boot after an OTA");
            /* Run diagnostic function */
            bool diagnostic_is_ok = true;
            if (ota_config->ota_diag) {
                diagnostic_is_ok = ota_config->ota_diag();
            }
            if (diagnostic_is_ok) {
                ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution ...");
                /* Will not mark the image valid here immediately, but instead will wait for
                 * MQTT connection. The below flag will tell the OTA functions that the earlier
                 * OTA is still in progress.
                 */
                ota->ota_in_progress = true;
                esp_ota_check_for_mqtt(ota);
            } else {
                ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
        } else {
            /* If rollback is enabled, and the ota update flag is found, it means that the firmware was rolled back
            */
            nvs_handle handle;
            esp_err_t err = nvs_open_from_partition(ESP_RMAKER_NVS_PART_NAME, RMAKER_OTA_NVS_NAMESPACE, NVS_READWRITE, &handle);
            if (err == ESP_OK) {
                uint8_t ota_update = 0;
                size_t len = sizeof(ota_update);
                if ((err = nvs_get_blob(handle, RMAKER_OTA_UPDATE_FLAG_NVS_NAME, &ota_update, &len)) == ESP_OK) {
                    ota->rolled_back = true;
                    nvs_erase_key(handle, RMAKER_OTA_UPDATE_FLAG_NVS_NAME);
                    if (ota->type == OTA_USING_PARAMS) {
                        /* Calling this only for OTA_USING_PARAMS, because for OTA_USING_TOPICS,
                         * the work queue function will manage the status reporting later.
                         */
                        esp_rmaker_ota_report_status((esp_rmaker_ota_handle_t )ota,
                                OTA_STATUS_REJECTED, "Firmware rolled back");
                    }
                }
                nvs_close(handle);
            }
#endif
        }
    }
}

static const esp_rmaker_ota_config_t ota_default_config = {
    .server_cert = esp_rmaker_ota_def_cert,
};
/* Enable the ESP RainMaker specific OTA */
esp_err_t esp_rmaker_ota_enable(esp_rmaker_ota_config_t *ota_config, esp_rmaker_ota_type_t type)
{
    if (ota_config == NULL) {
        ota_config = (esp_rmaker_ota_config_t *)&ota_default_config;
    }
    if ((type != OTA_USING_PARAMS) && (type != OTA_USING_TOPICS)) {
        ESP_LOGE(TAG,"Invalid arguments for esp_rmaker_ota_enable()");
        return ESP_ERR_INVALID_ARG;
    }
    static bool ota_init_done;
    if (ota_init_done) {
        ESP_LOGE(TAG, "OTA already initialised");
        return ESP_FAIL;
    }
    esp_rmaker_ota_t *ota = MEM_CALLOC_EXTRAM(1, sizeof(esp_rmaker_ota_t));
    if (!ota) {
        ESP_LOGE(TAG, "Failed to allocate memory for esp_rmaker_ota_t");
        return ESP_ERR_NO_MEM;
    }
    if (ota_config->ota_cb) {
        ota->ota_cb = ota_config->ota_cb;
    } else {
        ota->ota_cb = esp_rmaker_ota_default_cb;
    }
    ota->priv = ota_config->priv;
    ota->server_cert = ota_config->server_cert;
    esp_err_t err = ESP_FAIL;
    ota->type = type;
    if (type == OTA_USING_PARAMS) {
        err = esp_rmaker_ota_enable_using_params(ota);
    } else if (type == OTA_USING_TOPICS) {
        err = esp_rmaker_ota_enable_using_topics(ota);
    }
    if (err == ESP_OK) {
        esp_rmaker_ota_manage_rollback(ota_config, ota);
        ota_init_done = true;
    } else {
        free(ota);
        ESP_LOGE(TAG, "Failed to enable OTA");
    }
#ifdef CONFIG_ESP_RMAKER_OTA_TIME_SUPPORT
    esp_rmaker_time_sync_init(NULL);
#endif
    return err;
}

esp_err_t esp_rmaker_ota_enable_default(void)
{
    return esp_rmaker_ota_enable(NULL, OTA_USING_TOPICS);
}
