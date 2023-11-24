/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */
/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_log.h>
#include <stdlib.h>
#include <string.h>

#include <device.h>
#include <esp_matter.h>
#include <esp_matter_console.h>

#include <app_reset.h>
#include <app_priv.h>
#include "app_led.h"
#include "app_fan.h"
#include "app_switch.h"
#include <app_driver.h>
#include <controller/CHIPCluster.h>
#include <zap-generated/CHIPClusters.h>

using chip::kInvalidClusterId;
static constexpr chip::CommandId kInvalidCommandId = 0xFFFF'FFFF;

using namespace chip::app::Clusters;
using namespace esp_matter;
using namespace esp_matter::cluster;

static const char *TAG = "app_driver";
extern uint16_t switch_endpoint_id;

#if CONFIG_ENABLE_CHIP_SHELL
static char console_buffer[101] = {0};
static esp_err_t app_driver_bound_console_handler(int argc, char **argv)
{
    if (argc == 1 && strncmp(argv[0], "help", sizeof("help")) == 0) {
        printf("Bound commands:\n"
               "\thelp: Print help\n"
               "\tinvoke: <local_endpoint_id> <cluster_id> <command_id> parameters ... \n"
               "\t\tExample: matter esp bound invoke 0x0001 0x0008 0x0000 0x50 0x0 0x1 0x1.\n"
               "\tinvoke-group: <local_endpoint_id> <cluster_id> <command_id> parameters ...\n"
               "\t\tExample: matter esp bound invoke-group 0x0001 0x0008 0x0000 0x50 0x0 0x1 0x1.\n");
    } else if (argc >= 4 && strncmp(argv[0], "invoke", sizeof("invoke")) == 0) {
        client::command_handle_t cmd_handle;
        uint16_t local_endpoint_id = strtol((const char *)&argv[1][2], NULL, 16);
        cmd_handle.cluster_id = strtol((const char *)&argv[2][2], NULL, 16);
        cmd_handle.command_id = strtol((const char *)&argv[3][2], NULL, 16);

        if (argc > 4) {
            console_buffer[0] = argc - 4;
            for (int i = 0; i < (argc - 4); i++) {
                if ((argv[4 + i][0] != '0') || (argv[4 + i][1] != 'x') || (strlen((const char *)&argv[4 + i][2]) > 10)) {
                    ESP_LOGE(TAG, "Incorrect arguments. Check help for more details.");
                    return ESP_ERR_INVALID_ARG;
                }
                strcpy((console_buffer + 1 + 10 * i), &argv[4 + i][2]);
            }

            cmd_handle.command_data = console_buffer;
        }

        client::cluster_update(local_endpoint_id, &cmd_handle);
    } else if (argc >= 4 && strncmp(argv[0], "invoke-group", sizeof("invoke-group")) == 0) {
        client::command_handle_t cmd_handle;
        uint16_t local_endpoint_id = strtol((const char *)&argv[1][2], NULL, 16);
        cmd_handle.cluster_id = strtol((const char *)&argv[2][2], NULL, 16);
        cmd_handle.command_id = strtol((const char *)&argv[3][2], NULL, 16);

        if (argc > 4) {
            console_buffer[0] = argc - 4;
            for (int i = 0; i < (argc - 4); i++) {
                if ((argv[4 + i][0] != '0') || (argv[4 + i][1] != 'x') || (strlen((const char *)&argv[4 + i][2]) > 10)) {
                    ESP_LOGE(TAG, "Incorrect arguments. Check help for more details.");
                    return ESP_ERR_INVALID_ARG;
                }
                strcpy((console_buffer + 1 + 10 * i), &argv[4 + i][2]);
            }

            cmd_handle.command_data = console_buffer;
        }

        client::cluster_update(local_endpoint_id, &cmd_handle);
    } else {
        ESP_LOGE(TAG, "Incorrect arguments. Check help for more details.");
        return ESP_ERR_INVALID_ARG;
    }

    console_buffer[0] = 0;

    return ESP_OK;
}

