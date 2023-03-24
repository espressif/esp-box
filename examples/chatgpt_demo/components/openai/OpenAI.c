/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <string.h>
#include "OpenAI.h"
#include "esp_log.h"
#include <json_parser.h>
#include "esp_http_client.h"
#include "esp_crt_bundle.h"

static const char *json_fmt = "{\"model\":\"%s\",\"max_tokens\":%d,\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}]}";
static const char *TAG = "Open AI";

char OPENAI_API_KEY[OPENAI_API_KEY_MAX_LEN];
char message_content[MESSAGE_CONTENT_SIZE] = {0};
char message_content_for_whisper[1024] = {0};
char json_payload[512];

// Forward Declaration will be remove in new version of OpenAI component

typedef struct {
    bool need_hint;
    region_t current_server;
    char ssid[32];
    char password[64];
    uint8_t ssid_len;
    uint8_t password_len;
} sys_param_t;

// Declaration of the weak function will be remove in new version of OpenAI component

sys_param_t* __attribute__((weak)) settings_get_parameter(void) {
    static sys_param_t default_parameters = {
        .current_server = REGION_Espressif,  // Default server
    };
    return &default_parameters;
}

// Declaration of the weak function will be remove in new version of OpenAI component

esp_err_t __attribute__((weak)) settings_write_parameter_to_nvs(void){

    printf("This is the weak function.\n");
    return NULL;  // Or return a default sys_param_t value
}

/* Function to set the OpenAI API key */

void set_api_key(const char *key)
{
    if (key == NULL) {
        ESP_LOGE(TAG, "OpenAI API key is NULL");
        return;
    }
    size_t key_length = strlen(key);
    if (key_length >= OPENAI_API_KEY_MAX_LEN) {
        ESP_LOGE(TAG, "OpenAI API key is too long: %s", key);
        return;
    }
    strncpy(OPENAI_API_KEY, key, OPENAI_API_KEY_MAX_LEN - 1);
    OPENAI_API_KEY[OPENAI_API_KEY_MAX_LEN - 1] = '\0';

    ESP_LOGI(TAG, "API Key entered");
}

/* Parsing response coming from server */

void parse_response (const char *data, int len)
{
    jparse_ctx_t jctx;
    int ret = json_parse_start(&jctx, data, len);
    if (ret != OS_SUCCESS) {
        ESP_LOGE(TAG, "Parser failed");
        return;
    }
    printf("\n");
    int num_choices;
    /* Parsing OpenAI & Espressif server Chat GPT response */
    if (json_obj_get_array(&jctx, "choices", &num_choices) == OS_SUCCESS) {
        for (int i = 0; i < num_choices; i++) {
            if (json_arr_get_object(&jctx, i) == OS_SUCCESS && json_obj_get_object(&jctx, "message") == OS_SUCCESS &&
                    json_obj_get_string(&jctx, "content", message_content, sizeof(message_content)) == OS_SUCCESS) {
                ESP_LOGI(TAG, "ChatGPT message_content: %s\n", message_content);
            }
            json_arr_leave_object(&jctx);
        }
        json_obj_leave_array(&jctx);
    }
    /* Parsing OpenAI server Whisper AI response */
    else if (json_obj_get_string(&jctx, "text", message_content, sizeof(message_content)) == OS_SUCCESS) {
        ESP_LOGI(TAG, "Whisper message_content: %s", message_content);
    } else if (json_obj_get_object(&jctx, "error") == OS_SUCCESS) {
        if (json_obj_get_string(&jctx, "type", message_content, sizeof(message_content)) == OS_SUCCESS) {
            ESP_LOGE(TAG, "API returns an error: %s", message_content);
        }
    }
}

/* Parsing espressif server Whisper AI response */

void parse_espressif_server_whisper_response(const char *data, int len)
{
    jparse_ctx_t jctx;
    int ret = json_parse_start(&jctx, data, len);
    if (ret != OS_SUCCESS) {
        ESP_LOGE(TAG, "Parser failed");
        return;
    }
    if (json_obj_get_string(&jctx, "whisper", message_content_for_whisper, sizeof(message_content_for_whisper)) == OS_SUCCESS) {
        ESP_LOGI(TAG, "Whisper message_content: %s", message_content_for_whisper);
    } else {
        ESP_LOGE(TAG, "Error in parse whisper AI response data");
    }
}

