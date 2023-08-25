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

#include <time.h>
#include <string.h>
#include <inttypes.h>
#include <esp_log.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <json_parser.h>
#include <esp_rmaker_work_queue.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_utils.h>
#include <esp_rmaker_internal.h>
#include <esp_rmaker_utils.h>
#include <esp_rmaker_standard_services.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_schedule.h>
#include <esp_schedule.h>

#define MAX_ID_LEN 8
#define MAX_NAME_LEN 32
#define MAX_INFO_LEN 128
#define MAX_OPERATION_LEN 10
#define TIME_SYNC_DELAY 10          /* 10 seconds */
#define MAX_SCHEDULES CONFIG_ESP_RMAKER_SCHEDULING_MAX_SCHEDULES

static const char *TAG = "esp_rmaker_schedule";

typedef enum trigger_type {
    TRIGGER_TYPE_INVALID = 0,
    TRIGGER_TYPE_DAYS_OF_WEEK,
    TRIGGER_TYPE_DATE,
    TRIGGER_TYPE_RELATIVE,
} trigger_type_t;

typedef struct esp_rmaker_schedule_trigger {
    trigger_type_t type;
    /* Relative Seconds */
    int relative_seconds;
    /* Minutes from 12am */
    uint16_t minutes;
    struct {
        /* 'OR' list of days or the week. Eg. Monday = 0b1, Tuesday = 0b10 */
        uint8_t repeat_days;
    } day;
    struct {
        /* Day of the month */
        uint8_t day;
        /* 'OR' list of months of the year. Eg. January = 0b1, February = 0b10.
        0 for next date (either this month or next). */
        uint16_t repeat_months;
        uint16_t year;
        bool repeat_every_year;
    } date;
    /* Used for non repeating schedules */
    int64_t next_timestamp;
} esp_rmaker_schedule_trigger_t;

typedef struct esp_rmaker_schedule_action {
    void *data;
    size_t data_len;
} esp_rmaker_schedule_action_t;

typedef struct esp_rmaker_schedule {
    char name[MAX_NAME_LEN + 1];        /* +1 for NULL termination */
    char id[MAX_ID_LEN + 1];            /* +1 for NULL termination */
    /* Info is used to store additional information, it is limited to 128 bytes. */
    char *info;
    /* Index is used in the callback to get back the schedule. */
    int32_t index;
    /* Flags are used to identify the schedule. Eg. timing, countdown */
    uint32_t flags;
    bool enabled;
    esp_schedule_handle_t handle;
    esp_rmaker_schedule_action_t action;
    esp_rmaker_schedule_trigger_t trigger;
    struct esp_rmaker_schedule *next;
} esp_rmaker_schedule_t;

enum time_sync_state {
    TIME_SYNC_NOT_STARTED,
    TIME_SYNC_STARTED,
    TIME_SYNC_DONE,
};

typedef enum schedule_operation {
    OPERATION_INVALID,
    OPERATION_ADD,
    OPERATION_EDIT,
    OPERATION_REMOVE,
    OPERATION_ENABLE,
    OPERATION_DISABLE,
} schedule_operation_t;

typedef struct {
    esp_rmaker_schedule_t *schedule_list;
    int total_schedules;
    /* This index just increases. This makes sure it is unique for the given schedules */
    int32_t index;
    esp_rmaker_device_t *schedule_service;
    TimerHandle_t time_sync_timer;
    enum time_sync_state time_sync_state;;
} esp_rmaker_schedule_priv_data_t;

static esp_rmaker_schedule_priv_data_t *schedule_priv_data;

static esp_err_t esp_rmaker_schedule_operation_enable(esp_rmaker_schedule_t *schedule);
static esp_err_t esp_rmaker_schedule_operation_disable(esp_rmaker_schedule_t *schedule);
static esp_err_t esp_rmaker_schedule_report_params(void);
static esp_err_t esp_rmaker_schedule_timesync_timer_deinit(void);
static esp_err_t esp_rmaker_schedule_timesync_timer_start(void);

static void esp_rmaker_schedule_free(esp_rmaker_schedule_t *schedule)
{
    if (!schedule) {
        return;
    }
    if (schedule->action.data) {
        free(schedule->action.data);
    }
    if (schedule->info) {
        free(schedule->info);
    }
    free(schedule);
}

static esp_rmaker_schedule_t *esp_rmaker_schedule_get_schedule_from_id(const char *id)
{
    if (!id) {
        return NULL;
    }
    esp_rmaker_schedule_t *schedule = schedule_priv_data->schedule_list;
    while(schedule) {
        if (strncmp(id, schedule->id, sizeof(schedule->id)) == 0) {
            ESP_LOGD(TAG, "Schedule with id %s found in list for get.", id);
            return schedule;
        }
        schedule = schedule->next;
    }
    ESP_LOGD(TAG, "Schedule with id %s not found in list for get.", id);
    return NULL;
}

