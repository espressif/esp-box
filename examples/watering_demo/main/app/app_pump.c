/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdbool.h>
#include <stdio.h>

#include <sys/queue.h>

#include "freertos/FreeRTOS.h"
#include <freertos/queue.h>
#include <freertos/timers.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"

#include "driver/gpio.h"
#include "app_humidity.h"
#include "app_pump.h"
#include "app_nvs.h"

static const char *TAG = "app_pump";

enum {
    APP_PUMP_CMD_NA,
    APP_PUMP_CMD_STOP,
    APP_PUMP_CMD_START,
    APP_PUMP_CMD_MAX,
};

struct cb_entry {
    STAILQ_ENTRY(cb_entry) next;
    app_pump_cb_t cb;
    void *args;
};
typedef STAILQ_HEAD(cb_list_head, cb_entry) cb_list_t;

typedef struct {
    //pin
    gpio_num_t gpio_num;
    int        gpio_active_level;
    //configs
    int is_watering;
    int max_watering_time;
    int curr_watering_time; //in seconds

    // triggers
    bool auto_watering_en;
    int lower_humidity; // when humidity below this trigger watering automatically

    //cb
    cb_list_t cb_before;
    cb_list_t cb_during;
    cb_list_t cb_after;

    QueueHandle_t queue_handle;
    TaskHandle_t  task_handle;
    TimerHandle_t timer_handle;
} app_pump_t;

static app_pump_t _APP_PUMP;

static app_pump_t *pump_ref(void)
{
    return &_APP_PUMP;
}

static esp_err_t app_pump_drive_init(app_pump_t *ref)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL << ref->gpio_num;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    return gpio_config(&io_conf);
}

static void app_pump_drive_set(app_pump_t *ref, int onoff)
{
    gpio_set_level(ref->gpio_num, onoff == 0 ? !(ref->gpio_active_level) : ref->gpio_active_level);
}

/*handle auto watering*/
void app_pump_auto_watering(void *args)
{
    if (app_pump_get_auto_watering_enable() == 0) {
        return;
    }

    if (app_humidity_get_value() <= app_pump_get_lower_humidity()) {
        ESP_LOGW(TAG, "auto watering start");
        app_pump_watering_start();
    }
}

static void pump_timer_cb(TimerHandle_t tmr)
{
    app_pump_t *ref = (app_pump_t *) pvTimerGetTimerID(tmr);
    if (++ref->curr_watering_time < ref->max_watering_time) {
        struct cb_entry *entry;
        STAILQ_FOREACH(entry, &ref->cb_during, next) {
            entry->cb(entry->args);
        }
    } else {
        app_pump_watering_stop_isr();
    }
}

static void pump_task(void *pvParam)
{
    app_pump_t *ref = pvParam;
    ESP_ERROR_CHECK(app_nvs_get_watering_time(&ref->max_watering_time));
    ESP_ERROR_CHECK(app_nvs_get_auto_watering_enable(&ref->auto_watering_en));
    ESP_ERROR_CHECK(app_nvs_get_lower_humidity(&ref->lower_humidity));

    ref->queue_handle = xQueueCreate(1, sizeof(int));
    ESP_ERROR_CHECK(ref->queue_handle == NULL ? ESP_FAIL : ESP_OK);

    ref->timer_handle = xTimerCreate("watering timer",
                                     pdMS_TO_TICKS(1000),
                                     pdTRUE,
                                     ref,
                                     pump_timer_cb);
    ESP_ERROR_CHECK(ref->timer_handle == NULL ? ESP_FAIL : ESP_OK);

    /*gpio drive init*/
    ref->gpio_num = GPIO_NUM_41;
    ref->gpio_active_level = 0;
    ESP_ERROR_CHECK(app_pump_drive_init(ref));
    app_pump_drive_set(ref, 0);

    //app_humidity_add_watcher(app_pump_auto_watering, NULL);
    struct cb_entry *entry;

    for (;;) {
        int cmd = APP_PUMP_CMD_NA;
        if (xQueueReceive(ref->queue_handle, &cmd, portMAX_DELAY) == pdPASS) {
            switch (cmd) {
            case APP_PUMP_CMD_START: {
                if (ref->is_watering) {
                    ESP_LOGW(TAG, "alread in watering state");
                } else {
                    ref->is_watering = 1;
                    ref->curr_watering_time = 0;
                    xTimerStart(ref->timer_handle, 0);

                    STAILQ_FOREACH(entry, &ref->cb_before, next) {
                        entry->cb(entry->args);
                    }
                    app_pump_drive_set(ref, 1);
                }
            }
            break;
            case APP_PUMP_CMD_STOP: {
                if (ref->is_watering) {
                    ESP_LOGW(TAG, "stop watering (%d/%d)", ref->curr_watering_time, ref->max_watering_time);
                    ref->is_watering = 0;
                    xTimerStop(ref->timer_handle, 0);

                    STAILQ_FOREACH(entry, &ref->cb_after, next) {
                        entry->cb(entry->args);
                    }
                    app_pump_drive_set(ref, 0);
                    ref->curr_watering_time = 0;
                }
            }
            break;
            }
        }

    }
    //never reaches here
    vTaskDelete(NULL);
}

