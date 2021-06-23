
#pragma once

#include <stdint.h>

#include "freertos/FreeRTOS.h"

#include <esp_err.h>
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif


esp_err_t server_send(const char *data, size_t size);

typedef esp_err_t (*server_recv_params_cb_t)(const char *data, size_t size);
typedef esp_err_t (*server_recv_get_cb_t)(char *data, size_t *size);

esp_err_t server_start(const char *base_path, server_recv_params_cb_t recv_params_cb, server_recv_get_cb_t recv_get_cb);

#ifdef __cplusplus
}
#endif