static esp_err_t app_driver_client_console_handler(int argc, char **argv)
{
    if (argc == 1 && strncmp(argv[0], "help", sizeof("help")) == 0) {
        printf("Client commands:\n"
               "\thelp: Print help\n"
               "\tinvoke: <fabric_index> <remote_node_id> <remote_endpoint_id> <cluster_id> <command_id>  parameters ... \n"
               "\t\tExample: matter esp client invoke 0x0001 0xBC5C01 0x0001 0x0008 0x0000 0x50 0x0 0x1 0x1.\n"
               "\tinvoke-group: <fabric_index> <group_id> <cluster_id> <command_id> parameters ... \n"
               "\t\tExample: matter esp client invoke-group 0x0001 0x257 0x0008 0x0000 0x50 0x0 0x1 0x1.\n");
    } else if (argc >= 6 && strncmp(argv[0], "invoke", sizeof("invoke")) == 0) {
        client::command_handle_t cmd_handle;
        uint8_t fabric_index = strtol((const char *)&argv[1][2], NULL, 16);
        uint64_t node_id = strtol((const char *)&argv[2][2], NULL, 16);
        cmd_handle.endpoint_id = strtol((const char *)&argv[3][2], NULL, 16);
        cmd_handle.cluster_id = strtol((const char *)&argv[4][2], NULL, 16);
        cmd_handle.command_id = strtol((const char *)&argv[5][2], NULL, 16);

        if (argc > 6) {
            console_buffer[0] = argc - 6;
            for (int i = 0; i < (argc - 6); i++) {
                if ((argv[6 + i][0] != '0') || (argv[6 + i][1] != 'x') || (strlen((const char *)&argv[6 + i][2]) > 10)) {
                    ESP_LOGE(TAG, "Incorrect arguments. Check help for more details.");
                    return ESP_ERR_INVALID_ARG;
                }
                strcpy((console_buffer + 1 + 10 * i), &argv[6 + i][2]);
            }

            cmd_handle.command_data = console_buffer;
        }

        client::connect(fabric_index, node_id, &cmd_handle);
    } else if (argc >= 5 && strncmp(argv[0], "invoke-group", sizeof("invoke-group")) == 0) {
        client::command_handle_t cmd_handle;
        uint8_t fabric_index = strtol((const char *)&argv[1][2], NULL, 16);
        cmd_handle.group_id = strtol((const char *)&argv[2][2], NULL, 16);
        cmd_handle.cluster_id = strtol((const char *)&argv[3][2], NULL, 16);
        cmd_handle.command_id = strtol((const char *)&argv[4][2], NULL, 16);

        if (argc > 5) {
            console_buffer[0] = argc - 5;
            for (int i = 0; i < (argc - 5); i++) {
                if ((argv[5 + i][0] != '0') || (argv[5 + i][1] != 'x') || (strlen((const char *)&argv[5 + i][2]) > 10)) {
                    ESP_LOGE(TAG, "Incorrect arguments. Check help for more details.");
                    return ESP_ERR_INVALID_ARG;
                }
                strcpy((console_buffer + 1 + 10 * i), &argv[5 + i][2]);
            }

            cmd_handle.command_data = console_buffer;
        }

        client::group_command_send(fabric_index, &cmd_handle);
    } else {
        ESP_LOGE(TAG, "Incorrect arguments. Check help for more details.");
        return ESP_ERR_INVALID_ARG;
    }

    console_buffer[0] = 0;

    return ESP_OK;
}

static void app_driver_register_commands()
{
    /* Add console command for bound devices */
    static const esp_matter::console::command_t bound_command = {
        .name = "bound",
        .description = "This can be used to simulate on-device control for bound devices."
        "Usage: matter esp bound <bound_command>. "
        "Bound commands: help, invoke",
        .handler = app_driver_bound_console_handler,
    };
    esp_matter::console::add_commands(&bound_command, 1);

    /* Add console command for client to control non-bound devices */
    static const esp_matter::console::command_t client_command = {
        .name = "client",
        .description = "This can be used to simulate on-device control for client devices."
        "Usage: matter esp client <client_command>. "
        "Client commands: help, invoke",
        .handler = app_driver_client_console_handler,
    };
    esp_matter::console::add_commands(&client_command, 1);
}
#endif // CONFIG_ENABLE_CHIP_SHELL