esp_err_t app_pump_init(void)
{
    app_pump_t *ref = pump_ref();
    ESP_RETURN_ON_FALSE(ref->task_handle == NULL, ESP_FAIL, TAG, "already init");

    STAILQ_INIT(&ref->cb_before);
    STAILQ_INIT(&ref->cb_during);
    STAILQ_INIT(&ref->cb_after);

    BaseType_t ret_val = xTaskCreatePinnedToCore(
                             (TaskFunction_t)        pump_task,
                             (const char *const)    "Pump Task",
                             (const uint32_t)        5 * 512,
                             (void *const)          ref,
                             (UBaseType_t)           1,
                             (TaskHandle_t *const)  & (ref->task_handle),
                             (const BaseType_t)      0);
    ESP_ERROR_CHECK(ret_val == pdPASS ? ESP_OK : ESP_FAIL);
    return ESP_OK;
}

esp_err_t app_pump_watering_start(void)
{
    app_pump_t *ref = pump_ref();
    int cmd = APP_PUMP_CMD_START;
    if (xQueueSend(ref->queue_handle, &cmd, pdMS_TO_TICKS(10)) != pdPASS) {
        ESP_LOGW(TAG, "send cmd %d failed", cmd);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t app_pump_watering_stop(void)
{
    app_pump_t *ref = pump_ref();
    int cmd = APP_PUMP_CMD_STOP;
    if (xQueueSend(ref->queue_handle, &cmd, pdMS_TO_TICKS(10)) != pdPASS) {
        ESP_LOGW(TAG, "send cmd %d failed", cmd);
        return ESP_FAIL;
    }
    return ESP_OK;
}

void app_pump_watering_stop_isr(void)
{
    app_pump_t *ref = pump_ref();
    int cmd = APP_PUMP_CMD_STOP;
    BaseType_t do_yield = pdFALSE;
    xQueueSendFromISR(ref->queue_handle, &cmd, &do_yield);
}

void app_pump_add_cb_before_watering(app_pump_cb_t cb, void *args)
{
    app_pump_t *ref = pump_ref();
    struct cb_entry *entry = malloc(sizeof(*entry));// free TODO
    entry->cb = cb;
    entry->args = args;
    STAILQ_INSERT_TAIL(&ref->cb_before, entry, next);
}

void app_pump_add_cb_during_watering(app_pump_cb_t cb, void *args)
{
    app_pump_t *ref = pump_ref();
    struct cb_entry *entry = malloc(sizeof(*entry));// free TODO
    entry->cb = cb;
    entry->args = args;
    STAILQ_INSERT_TAIL(&ref->cb_during, entry, next);
}

void app_pump_add_cb_after_watering(app_pump_cb_t cb, void *args)
{
    app_pump_t *ref = pump_ref();
    struct cb_entry *entry = malloc(sizeof(*entry));// free TODO
    entry->cb = cb;
    entry->args = args;
    STAILQ_INSERT_TAIL(&ref->cb_after, entry, next);
}

int app_pump_is_watering(void)
{
    return pump_ref()->is_watering;
}

int app_pump_curr_watering_time(void)
{
    return pump_ref()->curr_watering_time;
}

int app_pump_watering_remaining_time(void)
{
    return pump_ref()->max_watering_time - pump_ref()->curr_watering_time;
}

int app_pump_get_watering_time(void)
{
    return pump_ref()->max_watering_time;
}

int app_pump_set_watering_time(int max_time)
{
    pump_ref()->max_watering_time = max_time;
    app_nvs_set_watering_time(max_time);
    return 0;
}

int app_pump_get_auto_watering_enable(void)
{
    return pump_ref()->auto_watering_en;
}

int app_pump_set_auto_watering_enable(int on)
{
    pump_ref()->auto_watering_en = on;
    app_nvs_set_auto_watering_enable(on);
    return 0;
}

int app_pump_get_lower_humidity(void)
{
    return pump_ref()->lower_humidity;
}

int app_pump_set_lower_humidity(int min)
{
    pump_ref()->lower_humidity = min;
    app_nvs_set_lower_humidity(min);
    return 0;
}
