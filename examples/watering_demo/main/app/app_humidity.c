/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */


#include <stdbool.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include <freertos/queue.h>
#include <freertos/timers.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"

#include "driver/gpio.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "soc/soc_caps.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#else
#include "driver/adc.h"
#include "esp_adc_cal.h"
#endif

#include "app_humidity.h"

#define APP_HUMIDITY_MAX_WATCHERS (5)
#define APP_HUMIDITY_MAX_DEBOUNCE (5)


#define DEFAULT_VREF    1100
#define APP_HUMIDITY_ADC_MAX_INPUT_V (3100)

static const char *TAG = "app_humidity";

typedef struct {
    app_humidity_cb_t cb;
    void *args;
} watcher_t;

typedef struct {
    //adc pin
    gpio_num_t gpio_num;
    //adc config
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    adc_channel_t adc_channel;
    adc_atten_t   adc_atten;
    adc_bitwidth_t adc_width;
    adc_cali_handle_t adc1_cali_handle;
    adc_oneshot_unit_handle_t adc1_handle;
#else
    adc_channel_t adc_channel;
    adc_atten_t   adc_atten;
    adc_bits_width_t adc_width;
    esp_adc_cal_characteristics_t *adc_chars;
#endif
    //value
    int humidity;
    //cb
    watcher_t watchers[APP_HUMIDITY_MAX_WATCHERS];
    TaskHandle_t  task_handle;
} app_humidity_t;

static app_humidity_t _APP_HUMIDITY;

static app_humidity_t *humidity_ref(void)
{
    return &_APP_HUMIDITY;
}

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
static esp_err_t adc_calibration_init(app_humidity_t *ref)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = ADC_UNIT_2,
            .atten = ref->adc_atten,
            .bitwidth = ref->adc_width,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = ref->adc_atten,
            .bitwidth = ref->adc_width,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    ref->adc1_cali_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated ? ESP_OK : ESP_FAIL;
}
#endif

static esp_err_t app_humidity_drive_init(app_humidity_t *ref)
{
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    if (NULL == ref->adc1_handle) {
        //ADC2 Init
        adc_oneshot_unit_init_cfg_t init_config = {
            .unit_id = ADC_UNIT_2,
        };
        if (adc_oneshot_new_unit(&init_config, &ref->adc1_handle) != ESP_OK) {
            ESP_LOGW(TAG, "adc oneshot new unit fail!");
        }

        //ADC2 Config
        adc_oneshot_chan_cfg_t oneshot_config = {
            .bitwidth = ref->adc_width,
            .atten = ref->adc_atten,
        };
        if (adc_oneshot_config_channel(ref->adc1_handle, ref->adc_channel, &oneshot_config) != ESP_OK) {
            ESP_LOGW(TAG, "adc oneshot config channel fail!");
        }
        //-------------ADC2 Calibration Init---------------//
        if (adc_calibration_init(ref) != ESP_OK) {
            ESP_LOGW(TAG, "ADC2 Calibration Init False");
        }
    }
#else
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        ESP_LOGI(TAG, "eFuse Two Point: Supported");
    } else {
        ESP_LOGW(TAG, "eFuse Two Point: NOT supported");
    }

    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        ESP_LOGI(TAG, "eFuse Vref: Supported");
    } else {
        ESP_LOGW(TAG, "eFuse Vref: NOT supported");
    }
    /** Configure ADC */
    adc1_config_width(ref->adc_width);
    /** initialize adc channel */
    adc1_config_channel_atten(ref->adc_channel, ref->adc_atten);
    /** Characterize ADC */
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ref->adc_atten, ref->adc_width, DEFAULT_VREF, ref->adc_chars);

    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        ESP_LOGI(TAG, "Characterized using Two Point Value");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        ESP_LOGI(TAG, "Characterized using eFuse Vref");
    } else {
        ESP_LOGI(TAG, "Characterized using Default Vref");
    }
#endif
    return ESP_OK;
}

static int voltage2humidity(int v)
{
    int h;
    float p;
    int max_h = 84;
    int min_h = 1;

    int max_v = APP_HUMIDITY_ADC_MAX_INPUT_V;
    int min_v = 1200;

    /*all these magic numbers come from measurement by hands*/
    if (v <= min_v) {
        h = max_h;
        p = 1;
    } else if (v >= max_v) {
        h = min_h;
        p = 0.01;
    } else {
        p = 1.0 - 1.0 * (v - min_v) / (max_v - min_v);
        h = (int)(p * (max_h - min_h));
    }
    //ESP_LOGI(TAG, "v %dmv p %f h %d%%",v,p,h);
    return h;
}