static esp_rmaker_schedule_t *esp_rmaker_schedule_get_schedule_from_index(int32_t index)
{
    esp_rmaker_schedule_t *schedule = schedule_priv_data->schedule_list;
    while(schedule) {
        if (schedule->index == index) {
            ESP_LOGD(TAG, "Schedule with index %"PRIi32" found in list for get.", index);
            return schedule;
        }
        schedule = schedule->next;
    }
    ESP_LOGD(TAG, "Schedule with index %"PRIi32" not found in list for get.", index);
    return NULL;
}

static esp_err_t esp_rmaker_schedule_add_to_list(esp_rmaker_schedule_t *schedule)
{
    if (!schedule) {
        ESP_LOGE(TAG, "Schedule is NULL. Not adding to list.");
        return ESP_ERR_INVALID_ARG;
    }

    if (esp_rmaker_schedule_get_schedule_from_id(schedule->id) != NULL) {
        ESP_LOGI(TAG, "Schedule with id %s already added to list. Not adding again.", schedule->id);
        return ESP_FAIL;
    }
    /* Parse list */
    esp_rmaker_schedule_t *prev_schedule = schedule_priv_data->schedule_list;
    while(prev_schedule) {
        if (prev_schedule->next) {
            prev_schedule = prev_schedule->next;
        } else {
            break;
        }
    }

    /* Add to list */
    if (prev_schedule) {
        prev_schedule->next = schedule;
    } else {
        schedule_priv_data->schedule_list = schedule;
    }
    ESP_LOGD(TAG, "Schedule with id %s added to list.", schedule->id);
    schedule_priv_data->total_schedules++;
    return ESP_OK;
}

static esp_err_t esp_rmaker_schedule_remove_from_list(esp_rmaker_schedule_t *schedule)
{
    if (!schedule) {
        ESP_LOGE(TAG, "Schedule is NULL. Not removing from list.");
        return ESP_ERR_INVALID_ARG;
    }
    /* Parse list */
    esp_rmaker_schedule_t *curr_schedule = schedule_priv_data->schedule_list;
    esp_rmaker_schedule_t *prev_schedule = curr_schedule;
    while(curr_schedule) {
        if (strncmp(schedule->id, curr_schedule->id, sizeof(schedule->id)) == 0) {
            ESP_LOGD(TAG, "Schedule with id %s found in list for removing", schedule->id);
            break;
        }
        prev_schedule = curr_schedule;
        curr_schedule = curr_schedule->next;
    }
    if (!curr_schedule) {
        ESP_LOGE(TAG, "Schedule with id %s not found in list. Not removing.", schedule->id);
        return ESP_ERR_NOT_FOUND;
    }

    /* Remove from list */
    if (curr_schedule == schedule_priv_data->schedule_list) {
        schedule_priv_data->schedule_list = curr_schedule->next;
    } else {
        prev_schedule->next = curr_schedule->next;
    }
    schedule_priv_data->total_schedules--;
    ESP_LOGD(TAG, "Schedule with id %s removed from list.", schedule->id);
    return ESP_OK;
}

static bool esp_rmaker_schedule_is_expired(esp_rmaker_schedule_t *schedule)
{
    time_t current_timestamp = 0;
    struct tm current_time = {0};
    time(&current_timestamp);
    localtime_r(&current_timestamp, &current_time);

    if (schedule->trigger.type == TRIGGER_TYPE_RELATIVE) {
        if (schedule->trigger.next_timestamp > 0 && schedule->trigger.next_timestamp <= current_timestamp) {
            /* Relative seconds based schedule has expired */
            return true;
        }
    } else if (schedule->trigger.type == TRIGGER_TYPE_DAYS_OF_WEEK) {
        if (schedule->trigger.day.repeat_days == 0) {
            if (schedule->trigger.next_timestamp > 0 && schedule->trigger.next_timestamp <= current_timestamp) {
                /* One time schedule has expired */
                return true;
            }
        }
    } else if (schedule->trigger.type == TRIGGER_TYPE_DATE) {
        if (schedule->trigger.date.repeat_months == 0) {
            if (schedule->trigger.next_timestamp > 0 && schedule->trigger.next_timestamp <= current_timestamp) {
                /* One time schedule has expired */
                return true;
            } else {
                return false;
            }
        }
        if (schedule->trigger.date.repeat_every_year == true) {
            return false;
        }

        struct tm schedule_time = {0};
        localtime_r(&current_timestamp, &schedule_time);
        schedule_time.tm_sec = 0;
        schedule_time.tm_min = schedule->trigger.minutes;
        schedule_time.tm_hour = 0;
        schedule_time.tm_mday = schedule->trigger.date.day;
        /* For expiry, just check the last month of the repeat_months. */
        /* '-1' because struct tm has months starting from 0 and we have months starting from 1. */
        schedule_time.tm_mon = fls(schedule->trigger.date.repeat_months) - 1;
        /* '-1900' because struct tm has number of years after 1900 */
        schedule_time.tm_year = schedule->trigger.date.year - 1900;
        time_t schedule_timestamp = mktime(&schedule_time);

        if (schedule_timestamp < current_timestamp) {
            return true;
        }
    }
    return false;
}