static uint16_t current_endpoint = 1;

extern "C" {

    void app_driver_bound_on_off(uint16_t endpoint, bool on)
    {
        //uint16_t local_endpoint_id = 1;
        client::command_handle_t cmd_handle;
        cmd_handle.cluster_id = OnOff::Id;
        cmd_handle.command_id = on ? OnOff::Commands::On::Id : OnOff::Commands::Off::Id;

        if (endpoint == LIGHT_ENDPOINT_ID) {
            app_pwm_led_set_power(on);
            printf("from box light control\n");
        } else if (endpoint == FAN_ENDPOINT_ID) {
            app_fan_set_power(on);
            printf("from box fan control\n");
        } else if (endpoint == SWITCH_ENDPOINT_ID) {
            app_switch_set_power(on);
            printf("from box switch control\n");
        }

        current_endpoint = endpoint;

        lock::chip_stack_lock(portMAX_DELAY);
        client::cluster_update(endpoint, &cmd_handle);
        lock::chip_stack_unlock();
    }

}

static on_change_cb_t g_cb_1 = NULL;
static on_change_cb_t g_cb_2 = NULL;
static on_change_cb_t g_cb_3 = NULL;

static void report_success_cb(void *context, chip::app::Clusters::OnOff::Attributes::OnOff::TypeInfo::DecodableArgType responseData)
{
    ESP_LOGI(TAG, "Report success Occupancy:%u", responseData);
    printf("report_success_cb: %d\n", *((uint16_t *)context));

    switch (*((uint16_t *)context)) {
    case LIGHT_ENDPOINT_ID:
        if (g_cb_1) {
            g_cb_1(responseData);
        }
        break;
    case FAN_ENDPOINT_ID:
        if (g_cb_2) {
            g_cb_2(responseData);
        }
        break;
    case SWITCH_ENDPOINT_ID:
        if (g_cb_3) {
            g_cb_3(responseData);
        }
        break;
    default:
        break;
    }
}

static void report_failure_cb(void *context, CHIP_ERROR err)
{
    ESP_LOGI(TAG, "Report failure, err:%ld", err.Format());
}

static esp_err_t subscribe_on_off(uint16_t local_endpoint, client::peer_device_t *remote_device, uint16_t remote_endpoint_id, on_change_cb_t cb)
{
    static uint16_t ep[4] = {0, 1, 2, 3};

    switch (ep[local_endpoint]) {
    case LIGHT_ENDPOINT_ID:
        g_cb_1 = cb;
        break;
    case FAN_ENDPOINT_ID:
        g_cb_2 = cb;
        break;
    case SWITCH_ENDPOINT_ID:
        g_cb_3 = cb;
        break;
    default:
        break;
    }

    chip::Controller::OnOffCluster cluster(*remote_device->GetExchangeManager(),
                                           remote_device->GetSecureSession().Value(), remote_endpoint_id);
    cluster.SubscribeAttribute<chip::app::Clusters::OnOff::Attributes::OnOff::TypeInfo>(&ep[local_endpoint], report_success_cb, report_failure_cb, 1, 10);
    return ESP_OK;
}

static void fan_control_cb(bool power)
{
    printf("fan_control_cb executed\n");
    app_fan_set_power(power);
}

static void light_control_cb(bool power)
{
    printf("light_control_cb executed\n");
    app_pwm_led_set_power(power);
}

static void switch_control_cb(bool power)
{
    printf("switch_control_cb executed\n");
    app_switch_set_power(power);
}