/* Intermediate function to correctly parse data*/

void parsing_data(const char *data, int len)
{
    if (data == NULL) {
        ESP_LOGE(TAG, "Response data is NULL");
        return;
    }
    sys_param_t *sys_set = settings_get_parameter();

    if (sys_set->current_server == REGION_Espressif) {
        ESP_LOGI(TAG, "parsing Espressif server response");
        parse_espressif_server_whisper_response(data, len);
    }
    parse_response(data, len);
}

/* Response handler for HTTPS request */

esp_err_t response_handler(esp_http_client_event_t *evt)
{
    static char *data = NULL; // Initialize data to NULL
    static int data_len = 0; // Initialize data to NULL

    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
        break;

    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;

    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;

    case HTTP_EVENT_ON_HEADER:
        if (evt->data_len) {
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
            ESP_LOGI(TAG, "%.*s", evt->data_len, (char *)evt->data);
        }
        break;

    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA (%d +)%d", data_len, evt->data_len);
        ESP_LOGI(TAG, "Raw Response: data length: (%d +)%d: %.*s", data_len, evt->data_len, evt->data_len, (char *)evt->data);
        data = heap_caps_realloc(data, data_len + evt->data_len + 1,  MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (data == NULL) {
            ESP_LOGE(TAG, "data realloc failed");
            free(data);
            data = NULL;
            break;
        }
        memcpy(data + data_len, (char *)evt->data, evt->data_len);
        data_len += evt->data_len;
        data[data_len] = '\0';
        break;

    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
        if (data != NULL) {
            parsing_data(data, strlen(data));
            free(data); // Free memory
            data = NULL;
            data_len = 0;
        }
        break;

    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        break;

    default:
        break;
    }
    return ESP_OK;
}

/* This function creates an HTTP POST request to the OpenAI chatGPT API */

esp_err_t create_chatgpt_request(const char *content)
{
    char url[128] = "https://api.openai.com/v1/chat/completions";
    char model[16] = "gpt-3.5-turbo";
    char headers[512];
    snprintf(headers, sizeof(headers), "Bearer %s", OPENAI_API_KEY);
    ESP_LOGI(TAG, "Set Header %s", headers);
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .event_handler = response_handler,
        .buffer_size = MAX_HTTP_RECV_BUFFER,
        .timeout_ms = 30000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    uint32_t starttime = esp_log_timestamp();
	ESP_LOGE(TAG, "[Start] create_CHATGPT_request, timestamp:%"PRIu32, starttime);

    // Set the headers
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", headers);

    snprintf(json_payload, sizeof(json_payload), json_fmt, model, MAX_RESPONSE_TOKEN, content);
    ESP_LOGI(TAG, "JSON Payload: %s", json_payload);
    esp_http_client_set_post_field(client, json_payload, strlen(json_payload));

    // Send the request
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "HTTP POST request failed: %s\n", esp_err_to_name(err));
    }
    ESP_LOGE(TAG, "[End] create_CHATGPT_request, + offset: %" PRIu32, esp_log_timestamp() - starttime);
    esp_http_client_cleanup(client);
    return err;
}

/* This function creates an HTTP POST request to the OpenAI whisper API */

