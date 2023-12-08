/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include "app_touch.h"

/* Touch buttons handle */
static touch_button_handle_t touch_button_handle;

/* Button event handler task */
static void touch_handler_task(void *arg)
{
    (void) arg; //Unused
    touch_elem_message_t element_message;
    ESP_LOGI(TOUCH_TAG, "Touch element library start");
    while (1) {
        /* Waiting for touch element messages */
        touch_element_message_receive(&element_message, portMAX_DELAY);
        if (element_message.element_type != TOUCH_ELEM_TYPE_BUTTON) {
            continue;
        }
        /* Decode message */
        const touch_button_message_t *button_message = touch_button_get_message(&element_message);
        if (button_message->event == TOUCH_BUTTON_EVT_ON_PRESS) {
            ESP_LOGI(TOUCH_TAG, "Button[%d] Press", (int)element_message.arg);
        } else if (button_message->event == TOUCH_BUTTON_EVT_ON_RELEASE) {
            ESP_LOGI(TOUCH_TAG, "Button[%d] Release", (int)element_message.arg);
        } else if (button_message->event == TOUCH_BUTTON_EVT_ON_LONGPRESS) {
            ESP_LOGI(TOUCH_TAG, "Button[%d] LongPress", (int)element_message.arg);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void touch_sensor_init()
{
    /* Initialize Touch Element library */
    touch_elem_global_config_t global_config = TOUCH_ELEM_GLOBAL_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(touch_element_install(&global_config));
    ESP_LOGI(TOUCH_TAG, "Touch element library installed");

    touch_button_global_config_t button_global_config = TOUCH_BUTTON_GLOBAL_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(touch_button_install(&button_global_config));
    ESP_LOGI(TOUCH_TAG, "Touch button installed");
    touch_button_config_t button_config = {
        .channel_num = TOUCH_CHANNEL,
        .channel_sens = TOUCH_CHANNEL_SENSOR
    };
    /* Create Touch buttons */
    ESP_ERROR_CHECK(touch_button_create(&button_config, &touch_button_handle));
    /* Subscribe touch button events (On Press, On Release, On LongPress) */
    ESP_ERROR_CHECK(touch_button_subscribe_event(touch_button_handle,
                    TOUCH_ELEM_EVENT_ON_PRESS | TOUCH_ELEM_EVENT_ON_RELEASE | TOUCH_ELEM_EVENT_ON_LONGPRESS,
                    (void *)TOUCH_CHANNEL));

    ESP_ERROR_CHECK(touch_button_set_dispatch_method(touch_button_handle, TOUCH_ELEM_DISP_EVENT));

    /* Set LongPress event trigger threshold time */
    ESP_ERROR_CHECK(touch_button_set_longpress(touch_button_handle, 2000));

    ESP_LOGI(TOUCH_TAG, "Touch buttons created");

    xTaskCreate(&touch_handler_task, "touch_handler_task", 4 * 1024, NULL, 5, NULL);
}