static esp_err_t esp_rmaker_schedule_process_action(esp_rmaker_schedule_action_t *action)
{
    return esp_rmaker_handle_set_params(action->data, action->data_len, ESP_RMAKER_REQ_SRC_SCHEDULE);
}

static void esp_rmaker_schedule_trigger_work_cb(void *priv_data)
{
    int32_t index = (int32_t)priv_data;
    esp_rmaker_schedule_t *schedule = esp_rmaker_schedule_get_schedule_from_index(index);
    if (!schedule) {
        ESP_LOGE(TAG, "Schedule with index %"PRIi32" not found for trigger work callback", index);
        return;
    }
    esp_rmaker_schedule_process_action(&schedule->action);
    if (esp_rmaker_schedule_is_expired(schedule))  {
        /* This schedule does not repeat anymore. Disable it and report the params. */
        esp_rmaker_schedule_operation_disable(schedule);
        esp_rmaker_schedule_report_params();
    }
}

static void esp_rmaker_schedule_trigger_common_cb(esp_schedule_handle_t handle, void *priv_data)
{
    /* Adding to work queue to change the context from timer's task. */
    esp_rmaker_work_queue_add_task(esp_rmaker_schedule_trigger_work_cb, priv_data);
}

static void esp_rmaker_schedule_timestamp_common_cb(esp_schedule_handle_t handle, uint32_t next_timestamp, void *priv_data)
{
    int32_t index = (int32_t)priv_data;
    esp_rmaker_schedule_t *schedule = esp_rmaker_schedule_get_schedule_from_index(index);
    if (!schedule) {
        ESP_LOGE(TAG, "Schedule with index %"PRIi32" not found for timestamp callback", index);
        return;
    }
    schedule->trigger.next_timestamp = next_timestamp;
}

static esp_err_t esp_rmaker_schedule_prepare_config(esp_rmaker_schedule_t *schedule, esp_schedule_config_t *schedule_config)
{
    if (!schedule || !schedule_config) {
        ESP_LOGE(TAG, "schedule or schedule_config is NULL.");
        return ESP_ERR_INVALID_ARG;
    }

    if (schedule->trigger.type == TRIGGER_TYPE_RELATIVE) {
        schedule_config->trigger.next_scheduled_time_utc = (time_t)schedule->trigger.next_timestamp;
        schedule_config->trigger.relative_seconds = schedule->trigger.relative_seconds;
        schedule_config->trigger.type = ESP_SCHEDULE_TYPE_RELATIVE;
        schedule_config->trigger_cb = esp_rmaker_schedule_trigger_common_cb;
        schedule_config->timestamp_cb = esp_rmaker_schedule_timestamp_common_cb;
    } else {
        int hours = schedule->trigger.minutes / 60;
        int minutes = schedule->trigger.minutes % 60;
        schedule_config->trigger.hours = hours;
        schedule_config->trigger.minutes = minutes;
        schedule_config->trigger_cb = esp_rmaker_schedule_trigger_common_cb;

        if (schedule->trigger.type == TRIGGER_TYPE_DAYS_OF_WEEK) {
            schedule_config->trigger.type = ESP_SCHEDULE_TYPE_DAYS_OF_WEEK;
            schedule_config->trigger.day.repeat_days = schedule->trigger.day.repeat_days;
            if (schedule->trigger.day.repeat_days == 0) {
                schedule_config->timestamp_cb = esp_rmaker_schedule_timestamp_common_cb;
            }
        } else if (schedule->trigger.type == TRIGGER_TYPE_DATE) {
            schedule_config->trigger.type = ESP_SCHEDULE_TYPE_DATE;
            schedule_config->trigger.date.day = schedule->trigger.date.day;
            schedule_config->trigger.date.repeat_months = schedule->trigger.date.repeat_months;
            schedule_config->trigger.date.year = schedule->trigger.date.year;
            schedule_config->trigger.date.repeat_every_year = schedule->trigger.date.repeat_every_year;
            if (schedule->trigger.date.repeat_months == 0) {
                schedule_config->timestamp_cb = esp_rmaker_schedule_timestamp_common_cb;
            }
        }
    }

    /* In esp_schedule, name should be unique and is used as the primary key.
    We are setting the id in esp_rmaker_schedule as the name in esp_schedule */
    strlcpy(schedule_config->name, schedule->id, sizeof(schedule_config->name));
    /* Just passing the schedule pointer as priv_data could create a race condition between the schedule getting a
    callback and the schedule getting removed. Using this unique index as the priv_data solves it to some extent. */
    schedule_config->priv_data = (void *)schedule->index;
    return ESP_OK;
}

