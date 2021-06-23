/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"

#include "web_server.h"
static const char *TAG = "rest_server";

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

static server_recv_params_cb_t g_recv_params_cb = NULL;
static server_recv_get_cb_t g_recv_get_cb    = NULL;

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "text/plain";

    if (CHECK_FILE_EXTENSION(filepath, ".html")) {
        type = "text/html";
    } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
        type = "application/javascript";
    } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
        type = "text/css";
    } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
        type = "image/png";
    } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
        type = "image/x-icon";
    } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
        type = "text/xml";
    }

    return httpd_resp_set_type(req, type);
}

/* Send HTTP response with the contents of the requested file */
/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];

    rest_server_context_t *rest_context = (rest_server_context_t *)req->user_ctx;
    strlcpy(filepath, rest_context->base_path, sizeof(filepath));
    if (req->uri[strlen(req->uri) - 1] == '/') {
        strlcat(filepath, "/index.html", sizeof(filepath));
    } else {
        strlcat(filepath, req->uri, sizeof(filepath));
    }
    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1) {
        ESP_LOGE(TAG, "Failed to open file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);

    char *chunk = rest_context->scratch;
    ssize_t read_bytes;
    do {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        if (read_bytes == -1) {
            ESP_LOGE(TAG, "Failed to read file : %s", filepath);
        } else if (read_bytes > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
                close(fd);
                ESP_LOGE(TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    /* Close file after sending complete */
    close(fd);
    ESP_LOGI(TAG, "File sending complete");
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

/*
 * Structure holding server handle
 * and internal socket fd in order
 * to use out of request send
 */
typedef struct {
    httpd_handle_t hd;
    int fd;
} async_resp_t;

static async_resp_t *g_resp_handle = NULL;

/*
 * async send function, which we put into the httpd work queue
 */
esp_err_t server_send(const char *data, size_t size)
{
    if (!g_resp_handle) {
        return ESP_OK;
    }

    esp_err_t ret = ESP_OK;

    ESP_LOGD(TAG, "ret: %d, %s", ret, data);

    httpd_ws_frame_t ws_pkt = {
        .payload = (uint8_t *) data,
        .len     = size,
        .type    = HTTPD_WS_TYPE_TEXT,
    };

    ret = httpd_ws_send_frame_async(g_resp_handle->hd, g_resp_handle->fd, &ws_pkt);

    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "<%s> httpd_ws_send_frame_async, err_str: %s", esp_err_to_name(ret), strerror(errno));
        close(g_resp_handle->fd);
        free(g_resp_handle);
        return ret;
    }

    return ESP_OK;
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t get_nodes_params(httpd_req_t *req)
{
    esp_err_t ret = ESP_OK;
    uint8_t buf[64] = { 0 };

    httpd_ws_frame_t ws_pkt = {
        .payload = buf,
        .type    = HTTPD_WS_TYPE_TEXT,
    };

    ret = httpd_ws_recv_frame(req, &ws_pkt, 64);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
        return ret;
    }

    // ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);

    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT &&
            strcmp((char *)ws_pkt.payload, "Trigger async") == 0) {
        if (!g_resp_handle) {
            g_resp_handle = malloc(sizeof(async_resp_t));
        }

        g_resp_handle->hd = req->handle;
        g_resp_handle->fd = httpd_req_to_sockfd(req);
    } else {
        ret = httpd_ws_send_frame(req, &ws_pkt);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
        }
    }

    return ret;
}

// {"method":"contrl","type":"light","params":{"hue":1,"saturation":1,"value":1}}
// {"method":"config","type":"light","params":[{"status":1},{"status":0}],"voice":["da kai dian deng","guang bi dian deng"],"gpio":[1,1,2]}
static esp_err_t put_nodes_params(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;

    static const char *resp_data =
        "{"
            "\"status\":\"success\","
            "\"description\":\"Success description\""
        "}";

    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }

    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);

        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }

        cur_len += received;
    }

    buf[total_len] = '\0';

    g_recv_params_cb(buf, strlen(buf));

    httpd_resp_sendstr(req, resp_data);

    return ESP_OK;
}

static esp_err_t put_nodes_get(httpd_req_t *req)
{
    char *resp_data  = calloc(1, 2048);
    size_t resp_size = 0;

    memset(resp_data, 0, 2048);

    g_recv_get_cb(resp_data, &resp_size);

    httpd_resp_sendstr(req, resp_data);
    free(resp_data);

    return ESP_OK;
}

#include "dns_server.h"

static const ip_addr_t ipaddr = IPADDR4_INIT_BYTES(192, 168, 4, 1);
static const ip_addr_t fake_addr = IPADDR4_INIT_BYTES(127, 0, 0, 1);

/* handle any DNS requests from dns-server */
static bool dns_query_proc(const char *name, ip_addr_t *addr)
{
    /**
     * captive: generate_204, cp.a, hotspot-detect.html
     */
    static const char *white_list[] = {
        "esp-box",
        "esp-box.local",
        // "cdn.experiment.xiaomi.com",
        // "cnbj1-fds.api.xiaomi.net",
        // "connect.rom.miui.com",
        // "data.mistat.xiaomi.com",
        // "o2o.api.xiaomi.com",
        // "tracker.ai.xiaomi.com",
        // "mtalk.google.com"
        // "alt2-mtalk.google.com"
    };

    for (size_t i = 0; i < sizeof(white_list) / sizeof(white_list[0]); i++) {
        if (0 == strcmp(white_list[i], name)) {
            ESP_LOGI(TAG, "name: %s", name);
            *addr = ipaddr;
            return true;
        }
    }

    ESP_LOGW(TAG, "DNS query : %s", name);
    *addr = fake_addr;

    return true;
}

esp_err_t server_start(const char *base_path, server_recv_params_cb_t recv_params_cb, server_recv_get_cb_t recv_get_cb)
{
    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

    g_recv_params_cb = recv_params_cb;
    g_recv_get_cb    = recv_get_cb;

    esp_err_t ret = ESP_OK;
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.recv_wait_timeout  = 30;
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP Server");
    ret = httpd_start(&server, &config);
    // ESP_ERROR_GOTO(ret!= ESP_OK, EXIT, "Start server failed");

    /* URI handler for fetching system info */
    httpd_uri_t put_params_uri = {
        .uri = "/v1/user/nodes/params",
        .method = HTTP_PUT, // ,
        .handler = put_nodes_params,
        .user_ctx = rest_context
    };

    httpd_register_uri_handler(server, &put_params_uri);

    /* URI handler for fetching system info */
    httpd_uri_t get_params_uri = {
        .uri = "/v1/user/nodes/info",
        .method = HTTP_GET,
        .handler = put_nodes_get,
        .user_ctx = rest_context
    };

    httpd_register_uri_handler(server, &get_params_uri);

    static const httpd_uri_t debug_recv_uri = {
        .uri        = "/v1/user/nodes/params",
        .method     = HTTP_GET,
        .handler    = get_nodes_params,
        .user_ctx   = NULL,
        .is_websocket = true
    };

    httpd_register_uri_handler(server, &debug_recv_uri);

    /* URI handler for getting web server files */
    httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = rest_common_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &common_get_uri);

    while (dnserv_init(&ipaddr, DNS_SERVER_PORT, dns_query_proc) != ERR_OK);

    return ret;
}