static int app_humidity_drive_read_value(app_humidity_t *ref)
{
    uint32_t adc_reading = 0;
#define NO_OF_SAMPLES 64
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    int adc_raw = 0;
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        adc_oneshot_read(ref->adc1_handle, ref->adc_channel, &adc_raw);
        adc_reading += adc_raw;
    }
#else
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        adc_reading += adc1_get_raw(ref->adc_channel);
    }
#endif
    adc_reading /= NO_OF_SAMPLES;
#undef NO_OF_SAMPLES

    //return esp_adc_cal_raw_to_voltage(adc_reading, ref->adc_chars);
    return voltage2humidity(adc_reading * APP_HUMIDITY_ADC_MAX_INPUT_V / 4095);
}

static void humidity_task(void *pvParam)
{
    app_humidity_t *ref = pvParam;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    ref->adc_channel = ADC_CHANNEL_0;
    ref->adc_atten   = ADC_ATTEN_DB_11;//for box s3
    ref->adc_width   = SOC_ADC_RTC_MAX_BITWIDTH;
#else
    ref->adc_channel = ADC2_CHANNEL_0;
    ref->adc_atten   = ADC_ATTEN_DB_11;//for box s3
    ref->adc_width   = ADC_WIDTH_BIT_DEFAULT;
    ref->adc_chars   = calloc(1, sizeof(esp_adc_cal_characteristics_t));
#endif
    app_humidity_drive_init(ref);

    //init
    int debounce_cnt = 0;
    int cur_humidity = app_humidity_drive_read_value(ref);
    vTaskDelay(pdMS_TO_TICKS(5000));//wait ui

    for (;;) {
        int value = app_humidity_drive_read_value(ref);
        //ESP_LOGI(TAG, "==>h %d%%",value);

        if (value != cur_humidity) {
            cur_humidity = value;
            debounce_cnt = 0;
        } else {
            debounce_cnt++;

            if (debounce_cnt > APP_HUMIDITY_MAX_DEBOUNCE) {
                //we got a stable humidity
                if (cur_humidity != ref->humidity) {
                    ref->humidity = cur_humidity;

                    for (int i = 0; i < APP_HUMIDITY_MAX_WATCHERS; i++) {
                        if (ref->watchers[i].cb) {
                            ref->watchers[i].cb(ref->watchers[i].args);
                        }
                    }
                } else {
                    debounce_cnt = 0;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

esp_err_t app_humidity_init(void)
{
    app_humidity_t *ref = humidity_ref();
    ESP_RETURN_ON_FALSE(ref->task_handle == NULL, ESP_FAIL, TAG, "already init");

    BaseType_t ret_val = xTaskCreatePinnedToCore(
                             (TaskFunction_t)        humidity_task,
                             (const char *const)    "RH Task",
                             (const uint32_t)        5 * 512,
                             (void *const)          ref,
                             (UBaseType_t)           1,
                             (TaskHandle_t *const)  & (ref->task_handle),
                             (const BaseType_t)      0);
    ESP_ERROR_CHECK(ret_val == pdPASS ? ESP_OK : ESP_FAIL);
    return ESP_OK;
}
int app_humidity_get_value(void)
{
    return humidity_ref()->humidity;
}
esp_err_t app_humidity_add_watcher(app_humidity_cb_t cb, void *args)
{
    app_humidity_t *ref = humidity_ref();

    for (int i = 0; i < APP_HUMIDITY_MAX_WATCHERS; i++) {
        if (ref->watchers[i].cb == NULL) {
            ref->watchers[i].cb = cb;
            ref->watchers[i].args = args;
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

esp_err_t app_humidity_del_watcher(app_humidity_cb_t cb, void *args)
{
    app_humidity_t *ref = humidity_ref();

    for (int i = 0; i < APP_HUMIDITY_MAX_WATCHERS; i++) {
        if (ref->watchers[i].cb == cb) {
            ref->watchers[i].cb = NULL;
            ref->watchers[i].args = NULL;
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}