void app_driver_client_command_callback(client::peer_device_t *peer_device, client::command_handle_t *cmd_handle,
                                        void *priv_data)
{
    static bool subed[3] = {false};

    printf("Peer device address: %p\n", (void *)peer_device);
    printf("endpointId: %u\n", cmd_handle->endpoint_id);
    if (current_endpoint == FAN_ENDPOINT_ID && !subed[1]) {
        subscribe_on_off(FAN_ENDPOINT_ID, peer_device, cmd_handle->endpoint_id, fan_control_cb);
        subed[1] = true;
    } else if (current_endpoint == LIGHT_ENDPOINT_ID && !subed[0]) {
        subscribe_on_off(LIGHT_ENDPOINT_ID, peer_device, cmd_handle->endpoint_id, light_control_cb);
        subed[0] = true;
    } else if (current_endpoint == SWITCH_ENDPOINT_ID && !subed[2]) {
        subscribe_on_off(SWITCH_ENDPOINT_ID, peer_device, cmd_handle->endpoint_id, switch_control_cb);
        subed[2] = true;
    }

    if (cmd_handle->cluster_id == OnOff::Id) {
        switch (cmd_handle->command_id) {
        case OnOff::Commands::Off::Id: {
            on_off::command::send_off(peer_device, cmd_handle->endpoint_id);
            break;
        };
        case OnOff::Commands::On::Id: {
            on_off::command::send_on(peer_device, cmd_handle->endpoint_id);
            break;
        };
        case OnOff::Commands::Toggle::Id: {
            on_off::command::send_toggle(peer_device, cmd_handle->endpoint_id);
            break;
        };
        default:
            break;
        }
    } else if (cmd_handle->cluster_id == LevelControl::Id) {
        switch (cmd_handle->command_id) {
        case LevelControl::Commands::Move::Id: {
            if (((char *)cmd_handle->command_data)[0] != 4) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            level_control::command::send_move(peer_device, cmd_handle->endpoint_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                                              strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 21, NULL, 16),
                                              strtol((const char *)(cmd_handle->command_data) + 31, NULL, 16));
            break;
        };
        case LevelControl::Commands::MoveToLevel::Id: {
            if (((char *)cmd_handle->command_data)[0] != 4) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            level_control::command::send_move_to_level(peer_device, cmd_handle->endpoint_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 21, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 31, NULL, 16));
            break;
        };
        case LevelControl::Commands::Step::Id: {
            if (((char *)cmd_handle->command_data)[0] != 5) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            level_control::command::send_step(peer_device, cmd_handle->endpoint_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                                              strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 21, NULL, 16),
                                              strtol((const char *)(cmd_handle->command_data) + 31, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 41, NULL, 16));
            break;
        };
        case LevelControl::Commands::Stop::Id: {
            if (((char *)cmd_handle->command_data)[0] != 2) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            level_control::command::send_stop(peer_device, cmd_handle->endpoint_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                                              strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16));
            break;
        };
        case LevelControl::Commands::MoveWithOnOff::Id: {
            if (((char *)cmd_handle->command_data)[0] != 2) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            level_control::command::send_move_with_on_off(peer_device, cmd_handle->endpoint_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16));
            break;
        };
        case LevelControl::Commands::MoveToLevelWithOnOff::Id: {
            if (((char *)cmd_handle->command_data)[0] != 2) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            level_control::command::send_move_to_level_with_on_off(peer_device, cmd_handle->endpoint_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16));
            break;
        };
        case LevelControl::Commands::StepWithOnOff::Id: {
            if (((char *)cmd_handle->command_data)[0] != 3) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            level_control::command::send_step_with_on_off(peer_device, cmd_handle->endpoint_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 21, NULL, 16));
            break;
        };
        case LevelControl::Commands::StopWithOnOff::Id: {
            level_control::command::send_stop_with_on_off(peer_device, cmd_handle->endpoint_id);
            break;
        };
        default:
            break;
        }
    } else if (cmd_handle->cluster_id == ColorControl::Id) {
        switch (cmd_handle->command_id) {
        case ColorControl::Commands::MoveHue::Id: {
            if (((char *)cmd_handle->command_data)[0] != 4) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            color_control::command::send_move_hue(peer_device, cmd_handle->endpoint_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                                                  strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 21, NULL, 16),
                                                  strtol((const char *)(cmd_handle->command_data) + 31, NULL, 16));
            break;
        };
        case ColorControl::Commands::MoveToHue::Id: {
            if (((char *)cmd_handle->command_data)[0] != 5) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            color_control::command::send_move_to_hue(peer_device, cmd_handle->endpoint_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 21, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 31, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 41, NULL, 16));
            break;
        };
        case ColorControl::Commands::StepHue::Id: {
            if (((char *)cmd_handle->command_data)[0] != 5) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            color_control::command::send_step_hue(peer_device, cmd_handle->endpoint_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                                                  strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 21, NULL, 16),
                                                  strtol((const char *)(cmd_handle->command_data) + 31, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 41, NULL, 16));
            break;
        };
        case ColorControl::Commands::MoveSaturation::Id: {
            if (((char *)cmd_handle->command_data)[0] != 4) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            color_control::command::send_move_saturation(peer_device, cmd_handle->endpoint_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 21, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 31, NULL, 16));
            break;
        };
        case ColorControl::Commands::MoveToSaturation::Id: {
            if (((char *)cmd_handle->command_data)[0] != 4) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            color_control::command::send_move_to_saturation(peer_device, cmd_handle->endpoint_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 21, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 31, NULL, 16));
            break;
        };
        case ColorControl::Commands::StepSaturation::Id: {
            if (((char *)cmd_handle->command_data)[0] != 5) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            color_control::command::send_step_saturation(peer_device, cmd_handle->endpoint_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 21, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 31, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 41, NULL, 16));
            break;
        };
        case ColorControl::Commands::MoveToHueAndSaturation::Id: {
            if (((char *)cmd_handle->command_data)[0] != 5) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            color_control::command::send_move_to_hue_and_saturation(peer_device, cmd_handle->endpoint_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 21, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 31, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 41, NULL, 16));
            break;
        };
        default:
            ESP_LOGE(TAG, "Unsupported command");
            break;
        }
    }
}