static esp_err_t esp_rmaker_schedule_add(esp_rmaker_schedule_t *schedule)
{
    esp_schedule_config_t schedule_config = {0};
    esp_rmaker_schedule_prepare_config(schedule, &schedule_config);

    schedule->handle = esp_schedule_create(&schedule_config);
    if (schedule->handle == NULL) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t esp_rmaker_schedule_operation_add(esp_rmaker_schedule_t *schedule)
{
    esp_err_t ret = esp_rmaker_schedule_add(schedule);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = esp_rmaker_schedule_add_to_list(schedule);
    return ret;
}

static esp_err_t esp_rmaker_schedule_operation_edit(esp_rmaker_schedule_t *schedule)
{
    esp_schedule_config_t schedule_config = {0};
    esp_rmaker_schedule_prepare_config(schedule, &schedule_config);

    esp_err_t ret = esp_schedule_edit(schedule->handle, &schedule_config);
    if (schedule->enabled == true) {
        /* If the schedule is already enabled, disable it and enable it again so that the new changes after the
        edit are reflected. */
        esp_rmaker_schedule_operation_disable(schedule);
        esp_rmaker_schedule_operation_enable(schedule);
    }
    return ret;
}

static esp_err_t esp_rmaker_schedule_remove(esp_rmaker_schedule_t *schedule)
{
    return esp_schedule_delete(schedule->handle);
}

static esp_err_t esp_rmaker_schedule_operation_remove(esp_rmaker_schedule_t *schedule)
{
    esp_err_t ret = esp_rmaker_schedule_remove_from_list(schedule);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = esp_rmaker_schedule_remove(schedule);
    return ret;
}

static void esp_rmaker_schedule_timesync_timer_work_cb(void *priv_data)
{
    if (esp_rmaker_time_check() != true) {
        esp_rmaker_schedule_timesync_timer_start();
        return;
    }
    esp_rmaker_schedule_timesync_timer_deinit();
    schedule_priv_data->time_sync_state = TIME_SYNC_DONE;
    ESP_LOGI(TAG, "Time is synchronised now. Enabling the schedules.");
    esp_rmaker_schedule_t *schedule = schedule_priv_data->schedule_list;
    while (schedule) {
        if (schedule->enabled == true) {
            esp_rmaker_schedule_operation_enable(schedule);
        }
        schedule = schedule->next;
    }
}

static void esp_rmaker_schedule_timesync_timer_cb(TimerHandle_t timer)
{
    esp_rmaker_work_queue_add_task(esp_rmaker_schedule_timesync_timer_work_cb, NULL);
}

static esp_err_t esp_rmaker_schedule_timesync_timer_init(void)
{
    schedule_priv_data->time_sync_timer = xTimerCreate("esp_rmaker_schedule", (TIME_SYNC_DELAY * 1000) / portTICK_PERIOD_MS, pdFALSE, NULL, esp_rmaker_schedule_timesync_timer_cb);
    if (schedule_priv_data->time_sync_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create timer for time sync");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t esp_rmaker_schedule_timesync_timer_deinit(void)
{
    xTimerDelete(schedule_priv_data->time_sync_timer, portMAX_DELAY);
    schedule_priv_data->time_sync_timer = NULL;
    return ESP_OK;
}

static esp_err_t esp_rmaker_schedule_timesync_timer_start(void)
{
    xTimerStart(schedule_priv_data->time_sync_timer, portMAX_DELAY);
    return ESP_OK;
}

static esp_err_t esp_rmaker_schedule_operation_enable(esp_rmaker_schedule_t *schedule)
{
    /* Setting enabled to true even if time is not synced yet. This reports the correct enabled state when reporting the schedules.*/
    schedule->enabled = true;

    /* Check for time sync */
    if (schedule_priv_data->time_sync_state == TIME_SYNC_NOT_STARTED) {
        if (esp_rmaker_time_check() != true) {
            ESP_LOGI(TAG, "Time is not synchronised yet. The schedule will actually be enabled when time is synchronised. This may take time.");
            esp_rmaker_schedule_timesync_timer_init();
            esp_rmaker_schedule_timesync_timer_start();
            schedule_priv_data->time_sync_state = TIME_SYNC_STARTED;
            return ESP_FAIL;
        }
        schedule_priv_data->time_sync_state = TIME_SYNC_DONE;
    } else if (schedule_priv_data->time_sync_state == TIME_SYNC_STARTED) {
        return ESP_FAIL;
    }

    if (esp_rmaker_schedule_is_expired(schedule)) {
        /* This schedule does not repeat anymore. Disable it. */
        /* While time sync is happening, it might be possible that this schedule will be shown as enabled, but actually it is disabled. */
        ESP_LOGI(TAG, "Schedule with id %s does not repeat anymore. Disabling it.", schedule->id);
        esp_rmaker_schedule_operation_disable(schedule);
        /* Since the enabled state has been changed, report this */
        esp_rmaker_schedule_report_params();
        return ESP_OK;
    }

    /* Time is synced. Enable the schedule */
    return esp_schedule_enable(schedule->handle);
}

static esp_err_t esp_rmaker_schedule_operation_disable(esp_rmaker_schedule_t *schedule)
{
    esp_err_t ret = esp_schedule_disable(schedule->handle);
    schedule->trigger.next_timestamp = 0;
    schedule->enabled = false;
    return ret;
}

schedule_operation_t esp_rmaker_schedule_get_operation_from_str(char *operation)
{
    if (!operation) {
        return OPERATION_INVALID;
    }
    if (strncmp(operation, "add", strlen(operation)) == 0) {
        return OPERATION_ADD;
    } else if (strncmp(operation, "edit", strlen(operation)) == 0) {
        return OPERATION_EDIT;
    } else if (strncmp(operation, "remove", strlen(operation)) == 0) {
        return OPERATION_REMOVE;
    } else if (strncmp(operation, "enable", strlen(operation)) == 0) {
        return OPERATION_ENABLE;
    } else if (strncmp(operation, "disable", strlen(operation)) == 0) {
        return OPERATION_DISABLE;
    }
    return OPERATION_INVALID;
}

static schedule_operation_t esp_rmaker_schedule_parse_operation(jparse_ctx_t *jctx, char *id)
{
    char operation_str[MAX_OPERATION_LEN + 1] = {0};        /* +1 for NULL termination */
    schedule_operation_t operation = OPERATION_INVALID;
    json_obj_get_string(jctx, "operation", operation_str, sizeof(operation_str));
    if (strlen(operation_str) <= 0) {
        ESP_LOGE(TAG, "Operation not found in schedule with id: %s", id);
        return operation;
    }
    operation = esp_rmaker_schedule_get_operation_from_str(operation_str);
    if (operation == OPERATION_EDIT) {
        /* Get schedule temporarily */
        if (esp_rmaker_schedule_get_schedule_from_id(id) == NULL) {
            /* Operation is edit, but schedule not present already. Consider this as add. */
            ESP_LOGD(TAG, "Operation is edit, but schedule with id %s not found. Changing the operation to add.", id);
            operation = OPERATION_ADD;
        }
    } else if (operation == OPERATION_INVALID) {
        ESP_LOGE(TAG, "Invalid schedule operation found: %s", operation_str);
    }
    return operation;
}

static esp_err_t esp_rmaker_schedule_parse_action(jparse_ctx_t *jctx, esp_rmaker_schedule_action_t *action)
{
    int data_len = 0;
    json_obj_get_object_strlen(jctx, "action", &data_len);
    if (data_len <= 0) {
        ESP_LOGD(TAG, "Action not found in JSON");
        return ESP_OK;
    }
    action->data_len = data_len + 1;

    if (action->data) {
        free(action->data);
    }
    action->data = (void *)MEM_CALLOC_EXTRAM(1, action->data_len);
    if (!action->data) {
        ESP_LOGE(TAG, "Could not allocate action");
        return ESP_ERR_NO_MEM;
    }
    json_obj_get_object_str(jctx, "action", action->data, action->data_len);
    return ESP_OK;
}

static esp_err_t esp_rmaker_schedule_parse_trigger(jparse_ctx_t *jctx, esp_rmaker_schedule_trigger_t *trigger)
{
    int total_triggers = 0;
    int relative_seconds = 0, minutes = 0, repeat_days = 0, day = 0, repeat_months = 0, year = 0;
    bool repeat_every_year = false;
    int64_t timestamp = 0;
    trigger_type_t type = TRIGGER_TYPE_INVALID;
    if(json_obj_get_array(jctx, "triggers", &total_triggers) != 0) {
        ESP_LOGD(TAG, "Trigger not found in JSON");
        return ESP_OK;
    }
    if (total_triggers <= 0) {
        ESP_LOGD(TAG, "No triggers found in trigger array");
        json_obj_leave_array(jctx);
        return ESP_OK;
    }
    if(json_arr_get_object(jctx, 0) == 0) {
        json_obj_get_int64(jctx, "ts", &timestamp);
        if (json_obj_get_int(jctx, "rsec", &relative_seconds) == 0) {
            type = TRIGGER_TYPE_RELATIVE;
        } else {
            json_obj_get_int(jctx, "m", &minutes);
            /* Check if it is of type day */
            if (json_obj_get_int(jctx, "d", &repeat_days) == 0) {
                type = TRIGGER_TYPE_DAYS_OF_WEEK;
            }
            if (json_obj_get_int(jctx, "dd", &day) == 0) {
                type = TRIGGER_TYPE_DATE;
                json_obj_get_int(jctx, "mm", &repeat_months);
                json_obj_get_int(jctx, "yy", &year);
                json_obj_get_bool(jctx, "r", &repeat_every_year);
            }
        }
        json_arr_leave_object(jctx);
    }
    json_obj_leave_array(jctx);

    trigger->type = type;
    trigger->relative_seconds = relative_seconds;
    trigger->minutes = minutes;
    trigger->day.repeat_days = repeat_days;
    trigger->date.day = day;
    trigger->date.repeat_months = repeat_months;
    trigger->date.year = year;
    trigger->date.repeat_every_year = repeat_every_year;
    trigger->next_timestamp = timestamp;
    return ESP_OK;
}

static esp_err_t esp_rmaker_schedule_parse_info_and_flags(jparse_ctx_t *jctx, char **info, uint32_t *flags)
{
    char _info[MAX_INFO_LEN + 1] = {0};  /* +1 for NULL termination */
    int _flags = 0;

    int err_code = json_obj_get_string(jctx, "info", _info, sizeof(_info));
    if (err_code == OS_SUCCESS) {
        if (*info) {
            free(*info);
            *info = NULL;
        }

        if (strlen(_info) > 0) {
            /* +1 for NULL termination */
            *info = (char *)MEM_CALLOC_EXTRAM(1, strlen(_info) + 1);
            if (*info) {
                strncpy(*info, _info, strlen(_info));
            }
        }
    }

    err_code = json_obj_get_int(jctx, "flags", &_flags);
    if (err_code == OS_SUCCESS) {
        if (flags) {
            *flags = _flags;
        }
    }

    return ESP_OK;
}

static esp_rmaker_schedule_t *esp_rmaker_schedule_find_or_create(jparse_ctx_t *jctx, char *id, schedule_operation_t operation)
{
    char name[MAX_NAME_LEN + 1] = {0};      /* +1 for NULL termination */
    esp_rmaker_schedule_t *schedule = NULL;
    if (operation == OPERATION_ADD) {
        /* Checking if schedule with same id already exists. */
        schedule = esp_rmaker_schedule_get_schedule_from_id(id);
        if (schedule) {
            ESP_LOGE(TAG, "Schedule with id %s already exists. Not adding it again.", id);
            return NULL;
        }

        /* Get name */
        json_obj_get_string(jctx, "name", name, sizeof(name));
        if (strlen(name) <= 0) {
            ESP_LOGE(TAG, "Name not found for schedule with id: %s", id);
            return NULL;
        }

        /* This is a new schedule. Fill it. */
        schedule = (esp_rmaker_schedule_t *)MEM_CALLOC_EXTRAM(1, sizeof(esp_rmaker_schedule_t));
        if (!schedule) {
            ESP_LOGE(TAG, "Couldn't allocate schedule with id: %s", id);
            return NULL;
        }
        strlcpy(schedule->id, id, sizeof(schedule->id));
        strlcpy(schedule->name, name, sizeof(schedule->name));
        schedule->index = schedule_priv_data->index++;
    } else {
        /* This schedule should already be present */
        schedule = esp_rmaker_schedule_get_schedule_from_id(id);
        if (!schedule) {
            ESP_LOGE(TAG, "Schedule with id %s not found", id);
            return NULL;
        }

        /* Get name */
        if (operation == OPERATION_EDIT) {
            json_obj_get_string(jctx, "name", name, sizeof(name));
            if (strlen(name) > 0) {
                /* If there is name in the request, replace the name in the schedule with this new one */
                memset(schedule->name, 0, sizeof(schedule->name));
                strlcpy(schedule->name, name, sizeof(schedule->name));
            }
        }
    }
    return schedule;
}

static esp_err_t esp_rmaker_schedule_perform_operation(esp_rmaker_schedule_t *schedule, schedule_operation_t operation, bool enabled)
{
    esp_err_t err = ESP_OK;
    switch (operation) {
        case OPERATION_ADD:
            if (schedule_priv_data->total_schedules < MAX_SCHEDULES) {
                esp_rmaker_schedule_operation_add(schedule);
                if (enabled == true) {
                    esp_rmaker_schedule_operation_enable(schedule);
                }
            } else {
                ESP_LOGE(TAG, "Max schedules (%d) reached. Not adding this schedule with id %s", MAX_SCHEDULES,
                        schedule->id);
                err = ESP_FAIL;
            }
            break;

        case OPERATION_EDIT:
            esp_rmaker_schedule_operation_edit(schedule);
            break;

        case OPERATION_REMOVE:
            esp_rmaker_schedule_operation_remove(schedule);
            esp_rmaker_schedule_free(schedule);
            break;

        case OPERATION_ENABLE:
            esp_rmaker_schedule_operation_enable(schedule);
            break;

        case OPERATION_DISABLE:
            esp_rmaker_schedule_operation_disable(schedule);
            break;

        default:
            ESP_LOGE(TAG, "Invalid Operation: %d", operation);
            err = ESP_FAIL;
            break;
    }
    return err;
}

static esp_err_t esp_rmaker_schedule_parse_json(void *data, size_t data_len, esp_rmaker_req_src_t src)
{
    char id[MAX_ID_LEN + 1] = {0};      /* +1 for NULL termination */
    schedule_operation_t operation = OPERATION_INVALID;
    bool enabled = true;
    int current_schedule = 0;
    esp_rmaker_schedule_t *schedule = NULL;

    /* Get details from JSON */
    jparse_ctx_t jctx;
    if (json_parse_start(&jctx, (char *)data, data_len) != 0) {
        ESP_LOGE(TAG, "Json parse start failed");
        return ESP_FAIL;
    }

    /* Parse all schedules */
    while(json_arr_get_object(&jctx, current_schedule) == 0) {
        /* Get ID */
        json_obj_get_string(&jctx, "id", id, sizeof(id));
        if (strlen(id) <= 0) {
            ESP_LOGE(TAG, "ID not found in schedule JSON");
            goto cleanup;
        }

        /* Get operation */
        if (src == ESP_RMAKER_REQ_SRC_INIT) {
            /* Schedule loaded from NVS. Add it */
            operation = OPERATION_ADD;
        } else {
            operation = esp_rmaker_schedule_parse_operation(&jctx, id);
            if (operation == OPERATION_INVALID) {
                ESP_LOGE(TAG, "Error getting operation");
                goto cleanup;
            }
        }

        /* Find/Create new schedule */
        schedule = esp_rmaker_schedule_find_or_create(&jctx, id, operation);
        if (!schedule) {
            goto cleanup;
        }

        /* Get other schedule details */
        if (operation == OPERATION_ADD || operation == OPERATION_EDIT) {
            /* Get enabled state */
            if (operation == OPERATION_ADD) {
                /* If loaded from NVS, check for previous enabled state. If new schedule, enable it */
                if (src == ESP_RMAKER_REQ_SRC_INIT) {
                    json_obj_get_bool(&jctx, "enabled", &enabled);
                } else {
                    enabled = true;
                }
            }

            /* Get action */
            esp_rmaker_schedule_parse_action(&jctx, &schedule->action);

            /* Get trigger */
            /* There is only one trigger for now. If more triggers are added, then they should be parsed here in a loop */
            esp_rmaker_schedule_parse_trigger(&jctx, &schedule->trigger);

            /* Get info and flags */
            esp_rmaker_schedule_parse_info_and_flags(&jctx, &schedule->info, &schedule->flags);
        }

        /* Perform operation */
        esp_rmaker_schedule_perform_operation(schedule, operation, enabled);

cleanup:
        json_arr_leave_object(&jctx);
        current_schedule++;
    }
    json_parse_end(&jctx);
    return ESP_OK;
}

static esp_err_t __esp_rmaker_schedule_get_params(char *buf, size_t *buf_size)
{
    esp_err_t err = ESP_OK;
    esp_rmaker_schedule_t *schedule = schedule_priv_data->schedule_list;
    json_gen_str_t jstr;
    json_gen_str_start(&jstr, buf, *buf_size, NULL, NULL);
    json_gen_start_array(&jstr);
    while (schedule) {
        json_gen_start_object(&jstr);

        /* Add details */
        json_gen_obj_set_string(&jstr, "name", schedule->name);
        json_gen_obj_set_string(&jstr, "id", schedule->id);
        json_gen_obj_set_bool(&jstr, "enabled", schedule->enabled);
        /* If info and flags is not zero, add it. */
        if (schedule->info != NULL) {
            json_gen_obj_set_string(&jstr, "info", schedule->info);
        }
        if (schedule->flags != 0) {
            json_gen_obj_set_int(&jstr, "flags", schedule->flags);
        }

        /* Add action */
        json_gen_push_object_str(&jstr, "action", schedule->action.data);

        /* Add trigger */
        json_gen_push_array(&jstr, "triggers");
        json_gen_start_object(&jstr);
        if (schedule->trigger.type == TRIGGER_TYPE_RELATIVE) {
            json_gen_obj_set_int(&jstr, "rsec", schedule->trigger.relative_seconds);
            json_gen_obj_set_int(&jstr, "ts", schedule->trigger.next_timestamp);
        } else {
            json_gen_obj_set_int(&jstr, "m", schedule->trigger.minutes);
            if (schedule->trigger.type == TRIGGER_TYPE_DAYS_OF_WEEK) {
                json_gen_obj_set_int(&jstr, "d", schedule->trigger.day.repeat_days);
                if (schedule->trigger.day.repeat_days == 0) {
                    json_gen_obj_set_int(&jstr, "ts", schedule->trigger.next_timestamp);
                }
            } else if (schedule->trigger.type == TRIGGER_TYPE_DATE) {
                json_gen_obj_set_int(&jstr, "dd", schedule->trigger.date.day);
                json_gen_obj_set_int(&jstr, "mm", schedule->trigger.date.repeat_months);
                json_gen_obj_set_int(&jstr, "yy", schedule->trigger.date.year);
                json_gen_obj_set_int(&jstr, "r", schedule->trigger.date.repeat_every_year);
                if (schedule->trigger.date.repeat_months == 0) {
                    json_gen_obj_set_int(&jstr, "ts", schedule->trigger.next_timestamp);
                }
            }
        }
        json_gen_end_object(&jstr);
        json_gen_pop_array(&jstr);

        json_gen_end_object(&jstr);

        /* Go to next schedule */
        schedule = schedule->next;
    }
    if (json_gen_end_array(&jstr) < 0) {
        ESP_LOGE(TAG, "Buffer size %d not sufficient for reporting Schedule Params.", *buf_size);
        err = ESP_ERR_NO_MEM;
    }
    *buf_size = json_gen_str_end(&jstr);
    return err;
}

static char *esp_rmaker_schedule_get_params(void)
{
    size_t req_size = 0;
    esp_err_t err = __esp_rmaker_schedule_get_params(NULL, &req_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get required size for schedules JSON.");
        return NULL;
    }
    char *data = MEM_CALLOC_EXTRAM(1, req_size);
    if (!data) {
        ESP_LOGE(TAG, "Failed to allocate %d bytes for schedule.", req_size);
        return NULL;
    }
    err = __esp_rmaker_schedule_get_params(data, &req_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error occured while trying to populate schedules JSON.");
        free(data);
        return NULL;
    }
    return data;
}

static esp_err_t esp_rmaker_schedule_report_params(void)
{
    char *data = esp_rmaker_schedule_get_params();
    esp_rmaker_param_val_t val = {
        .type = RMAKER_VAL_TYPE_ARRAY,
        .val.s = data,
    };
    esp_rmaker_param_t *param = esp_rmaker_device_get_param_by_type(schedule_priv_data->schedule_service, ESP_RMAKER_PARAM_SCHEDULES);
    esp_rmaker_param_update_and_report(param, val);

    free(data);
    return ESP_OK;
}

static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
            const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (strcmp(esp_rmaker_param_get_type(param), ESP_RMAKER_PARAM_SCHEDULES) != 0) {
        ESP_LOGE(TAG, "Got callback for invalid param with name %s and type %s", esp_rmaker_param_get_name(param), esp_rmaker_param_get_type(param));
        return ESP_ERR_INVALID_ARG;
    }
    if (strlen(val.val.s) <= 0) {
        ESP_LOGI(TAG, "Invalid length for params: %d", strlen(val.val.s));
        return ESP_ERR_INVALID_ARG;
    }
    esp_rmaker_schedule_parse_json(val.val.s, strlen(val.val.s), ctx->src);
    if (ctx->src != ESP_RMAKER_REQ_SRC_INIT) {
        /* Since this is a persisting param, we get a write_cb while booting up. We need not report the param when the source is 'init' as this will get reported when the device first reports all the params. */
        esp_rmaker_schedule_report_params();
    }
    return ESP_OK;
}

esp_err_t esp_rmaker_schedule_enable(void)
{
    schedule_priv_data = (esp_rmaker_schedule_priv_data_t *)MEM_CALLOC_EXTRAM(1, sizeof(esp_rmaker_schedule_priv_data_t));
    if (!schedule_priv_data) {
        ESP_LOGE(TAG, "Couldn't allocate schedule_priv_data");
        return ESP_ERR_NO_MEM;
    }
    esp_rmaker_time_sync_init(NULL);

    esp_schedule_init(false, NULL, NULL);

    schedule_priv_data->schedule_service = esp_rmaker_create_schedule_service("Schedule", write_cb, NULL, MAX_SCHEDULES, NULL);
    if (!schedule_priv_data->schedule_service) {
        ESP_LOGE(TAG, "Failed to create Schedule Service");
        return ESP_FAIL;
    }
    esp_err_t err = esp_rmaker_node_add_device(esp_rmaker_get_node(), schedule_priv_data->schedule_service);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add service Service");
        return err;
    }
    ESP_LOGD(TAG, "Scheduling Service Enabled");
    return err;
}