esp_err_t create_whisper_request_from_record(uint8_t *audio, int audio_len)
{
    if (audio == NULL) {
        // Handle the error condition appropriately
        ESP_LOGE(TAG, "Audio data is NULL");
        return ESP_ERR_INVALID_ARG; // Or any other error code indicating the issue
    }
    char url[128]; // declare the variable here
    char headers[512];
    sys_param_t *sys_set = settings_get_parameter();

    if (sys_set->current_server == REGION_Espressif) {
        ESP_LOGI(TAG, "Espressif server");
        snprintf(url, sizeof(url), "https://flask-production-1b02.up.railway.app/audio");
    } else {
        snprintf(url, sizeof(url), "https://api.openai.com/v1/audio/transcriptions");
    }
    snprintf(headers, sizeof(headers), "Bearer %s", OPENAI_API_KEY);
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .event_handler = response_handler,
        .buffer_size = MAX_HTTP_RECV_BUFFER,
        .timeout_ms = 60000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    uint32_t starttime = esp_log_timestamp();
	ESP_LOGE(TAG, "[Start] create_whisper_request, timestamp:%"PRIu32, starttime);

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Set the headers
    esp_http_client_set_header(client, "Authorization", headers);

    // Set the content type and the boundary string
    char boundary[] = "boundary1234567890";
    char content_type[64];
    snprintf(content_type, sizeof(content_type), "multipart/form-data; boundary=%s", boundary);
    esp_http_client_set_header(client, "Content-Type", content_type);

    char *file_data = NULL;
    size_t file_size;

    file_data = (char *)audio;
    file_size = audio_len;

    // Build the multipart/form-data request
    char *form_data = (char *)malloc(MAX_HTTP_RECV_BUFFER);
    assert(form_data);
    ESP_LOGI(TAG, "Size of form_data buffer: %zu bytes", sizeof(*form_data) * MAX_HTTP_RECV_BUFFER);
    int form_data_len = 0;
    form_data_len += snprintf(form_data + form_data_len, MAX_HTTP_RECV_BUFFER - form_data_len,
                              "--%s\r\n"
                              "Content-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\n"
                              "Content-Type: application/octet-stream\r\n"
                              "\r\n", boundary, get_file_format(file_type));
    ESP_LOGI(TAG, "form_data_len %d", form_data_len);
    ESP_LOGI(TAG, "form_data %s\n", form_data);

    // Append the audio file contents
    memcpy(form_data + form_data_len, file_data, file_size);
    form_data_len += file_size;
    ESP_LOGI(TAG, "Size of form_data: %zu", form_data_len);

    // Append the rest of the form-data
    form_data_len += snprintf(form_data + form_data_len, MAX_HTTP_RECV_BUFFER - form_data_len,
                              "\r\n"
                              "--%s\r\n"
                              "Content-Disposition: form-data; name=\"model\"\r\n"
                              "\r\n"
                              "whisper-1\r\n"
                              "--%s--\r\n", boundary, boundary);

    ESP_LOGI(TAG, "Size of form_data: %zu", form_data_len);
    // Set the headers and post field
    esp_http_client_set_post_field(client, form_data, form_data_len);

    // Send the request
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "HTTP POST request failed: %s\n", esp_err_to_name(err));
    }
    ESP_LOGE(TAG, "[End] create_wisper_request, + offset:%"PRIu32, esp_log_timestamp() - starttime);

    esp_http_client_cleanup(client);
    free(form_data);
    return err;
}

/* This function set the server based on region */

void set_server(const region_t server)
{
    sys_param_t *sys_set = settings_get_parameter();

    switch (server) {
    case REGION_OpenAI:
        sys_set->current_server = REGION_OpenAI;
        ESP_LOGI(TAG, "Current server: OpenAI");
        break;
    case REGION_Espressif:
        sys_set->current_server = REGION_Espressif;
        ESP_LOGI(TAG, "Current server: Espressif");
        break;
    default:
        // Invalid region value
        ESP_LOGE(TAG, "Error: Invalid region value %d", sys_set->current_server);
        break;
    }

    settings_write_parameter_to_nvs();
}

/* Set the audio file format to use */

void set_audio_type(const format_t type)
{
    file_type = type;
    ESP_LOGI(TAG, "Audio file format: %s", get_file_format(file_type));
}

/* Get the message content for the last response from ChatGPT */

char *get_message_content_for_chatgpt(void)
{
    return message_content;
}

/* Get the message content for the last response from Whisper AI */

char *get_message_content_for_whisper(void)
{
    return message_content_for_whisper;
}

/* Get the audio file_format use */

char *get_file_format(format_t file_type)
{
    switch (file_type) {
    case FORMAT_M4A:
        return "m4a";
    case FORMAT_MP3:
        return "mp3";
    case FORMAT_WEBM:
        return "webm";
    case FORMAT_MP4:
        return "mp4";
    case FORMAT_MPGA:
        return "mpga";
    case FORMAT_WAV:
        return "wav";
    case FORMAT_MPEG:
        return "mpeg";
    default:
        return "wav";
    }
}