void app_driver_client_group_command_callback(uint8_t fabric_index, client::command_handle_t *cmd_handle, void *priv_data)
{
    if (cmd_handle->cluster_id == OnOff::Id) {
        switch (cmd_handle->command_id) {
        case OnOff::Commands::Off::Id: {
            on_off::command::group_send_off(fabric_index, cmd_handle->group_id);
            break;
        };
        case OnOff::Commands::On::Id: {
            on_off::command::group_send_on(fabric_index, cmd_handle->group_id);
            break;
        };
        case OnOff::Commands::Toggle::Id: {
            on_off::command::group_send_toggle(fabric_index, cmd_handle->group_id);
            break;
        };
        default:
            break;
        }
    } else if (cmd_handle->cluster_id == LevelControl::Id) {
        switch (cmd_handle->command_id) {
        case LevelControl::Commands::Move::Id: {
            if (((char *)cmd_handle->command_data)[0] != 4) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            level_control::command::group_send_move(fabric_index, cmd_handle->group_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                                                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 21, NULL, 16),
                                                    strtol((const char *)(cmd_handle->command_data) + 31, NULL, 16));
            break;
        };
        case LevelControl::Commands::MoveToLevel::Id: {
            if (((char *)cmd_handle->command_data)[0] != 4) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            level_control::command::group_send_move_to_level(fabric_index, cmd_handle->group_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 21, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 31, NULL, 16));
            break;
        };
        case LevelControl::Commands::Step::Id: {
            if (((char *)cmd_handle->command_data)[0] != 5) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            level_control::command::group_send_step(fabric_index, cmd_handle->group_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                                                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 21, NULL, 16),
                                                    strtol((const char *)(cmd_handle->command_data) + 31, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 41, NULL, 16));
            break;
        };
        case LevelControl::Commands::Stop::Id: {
            if (((char *)cmd_handle->command_data)[0] != 2) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            level_control::command::group_send_stop(fabric_index, cmd_handle->group_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                                                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16));
            break;
        };
        case LevelControl::Commands::MoveWithOnOff::Id: {
            if (((char *)cmd_handle->command_data)[0] != 2) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            level_control::command::group_send_move_with_on_off(fabric_index, cmd_handle->group_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16));
            break;
        };
        case LevelControl::Commands::MoveToLevelWithOnOff::Id: {
            if (((char *)cmd_handle->command_data)[0] != 2) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            level_control::command::group_send_move_to_level_with_on_off(fabric_index, cmd_handle->group_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16));
            break;
        };
        case LevelControl::Commands::StepWithOnOff::Id: {
            if (((char *)cmd_handle->command_data)[0] != 3) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            level_control::command::group_send_step_with_on_off(fabric_index, cmd_handle->group_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 21, NULL, 16));
            break;
        };
        case LevelControl::Commands::StopWithOnOff::Id: {
            level_control::command::group_send_stop_with_on_off(fabric_index, cmd_handle->group_id);
            break;
        };
        default:
            break;
        }
    } else if (cmd_handle->cluster_id == ColorControl::Id) {
        switch (cmd_handle->command_id) {
        case ColorControl::Commands::MoveHue::Id: {
            if (((char *)cmd_handle->command_data)[0] != 4) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            color_control::command::group_send_move_hue(fabric_index, cmd_handle->group_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 21, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 31, NULL, 16));
            break;
        };
        case ColorControl::Commands::MoveToHue::Id: {
            if (((char *)cmd_handle->command_data)[0] != 5) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            color_control::command::group_send_move_to_hue(fabric_index, cmd_handle->group_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 21, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 31, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 41, NULL, 16));
            break;
        };
        case ColorControl::Commands::StepHue::Id: {
            if (((char *)cmd_handle->command_data)[0] != 5) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            color_control::command::group_send_step_hue(fabric_index, cmd_handle->group_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 21, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 31, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 41, NULL, 16));
            break;
        };
        case ColorControl::Commands::MoveSaturation::Id: {
            if (((char *)cmd_handle->command_data)[0] != 4) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            color_control::command::group_send_move_saturation(fabric_index, cmd_handle->group_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 21, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 31, NULL, 16));
            break;
        };
        case ColorControl::Commands::MoveToSaturation::Id: {
            if (((char *)cmd_handle->command_data)[0] != 4) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            color_control::command::group_send_move_to_saturation(fabric_index, cmd_handle->group_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 21, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 31, NULL, 16));
            break;
        };
        case ColorControl::Commands::StepSaturation::Id: {
            if (((char *)cmd_handle->command_data)[0] != 5) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            color_control::command::group_send_step_saturation(fabric_index, cmd_handle->group_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 21, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 31, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 41, NULL, 16));
            break;
        };
        case ColorControl::Commands::MoveToHueAndSaturation::Id: {
            if (((char *)cmd_handle->command_data)[0] != 5) {
                ESP_LOGE(TAG, "Number of parameters error");
                return;
            }
            color_control::command::group_send_move_to_hue_and_saturation(fabric_index, cmd_handle->group_id, strtol((const char *)(cmd_handle->command_data) + 1, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 11, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 21, NULL, 16),
                    strtol((const char *)(cmd_handle->command_data) + 31, NULL, 16), strtol((const char *)(cmd_handle->command_data) + 41, NULL, 16));
            break;
        };
        default:
            ESP_LOGE(TAG, "Unsupported command");
            break;
        }
    }
}

app_driver_handle_t matter_app_driver_switch_init()
{
    /* Initialize button */
    button_config_t button_config = button_driver_get_config();
    button_handle_t handle = iot_button_create(&button_config);

    /* Other initializations */
#if CONFIG_ENABLE_CHIP_SHELL
    app_driver_register_commands();
#endif // CONFIG_ENABLE_CHIP_SHELL
    client::set_command_callback(app_driver_client_command_callback, app_driver_client_group_command_callback, NULL);

    return (app_driver_handle_t)handle;
}
