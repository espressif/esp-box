
/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sdkconfig.h>
#include <esp_log.h>
#include <esp_err.h>
#include <stdbool.h>
static const char *TAG = "esp_rmaker_mqtt_budget";

#ifdef CONFIG_ESP_RMAKER_MQTT_ENABLE_BUDGETING

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <freertos/semphr.h>

#define DEFAULT_BUDGET              CONFIG_ESP_RMAKER_MQTT_DEFAULT_BUDGET
#define MAX_BUDGET                  CONFIG_ESP_RMAKER_MQTT_MAX_BUDGET
#define BUDGET_REVIVE_COUNT         CONFIG_ESP_RMAKER_MQTT_BUDGET_REVIVE_COUNT
#define BUDGET_REVIVE_PERIOD        CONFIG_ESP_RMAKER_MQTT_BUDGET_REVIVE_PERIOD

static int16_t mqtt_budget = DEFAULT_BUDGET;
static TimerHandle_t mqtt_budget_timer;
static SemaphoreHandle_t mqtt_budget_lock;
#define SEMAPHORE_DELAY_MSEC         500

bool esp_rmaker_mqtt_is_budget_available(void)
{
    if (mqtt_budget_lock == NULL) {
        ESP_LOGW(TAG, "MQTT budgeting not started yet. Allowing publish.");
        return true;
    }
    if (xSemaphoreTake(mqtt_budget_lock, SEMAPHORE_DELAY_MSEC/portTICK_PERIOD_MS) != pdTRUE) {
        ESP_LOGW(TAG, "Could not acquire MQTT budget lock. Allowing publish.");
        return true;
    }
    int16_t budget = mqtt_budget;
    xSemaphoreGive(mqtt_budget_lock);
    return budget ? true : false;
}

esp_err_t esp_rmaker_mqtt_increase_budget(uint8_t budget)
{
    if (mqtt_budget_lock == NULL) {
        ESP_LOGW(TAG, "MQTT budgeting not started. Not increasing the budget.");
        return ESP_FAIL;
    }
    if (xSemaphoreTake(mqtt_budget_lock, SEMAPHORE_DELAY_MSEC/portTICK_PERIOD_MS) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to increase MQTT budget.");
        return ESP_FAIL;
    }
    mqtt_budget += budget;
    if (mqtt_budget > MAX_BUDGET) {
        mqtt_budget = MAX_BUDGET;
    }
    xSemaphoreGive(mqtt_budget_lock);
    ESP_LOGD(TAG, "MQTT budget increased to %d", mqtt_budget);
    return ESP_OK;
}

esp_err_t esp_rmaker_mqtt_decrease_budget(uint8_t budget)
{
    if (mqtt_budget_lock == NULL) {
        ESP_LOGW(TAG, "MQTT budgeting not started. Not decreasing the budget.");
        return ESP_FAIL;
    }
    if (xSemaphoreTake(mqtt_budget_lock, SEMAPHORE_DELAY_MSEC/portTICK_PERIOD_MS) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to decrease MQTT budget.");
        return ESP_FAIL;
    }
    mqtt_budget -= budget;
    if (mqtt_budget < 0) {
        mqtt_budget = 0;
    }
    xSemaphoreGive(mqtt_budget_lock);
    ESP_LOGD(TAG, "MQTT budget decreased to %d.", mqtt_budget);
    return ESP_OK;
}

static void esp_rmaker_mqtt_revive_budget(TimerHandle_t handle)
{
    esp_rmaker_mqtt_increase_budget(BUDGET_REVIVE_COUNT);
}

esp_err_t esp_rmaker_mqtt_budgeting_start(void)
{
    if (mqtt_budget_timer) {
        xTimerStart(mqtt_budget_timer, 0);
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t esp_rmaker_mqtt_budgeting_stop(void)
{
    if (mqtt_budget_timer) {
        xTimerStop(mqtt_budget_timer, 100);
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t esp_rmaker_mqtt_budgeting_deinit(void)
{
    if (mqtt_budget_timer) {
        esp_rmaker_mqtt_budgeting_stop();
        xTimerDelete(mqtt_budget_timer, 100);
        mqtt_budget_timer = NULL;
    }
    if (mqtt_budget_lock) {
        vSemaphoreDelete(mqtt_budget_lock);
        mqtt_budget_lock = NULL;
    }
    return ESP_OK;
}

esp_err_t esp_rmaker_mqtt_budgeting_init(void)
{
    if (mqtt_budget_timer) {
        ESP_LOGI(TAG, "MQTT budgeting already initialised.");
        return ESP_OK;
    }

    mqtt_budget_lock = xSemaphoreCreateMutex();
    if (!mqtt_budget_lock) {
        return ESP_FAIL;
    }

    mqtt_budget_timer = xTimerCreate("mqtt_budget_tm", (BUDGET_REVIVE_PERIOD * 1000) / portTICK_PERIOD_MS,
                            pdTRUE, NULL, esp_rmaker_mqtt_revive_budget);
    if (mqtt_budget_timer) {
        ESP_LOGI(TAG, "MQTT Budgeting initialised. Default: %d, Max: %d, Revive count: %d, Revive period: %d",
                DEFAULT_BUDGET, MAX_BUDGET, BUDGET_REVIVE_COUNT, BUDGET_REVIVE_PERIOD);
        return ESP_OK;
    }
    return ESP_FAIL;
}

#else /* ! CONFIG_ESP_RMAKER_MQTT_ENABLE_BUDGETING */

esp_err_t esp_rmaker_mqtt_budgeting_init(void)
{
    /* Adding a print only here, because this is always going to be the first function
     * to be invoked since it is called from MQTT init. Else, MQTT itself is going to fail.
     */
    ESP_LOGW(TAG, "MQTT Budgeting is not enabled.");
    return ESP_FAIL;
}

esp_err_t esp_rmaker_mqtt_budgeting_deinit(void)
{
    return ESP_FAIL;
}

esp_err_t esp_rmaker_mqtt_budgeting_stop(void)
{
    return ESP_FAIL;
}

esp_err_t esp_rmaker_mqtt_budgeting_start(void)
{
    return ESP_FAIL;
}

esp_err_t esp_rmaker_mqtt_increase_budget(uint8_t budget)
{
    return ESP_OK;
}

esp_err_t esp_rmaker_mqtt_decrease_budget(uint8_t budget)
{
    return ESP_OK;
}

bool esp_rmaker_mqtt_is_budget_available(void)
{
    return true;
}

#endif /* ! CONFIG_ESP_RMAKER_MQTT_ENABLE_BUDGETING */
