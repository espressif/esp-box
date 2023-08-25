/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#define NODE_CONFIG_TOPIC_RULE                  "esp_node_config"
#define NODE_PARAMS_LOCAL_TOPIC_RULE            "esp_set_params"
#define NODE_PARAMS_LOCAL_INIT_RULE             "esp_init_params"
#define NODE_PARAMS_ALERT_TOPIC_RULE            "esp_node_alert"
#define USER_MAPPING_TOPIC_RULE                 "esp_user_node_mapping"
#define OTAFETCH_TOPIC_RULE                     "esp_node_otafetch"
#define OTASTATUS_TOPIC_RULE                    "esp_node_otastatus"
#define TIME_SERIES_DATA_TOPIC_RULE             "esp_ts_ingest"
#define CMD_RESP_TOPIC_RULE                     "esp_cmd_resp"


#define USER_MAPPING_TOPIC_SUFFIX               "user/mapping"
#define NODE_PARAMS_LOCAL_TOPIC_SUFFIX          "params/local"
#define NODE_PARAMS_LOCAL_INIT_TOPIC_SUFFIX     "params/local/init"
#define NODE_PARAMS_REMOTE_TOPIC_SUFFIX         "params/remote"
#define TIME_SERIES_DATA_TOPIC_SUFFIX           "tsdata"
#define NODE_PARAMS_ALERT_TOPIC_SUFFIX          "alert"
#define NODE_CONFIG_TOPIC_SUFFIX                "config"
#define OTAURL_TOPIC_SUFFIX                     "otaurl"
#define OTAFETCH_TOPIC_SUFFIX                   "otafetch"
#define OTASTATUS_TOPIC_SUFFIX                  "otastatus"
#define CMD_RESP_TOPIC_SUFFIX                   "from-node"
#define TO_NODE_TOPIC_SUFFIX                    "to-node"
#define INSIGHTS_TOPIC_SUFFIX                   "diagnostics/from-node"

#define MQTT_TOPIC_BUFFER_SIZE 150
