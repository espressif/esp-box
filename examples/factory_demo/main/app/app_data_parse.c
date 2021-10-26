/**
 * @file app_data_parse.c
 * @brief 
 * @version 0.1
 * @date 2021-09-27
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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "app_led.h"
#include "app_sr.h"
#include "cJSON.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "lvgl.h"

static char cmd_on[32] = "Turn On The Light";
static char cmd_off[32] = "Turn Off The Light";
static char voice_on[32] = CONFIG_EN_SPEECH_COMMAND_ID4;
static char voice_off[32] = CONFIG_EN_SPEECH_COMMAND_ID5;
static const char *TAG = "data_parse";

char *get_cmd_string(bool on)
{
    if (on) {
        return (char *) cmd_on;
    }

    return (char *) cmd_off;
}

static void parse_config(cJSON *root_obj)
{
    cJSON *item_obj = NULL;

    item_obj = cJSON_GetObjectItem(root_obj, "type");
    if (NULL != item_obj) {
        printf("%s : %s\n", item_obj->string, item_obj->valuestring);
    }

    item_obj = cJSON_GetObjectItem(root_obj, "node_id");
    if (NULL != item_obj) {
        printf("%s : %d\n", item_obj->string, (int) item_obj->valuedouble);
    }

    cJSON *param_array = cJSON_GetObjectItem(root_obj, "params");
    if (NULL != param_array) {
        int param_array_num = cJSON_GetArraySize(param_array);
        printf("Array %s has %d object(s)\n", param_array->string, param_array_num);
        for (int i = 0; i < param_array_num; i++) {
            cJSON *param_item = cJSON_GetArrayItem(param_array, i);

            if (strcmp("config", param_item->string)) {
                printf("%s : %d\n", param_item->string, (int) param_item->valuedouble);
            } else {
                cJSON *item_led_obj = cJSON_GetObjectItem(param_item, "gpio");
                if (NULL != item_led_obj) {
                    printf("%s : %d\n", item_led_obj->string, (int) item_led_obj->valuedouble);
                }

                cJSON *voice_item_array = cJSON_GetObjectItem(param_item, "params");
                if (NULL == voice_item_array) {
                    return;
                }
    
                int voice_item_size = cJSON_GetArraySize(voice_item_array);
                printf("Voice item size : %d\n", voice_item_size);
                for (size_t i = 0; i < voice_item_size; i++) {
                    cJSON *voice_item = cJSON_GetArrayItem(voice_item_array, i);
                    if (cJSON_GetObjectItem(voice_item, "status")->valuedouble == 1) {
                        printf("Voice to turn on : (%s)[%s]\n",
                            cJSON_GetObjectItem(voice_item, "zh")->valuestring,
                            cJSON_GetObjectItem(voice_item, "voice")->valuestring);
                            strcpy(cmd_on, cJSON_GetObjectItem(voice_item, "zh")->valuestring);
                            strcpy(voice_on, cJSON_GetObjectItem(voice_item, "voice")->valuestring);
                    } else {
                        printf("Voice to turn on : (%s)[%s]\n",
                            cJSON_GetObjectItem(voice_item, "zh")->valuestring,
                            cJSON_GetObjectItem(voice_item, "voice")->valuestring);
                            strcpy(cmd_off, cJSON_GetObjectItem(voice_item, "zh")->valuestring);
                            strcpy(voice_off, cJSON_GetObjectItem(voice_item, "voice")->valuestring);
                    }

                }

                /* Reset command list */
                char *cmd = (char *) heap_caps_malloc(256, MALLOC_CAP_SPIRAM);
                if (NULL != cmd) {
                    cmd[0] = '\0';  /* Make cmd a valid string */

                    /* Command should match `sdkconfig` */
                    strcat(cmd, CONFIG_EN_SPEECH_COMMAND_ID0 ";");  /*!< Red */
                    strcat(cmd, CONFIG_EN_SPEECH_COMMAND_ID1 ";");  /*!< Green */
                    strcat(cmd, CONFIG_EN_SPEECH_COMMAND_ID2 ";");  /*!< Blue */
                    strcat(cmd, CONFIG_EN_SPEECH_COMMAND_ID3 ";");  /*!< White */
                    strcat(strcat(cmd, voice_on), ";");             /*!< On */
                    strcat(strcat(cmd, voice_off), ";");            /*!< Off */

                    /* Print new command list */
                    ESP_LOGI(TAG, "New command set to :\n" "%s", cmd);

                    /* Reset command list */
                    app_sr_reset_command_list(cmd);

                    free(cmd);
                } else {    /* NULL == cmd */
                    ESP_LOGE(TAG, "No memory for new command list");
                    return;
                }
            }
        }
    } else {    /* (NULL == param_array) */
        return;
    }
}

static void parse_control(cJSON *root_obj)
{
    cJSON *item_obj = NULL;
    led_state_t led_state;
    app_led_get_state(&led_state);

    item_obj = cJSON_GetObjectItem(root_obj, "type");
    printf("%s : %s\n", item_obj->string, item_obj->valuestring);

    item_obj = cJSON_GetObjectItem(root_obj, "node_id");
    printf("%s : %d\n", item_obj->string, (int) item_obj->valuedouble);
    
    cJSON *param_array = cJSON_GetObjectItem(root_obj, "params");
    int param_array_num = cJSON_GetArraySize(param_array);
    printf("Array %s has %d object(s)\n",
        param_array->string, param_array_num);

    item_obj = cJSON_GetObjectItem(param_array, "status");
    if (NULL != item_obj) {
        printf("Light status : %d\n", (int) item_obj->valuedouble);
        led_state.on = (int) item_obj->valuedouble;
    }

    item_obj = cJSON_GetObjectItem(param_array, "hue");
    if (NULL != item_obj) {
        printf("Light hue : %d\n", (int) item_obj->valuedouble);
        led_state.h = (int) item_obj->valuedouble;
    }

    item_obj = cJSON_GetObjectItem(param_array, "saturation");
    if (NULL != item_obj) {
        printf("Light saturation : %d\n", (int) item_obj->valuedouble);
        led_state.s = (int) item_obj->valuedouble;
    }

    item_obj = cJSON_GetObjectItem(param_array, "value");
    if (NULL != item_obj) {
        printf("Light value : %d\n", (int) item_obj->valuedouble);
        led_state.v = (int) item_obj->valuedouble;
    }

    if (led_state.on) {
        lv_color_t color = lv_color_hsv_to_rgb(led_state.h, led_state.s, led_state.v);
        app_pwm_led_set_all(color.ch.red << 3, ((color.ch.green_h << 3) + (color.ch.green_l)) << 2, color.ch.blue << 3);
    } else {
        app_pwm_led_set_all(0, 0, 0);
    }
}

esp_err_t app_wifi_parse_json_string(char *text)
{
    cJSON *root_obj = NULL;
    cJSON *method_obj = NULL;

    if (NULL == text) {
        return ESP_ERR_INVALID_ARG;
    }

    root_obj = cJSON_Parse(text);
    method_obj = cJSON_GetObjectItem(root_obj, "method");

    if (0 == strcmp(method_obj->valuestring, "contrl")) {
        printf("Control CMD : \n");
        parse_control(root_obj);
    } else if (0 == strcmp(method_obj->valuestring, "config")) {
        printf("Config CMD : \n");
        parse_config(root_obj);
    } else {
        printf("Invalid CMD : %s\n", method_obj->valuestring);
    }

    cJSON_Delete(root_obj);

    return ESP_OK;
}

esp_err_t app_wifi_build_json_string(led_state_t *led_state, char **json_string)
{
    if (NULL == led_state) {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *root_obj = NULL;
    cJSON *node_list_obj = NULL;
    cJSON *led_obj = NULL;
    cJSON *led_param_obj = NULL;
    cJSON *led_config_obj = NULL;
    cJSON *led_param_array = NULL;
    cJSON *led_param_on = NULL;
    cJSON *led_param_off = NULL;

    /* Create root object */
    root_obj = cJSON_CreateObject();

    /* Add object(s) to root */
    cJSON_AddStringToObject(root_obj, "method", "info");
    node_list_obj = cJSON_AddArrayToObject(root_obj, "node_list");

    led_obj = cJSON_CreateObject();
    cJSON_AddItemToArray(node_list_obj, led_obj);
    cJSON_AddStringToObject(led_obj, "type", "light");
    cJSON_AddNumberToObject(led_obj, "node_id", 1);
    led_param_obj = cJSON_AddObjectToObject(led_obj, "params");

    cJSON_AddNumberToObject(led_param_obj, "status", led_state->on);
    cJSON_AddNumberToObject(led_param_obj, "hue", led_state->h);
    cJSON_AddNumberToObject(led_param_obj, "saturation", led_state->s);
    cJSON_AddNumberToObject(led_param_obj, "value", led_state->v);

    led_config_obj = cJSON_AddObjectToObject(led_param_obj, "config");
    cJSON_AddNumberToObject(led_config_obj, "gpio", led_state->gpio);
    led_param_array = cJSON_AddArrayToObject(led_config_obj, "params");

    led_param_on = cJSON_CreateObject();
    led_param_off = cJSON_CreateObject();
    cJSON_AddItemToArray(led_param_array, led_param_on);
    cJSON_AddNumberToObject(led_param_on, "status", 1);
    cJSON_AddStringToObject(led_param_on, "voice", voice_on);
    cJSON_AddStringToObject(led_param_on, "zh", cmd_on);

    cJSON_AddItemToArray(led_param_array, led_param_off);
    cJSON_AddNumberToObject(led_param_off, "status", 0);
    cJSON_AddStringToObject(led_param_off, "voice", voice_off);
    cJSON_AddStringToObject(led_param_off, "zh", cmd_off);

    /* Print serialized object */
    *json_string = cJSON_PrintUnformatted(root_obj);

    /* Delete object after used */
    cJSON_Delete(root_obj);

    return ESP_OK;
}
