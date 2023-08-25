/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <inttypes.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_spiffs.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "app_disp_fs.h"
#include "jpeg_decoder.h"

/* SPIFFS mount root */
#define FS_MNT_PATH  BSP_SPIFFS_MOUNT_POINT

/* Buffer for reading/writing to I2S driver. Same length as SPIFFS buffer and I2S buffer, for optimal read/write performance.
   Recording audio data path:
   I2S peripheral -> I2S buffer (DMA) -> App buffer (RAM) -> SPIFFS buffer -> External SPI Flash.
   Vice versa for playback. */
#define BUFFER_SIZE     (1024)
#define SAMPLE_RATE     (22050)
#define DEFAULT_VOLUME  (70)
/* The recording will be RECORDING_LENGTH * BUFFER_SIZE long (in bytes)
   With sampling frequency 22050 Hz and 16bit mono resolution it equals to ~3.715 seconds */
#define RECORDING_LENGTH (160)

#define REC_FILENAME    FS_MNT_PATH"/recording.wav"

static const char *TAG = "DISP";

static esp_codec_dev_handle_t spk_codec_dev = NULL;
static esp_codec_dev_handle_t mic_codec_dev = NULL;

/*******************************************************************************
* Types definitions
*******************************************************************************/
typedef enum {
    APP_FILE_TYPE_UNKNOWN,
    APP_FILE_TYPE_TXT,
    APP_FILE_TYPE_IMG,
    APP_FILE_TYPE_WAV,
} app_file_type_t;

// Very simple WAV header, ignores most fields
typedef struct __attribute__((packed))
{
    uint8_t ignore_0[22];
    uint16_t num_channels;
    uint32_t sample_rate;
    uint8_t ignore_1[6];
    uint16_t bits_per_sample;
    uint8_t ignore_2[4];
    uint32_t data_size;
    uint8_t data[];
} dumb_wav_header_t;

/*******************************************************************************
* Function definitions
*******************************************************************************/
static void app_disp_lvgl_show_settings(lv_obj_t *screen, lv_group_t *group);
static void app_disp_lvgl_show_record(lv_obj_t *screen, lv_group_t *group);
static void app_disp_lvgl_show_filesystem(lv_obj_t *screen, lv_group_t *group);
static void app_disp_lvgl_show_files(const char *path);
static void scroll_begin_event(lv_event_t *e);
static void tab_changed_event(lv_event_t *e);
static void set_tab_group(void);

/*******************************************************************************
* Local variables
*******************************************************************************/

static lv_obj_t *tabview = NULL;
static lv_group_t *filesystem_group = NULL;
static lv_group_t *recording_group = NULL;
static lv_group_t *settings_group = NULL;

/* FS */
static lv_obj_t *fs_list = NULL;
static lv_obj_t *fs_img = NULL;
static char fs_current_path[250];

static uint8_t *file_buffer = NULL;
static size_t file_buffer_size = 0;

/* Audio */
static SemaphoreHandle_t audio_mux;
static bool play_file_repeat = false;
static bool play_file_stop = false;
static char usb_drive_play_file[250];
static lv_obj_t *play_btn = NULL, *play1_btn = NULL, *rec_btn = NULL, *rec_stop_btn = NULL;

/*******************************************************************************
* Public API functions
*******************************************************************************/

void app_disp_lvgl_show(void)
{
    bsp_display_lock(0);

    /* Tabview */
    tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 40);
    lv_obj_set_size(tabview, BSP_LCD_H_RES, BSP_LCD_V_RES);
    lv_obj_align(tabview, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_text_font(tabview, &lv_font_montserrat_14, 0);
    /* Change animation time of moving between tabs */
    lv_obj_add_event_cb(lv_tabview_get_content(tabview), scroll_begin_event, LV_EVENT_SCROLL_BEGIN, NULL);
    lv_obj_add_event_cb(tabview, tab_changed_event, LV_EVENT_VALUE_CHANGED, NULL);

    /* Tabview buttons style */
    lv_obj_t *tab_btns = lv_tabview_get_tab_btns(tabview);
    lv_obj_set_style_bg_color(tab_btns, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    lv_obj_set_style_text_color(tab_btns, lv_palette_lighten(LV_PALETTE_GREEN, 5), 0);
    lv_obj_set_style_border_side(tab_btns, LV_BORDER_SIDE_BOTTOM, LV_PART_ITEMS | LV_STATE_CHECKED);

    /* Add tabs (the tabs are page (lv_page) and can be scrolled */
    lv_obj_t *tab_filesystem = lv_tabview_add_tab(tabview, LV_SYMBOL_LIST" File System");
    lv_obj_t *tab_rec = lv_tabview_add_tab(tabview, LV_SYMBOL_AUDIO" Record");
    lv_obj_t *tab_settings = lv_tabview_add_tab(tabview, LV_SYMBOL_SETTINGS" Settings");

    /* Input device group */
    lv_indev_t *indev = bsp_display_get_input_dev();
    if (indev && indev->driver && indev->driver->type == LV_INDEV_TYPE_ENCODER) {
        filesystem_group = lv_group_create();
        recording_group = lv_group_create();
        settings_group = lv_group_create();
        lv_group_add_obj(filesystem_group, tab_btns);
        lv_group_add_obj(recording_group, tab_btns);
        lv_group_add_obj(settings_group, tab_btns);
        lv_indev_set_group(indev, filesystem_group);
        ESP_LOGI(TAG, "Input device group was set.");
    }

    /* Show file system tab page */
    app_disp_lvgl_show_filesystem(tab_filesystem, filesystem_group);

    /* Show record tab page */
    app_disp_lvgl_show_record(tab_rec, recording_group);

    /* Show settings tab page */
    app_disp_lvgl_show_settings(tab_settings, settings_group);

    bsp_display_unlock();
}

void app_audio_init(void)
{
    /* Initialize speaker */
    spk_codec_dev = bsp_audio_codec_speaker_init();
    assert(spk_codec_dev);
    /* Speaker output volume */
    esp_codec_dev_set_out_vol(spk_codec_dev, DEFAULT_VOLUME);

    /* Initialize microphone */
    mic_codec_dev = bsp_audio_codec_microphone_init();
    assert(mic_codec_dev);
    /* Microphone input gain */
    esp_codec_dev_set_in_gain(mic_codec_dev, 50.0);
}

void app_disp_fs_init(void)
{
    file_buffer_size = BSP_LCD_H_RES * BSP_LCD_V_RES * sizeof(lv_color_t);
    file_buffer = heap_caps_calloc(file_buffer_size, 1, MALLOC_CAP_DEFAULT);
    assert(file_buffer);

    /* Initialize root path */
    strcpy(fs_current_path, FS_MNT_PATH);

    /* Show list of files */
    app_disp_lvgl_show_files(FS_MNT_PATH);
}

/*******************************************************************************
* Private API function
*******************************************************************************/

static void app_lvgl_add_text(const char *text)
{
    lv_list_add_text(fs_list, text);
}

static void folder_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED) {
        const char *foldername = lv_list_get_btn_text(fs_list, obj);
        if (foldername != NULL) {
            strcat(fs_current_path, "/");
            strcat(fs_current_path, foldername);
            ESP_LOGI(TAG, "Clicked: \"%s\"", fs_current_path);
            app_disp_lvgl_show_files(fs_current_path);
        }

    }
}

static void close_window_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        memset(file_buffer, 0, file_buffer_size);
        lv_obj_del(e->user_data);

        /* Re-set the TAB group */
        set_tab_group();
    }
}

static void show_window(const char *path, app_file_type_t type)
{
    struct stat st;
    lv_obj_t *label = NULL;
    lv_obj_t *btn;
    lv_obj_t *win = lv_win_create(lv_scr_act(), 40);
    lv_win_add_title(win, path);

    /* Close button */
    btn = lv_win_add_btn(win, LV_SYMBOL_CLOSE, 60);
    lv_obj_add_event_cb(btn, close_window_handler, LV_EVENT_CLICKED, win);

    lv_obj_t *cont = lv_win_get_content(win);   /*Content can be added here*/

    label = lv_label_create(cont);
    lv_obj_set_width(label, 290);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label, "");
    lv_obj_center(label);

    if (type == APP_FILE_TYPE_IMG) {
        fs_img = lv_canvas_create(cont);
    }

    /* Show image or text file */
    if (type == APP_FILE_TYPE_TXT || type == APP_FILE_TYPE_IMG) {
        /* Get file size */
        int f = stat(path, &st);
        if (f == 0) {
            uint32_t filesize = (uint32_t) st.st_size;
            char *file_buf = heap_caps_malloc(filesize + 1, MALLOC_CAP_DMA);
            if (file_buf == NULL) {
                lv_label_set_text(label, "Not enough memory!");
                return;
            }

            /* Open file */
            f = open(path, O_RDONLY);
            if (f > 0) {
                /* Read file */
                read(f, file_buf, filesize);
                if (type == APP_FILE_TYPE_TXT && label) {
                    file_buf[filesize] = 0;
                    lv_label_set_text(label, file_buf);
                } else if (fs_img) {
                    ESP_LOGI(TAG, "Decoding JPEG image...");
                    /* JPEG decode */
                    esp_jpeg_image_cfg_t jpeg_cfg = {
                        .indata = (uint8_t *)file_buf,
                        .indata_size = filesize,
                        .outbuf = file_buffer,
                        .outbuf_size = file_buffer_size,
                        .out_format = JPEG_IMAGE_FORMAT_RGB565,
                        .out_scale = JPEG_IMAGE_SCALE_0,
                        .flags = {
                            .swap_color_bytes = 1,
                        }
                    };
                    esp_jpeg_image_output_t outimg;
                    esp_jpeg_decode(&jpeg_cfg, &outimg);

                    lv_canvas_set_buffer(fs_img, file_buffer, outimg.width, outimg.height, LV_IMG_CF_TRUE_COLOR);
                    lv_obj_center(fs_img);
                    lv_obj_invalidate(fs_img);
                }

                close(f);
            } else {
                lv_label_set_text(label, "File not found!");
            }

            free(file_buf);
        } else {
            lv_label_set_text(label, "File not found!");
        }
    } else if (label) {
        lv_label_set_text(label, "Unsupported file type!");
    }

    /* Input device group */
    lv_indev_t *indev = bsp_display_get_input_dev();
    if (indev && indev->driver && indev->driver->type == LV_INDEV_TYPE_ENCODER) {
        lv_group_t *group = lv_group_create();
        lv_group_add_obj(group, btn);
        lv_indev_set_group(indev, group);
    }

}

static void play_file(void *arg)
{
    char *path = arg;
    FILE *file = NULL;
    int16_t *wav_bytes = heap_caps_malloc(BUFFER_SIZE, MALLOC_CAP_DEFAULT);
    if (wav_bytes == NULL) {
        ESP_LOGE(TAG, "Not enough memory for playing!");
        goto END;
    }

    /* Open file */
    file = fopen(path, "rb");
    if (file == NULL) {
        ESP_LOGE(TAG, "%s file does not exist!", path);
        goto END;
    }

    /* Read WAV header file */
    dumb_wav_header_t wav_header;
    if (fread((void *)&wav_header, 1, sizeof(wav_header), file) != sizeof(wav_header)) {
        ESP_LOGW(TAG, "Error in reading file");
        goto END;
    }
    ESP_LOGI(TAG, "Number of channels: %" PRIu16 "", wav_header.num_channels);
    ESP_LOGI(TAG, "Bits per sample: %" PRIu16 "", wav_header.bits_per_sample);
    ESP_LOGI(TAG, "Sample rate: %" PRIu32 "", wav_header.sample_rate);
    ESP_LOGI(TAG, "Data size: %" PRIu32 "", wav_header.data_size);


    esp_codec_dev_sample_info_t fs = {
        .sample_rate = wav_header.sample_rate,
        .channel = wav_header.num_channels,
        .bits_per_sample = wav_header.bits_per_sample,
    };
    esp_codec_dev_open(spk_codec_dev, &fs);

    uint32_t bytes_send_to_i2s = 0;
    do {
        bytes_send_to_i2s = 0;
        fseek(file, sizeof(wav_header), SEEK_SET);
        while (bytes_send_to_i2s < wav_header.data_size) {
            if (play_file_stop) {
                goto END;
            }
            xSemaphoreTake(audio_mux, portMAX_DELAY);

            /* Get data from SPIFFS */
            size_t bytes_read_from_spiffs = fread(wav_bytes, 1, BUFFER_SIZE, file);

            /* Send it to I2S */
            esp_codec_dev_write(spk_codec_dev, wav_bytes, bytes_read_from_spiffs);
            bytes_send_to_i2s += bytes_read_from_spiffs;
            xSemaphoreGive(audio_mux);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    } while (play_file_repeat);



END:
    esp_codec_dev_close(spk_codec_dev);

    if (file) {
        fclose(file);
    }

    if (wav_bytes) {
        free(wav_bytes);
    }

    if (play_btn) {
        bsp_display_lock(0);
        lv_obj_clear_state(play_btn, LV_STATE_DISABLED);
        bsp_display_unlock();
    }

    if (play1_btn) {
        bsp_display_lock(0);
        lv_obj_clear_state(play1_btn, LV_STATE_DISABLED);
        bsp_display_unlock();
    }

    vTaskDelete(NULL);
}

/* Play selected audio file */
static void play_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED) {
        play_file_stop = false;
        lv_obj_add_state(obj, LV_STATE_DISABLED);
        xTaskCreate(play_file, "audio_task", 4096, e->user_data, 6, NULL);
    }
}

/* Stop playing audio file */
static void stop_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        play_file_stop = true;
    }
}

/* Enable repeat playing of the file */
static void repeat_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        play_file_repeat = ( (lv_obj_get_state(obj) & LV_STATE_CHECKED) ? true : false);
    }
}

static void volume_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);

    assert(slider != NULL);

    int32_t volume = lv_slider_get_value(slider);
    if (spk_codec_dev) {
        esp_codec_dev_set_out_vol(spk_codec_dev, volume);
    }
}

static void close_window_wav_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        memset(file_buffer, 0, file_buffer_size);
        lv_obj_del(e->user_data);
        play_file_stop = true;

        xSemaphoreTake(audio_mux, portMAX_DELAY);
        vSemaphoreDelete(audio_mux);

        /* Re-set the TAB group */
        set_tab_group();
    }
}

static void show_window_wav(const char *path)
{
    lv_obj_t *label;
    lv_obj_t *btn, *stop_btn, *repeat_btn;
    lv_obj_t *win = lv_win_create(lv_scr_act(), 40);
    lv_win_add_title(win, path);

    strcpy(usb_drive_play_file, path);

    play_file_repeat = false;

    audio_mux = xSemaphoreCreateMutex();
    assert(audio_mux);

    /* Close button */
    btn = lv_win_add_btn(win, LV_SYMBOL_CLOSE, 60);
    lv_obj_add_event_cb(btn, close_window_wav_handler, LV_EVENT_CLICKED, win);

    lv_obj_t *cont = lv_win_get_content(win);   /*Content can be added here*/

    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *cont_row = lv_obj_create(cont);
    lv_obj_set_size(cont_row, BSP_LCD_H_RES - 20, 80);
    lv_obj_align(cont_row, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_flex_flow(cont_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_top(cont_row, 2, 0);
    lv_obj_set_style_pad_bottom(cont_row, 2, 0);
    lv_obj_set_flex_align(cont_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Play button */
    play_btn = lv_btn_create(cont_row);
    label = lv_label_create(play_btn);
    lv_label_set_text_static(label, LV_SYMBOL_PLAY);
    lv_obj_add_event_cb(play_btn, play_event_cb, LV_EVENT_CLICKED, (char *)usb_drive_play_file);

    /* Stop button */
    stop_btn = lv_btn_create(cont_row);
    label = lv_label_create(stop_btn);
    lv_label_set_text_static(label, LV_SYMBOL_STOP);
    lv_obj_add_event_cb(stop_btn, stop_event_cb, LV_EVENT_CLICKED, NULL);

    /* Repeat button */
    repeat_btn = lv_btn_create(cont_row);
    label = lv_label_create(repeat_btn);
    lv_obj_add_flag(repeat_btn, LV_OBJ_FLAG_CHECKABLE);
    lv_label_set_text_static(label, LV_SYMBOL_LOOP);
    lv_obj_add_event_cb(repeat_btn, repeat_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    cont_row = lv_obj_create(cont);
    lv_obj_set_size(cont_row, BSP_LCD_H_RES - 20, 80);
    lv_obj_align(cont_row, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_flex_flow(cont_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_top(cont_row, 2, 0);
    lv_obj_set_style_pad_bottom(cont_row, 2, 0);
    lv_obj_set_flex_align(cont_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Volume */
    label = lv_label_create(cont_row);
    lv_label_set_text_static(label, "Volume: ");

    /* Slider */
    lv_obj_t *slider = lv_slider_create(cont_row);
    lv_obj_set_width(slider, BSP_LCD_H_RES - 180);
    lv_slider_set_range(slider, 0, 90);
    lv_slider_set_value(slider, DEFAULT_VOLUME, false);
    lv_obj_center(slider);
    lv_obj_add_event_cb(slider, volume_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Input device group */
    lv_indev_t *indev = bsp_display_get_input_dev();
    if (indev && indev->driver && indev->driver->type == LV_INDEV_TYPE_ENCODER) {
        lv_group_t *group = lv_group_create();
        lv_group_add_obj(group, btn);
        lv_group_add_obj(group, play_btn);
        lv_group_add_obj(group, stop_btn);
        lv_group_add_obj(group, repeat_btn);
        lv_group_add_obj(group, slider);
        lv_indev_set_group(indev, group);
    }
}

/* Get file type by filename extension */
static app_file_type_t get_file_type(const char *filepath)
{
    assert(filepath != NULL);

    /* Find last dot */
    for (int i = (strlen(filepath) - 1); i >= 0; i--) {
        if (filepath[i] == '.') {

            if (strcmp(&filepath[i + 1], "JPG") == 0 || strcmp(&filepath[i + 1], "jpg") == 0) {
                return APP_FILE_TYPE_IMG;
            } else if (strcmp(&filepath[i + 1], "TXT") == 0 || strcmp(&filepath[i + 1], "txt") == 0) {
                return APP_FILE_TYPE_TXT;
            } else if (strcmp(&filepath[i + 1], "WAV") == 0 || strcmp(&filepath[i + 1], "wav") == 0) {
                return APP_FILE_TYPE_WAV;
            }

            break;
        }
    }

    return APP_FILE_TYPE_UNKNOWN;
}

/* Clicked to file button */
static void file_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED) {
        char filepath[250];
        const char *filename = lv_list_get_btn_text(fs_list, obj);

        strcpy(filepath, fs_current_path);
        strcat(filepath, "/");
        strcat(filepath, filename);

        /* Open window by file type (Image, text or music) */
        ESP_LOGI(TAG, "Clicked: %s", lv_list_get_btn_text(fs_list, obj));
        app_file_type_t filetype = get_file_type(filepath);
        if (filetype == APP_FILE_TYPE_WAV) {
            show_window_wav(filepath);
        } else {
            show_window(filepath, filetype);
        }
    }
}

static void remove_last_folder(char *str)
{
    assert(str != NULL);

    for (int i = (strlen(str) - 1); i >= 0; i--) {
        if (str[i] == '/') {
            str[i] = '\0';
            break;
        }
    }
}

/* Clicked to back button */
static void back_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        remove_last_folder(fs_current_path);
        ESP_LOGI(TAG, "Clicked back to: \"%s\"", fs_current_path);
        app_disp_lvgl_show_files(fs_current_path);
    }
}

static void app_lvgl_add_back(void)
{
    lv_obj_t *btn;

    /* Back button */
    btn = lv_list_add_btn(fs_list, LV_SYMBOL_LEFT, "Back");
    lv_obj_set_style_bg_color(btn, lv_color_make(0x00, 0x00, 0x00), 0);
    lv_obj_set_style_text_color(btn, lv_color_make(0xFF, 0xFF, 0xFF), 0);
    lv_obj_add_event_cb(btn, back_handler, LV_EVENT_CLICKED, NULL);
}

static void app_lvgl_add_file(const char *filename)
{
    lv_obj_t *btn;
    char *icon = LV_SYMBOL_FILE;
    app_file_type_t filetype = get_file_type(filename);

    /* File icon by type */
    switch (filetype) {
    case APP_FILE_TYPE_IMG:
        icon = LV_SYMBOL_IMAGE;
        break;
    case APP_FILE_TYPE_WAV:
        icon = LV_SYMBOL_AUDIO;
        break;
    default:
        icon = LV_SYMBOL_FILE;
    }

    /* File button */
    btn = lv_list_add_btn(fs_list, icon, filename);
    lv_obj_set_style_bg_color(btn, lv_color_make(0x00, 0x00, 0x00), 0);
    lv_obj_set_style_text_color(btn, lv_color_make(0xFF, 0xFF, 0xFF), 0);
    lv_obj_add_event_cb(btn, file_handler, LV_EVENT_CLICKED, NULL);

    if (filesystem_group) {
        lv_group_add_obj(filesystem_group, btn);
    }
}

static void app_lvgl_add_folder(const char *filename)
{
    lv_obj_t *btn;

    /* Directory button */
    btn = lv_list_add_btn(fs_list, LV_SYMBOL_DIRECTORY, filename);
    lv_obj_set_style_bg_color(btn, lv_color_make(0x00, 0x00, 0x00), 0);
    lv_obj_set_style_text_color(btn, lv_color_make(0xFF, 0xFF, 0xFF), 0);
    lv_obj_add_event_cb(btn, folder_handler, LV_EVENT_CLICKED, NULL);

    if (filesystem_group) {
        lv_group_add_obj(filesystem_group, btn);
    }
}

static void app_disp_lvgl_show_files(const char *path)
{
    struct dirent *dir;
    DIR *d;

    /* Clean all items in the list */
    lv_obj_clean(fs_list);

    /* Current path */
    app_lvgl_add_text(path);

    /* Not root -> Add back button */
    if (strcmp(path, FS_MNT_PATH) != 0) {
        app_lvgl_add_back();
    }

    /* Open directory */
    d = opendir(path);
    if (d != NULL) {
        /* Show button in the list for file of directory (Note: Directories are not supported in SPIFFS) */
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_DIR) {
                app_lvgl_add_folder(dir->d_name);
            } else {
                app_lvgl_add_file(dir->d_name);
            }
        }

        closedir(d);
    }
}

static void app_disp_lvgl_show_filesystem(lv_obj_t *screen, lv_group_t *group)
{
    /* Disable scrolling in this TAB */
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    /* TAB style */
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_bg_color(screen, lv_color_make(0x00, 0x00, 0x00), 0);
    lv_obj_set_style_bg_grad_color(screen, lv_color_make(0x05, 0x05, 0x05), 0);
    lv_obj_set_style_bg_grad_dir(screen, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(screen, 255, 0);

    /* File list */
    fs_list = lv_list_create(screen);
    lv_obj_set_size(fs_list, 320, 200);
    lv_obj_set_style_bg_color(fs_list, lv_color_make(0x00, 0x00, 0x00), 0);
    lv_obj_set_style_text_color(fs_list, lv_color_make(0xFF, 0xFF, 0xFF), 0);
    lv_obj_center(fs_list);
}

static void slider_brightness_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);

    assert(slider != NULL);

    /* Set brightness */
    bsp_display_brightness_set(lv_slider_get_value(slider));
}

/* Play recorded audio file */
static void rec_play_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED) {
        audio_mux = xSemaphoreCreateMutex();
        assert(audio_mux);
        play_file_stop = false;
        lv_obj_add_state(obj, LV_STATE_DISABLED);
        xTaskCreate(play_file, "audio_task", 4096, e->user_data, 6, NULL);
    }
}

/* Stop playing recorded audio file */
static void rec_stop_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        play_file_stop = true;
    }
}

static void rec_file(void *arg)
{
    char *path = arg;
    FILE *record_file = NULL;
    int16_t *recording_buffer = heap_caps_malloc(BUFFER_SIZE, MALLOC_CAP_DEFAULT);
    if (recording_buffer == NULL) {
        ESP_LOGE(TAG, "Not enough memory for playing!");
        goto END;
    }

    /* Open file for recording */
    record_file = fopen(path, "wb");
    if (record_file == NULL) {
        ESP_LOGE(TAG, "%s file does not exist!", path);
        goto END;
    }

    /* Write WAV file header */
    const dumb_wav_header_t recording_header = {
        .bits_per_sample = 16,
        .data_size = RECORDING_LENGTH * BUFFER_SIZE,
        .num_channels = 1,
        .sample_rate = SAMPLE_RATE
    };
    if (fwrite((void *)&recording_header, 1, sizeof(dumb_wav_header_t), record_file) != sizeof(dumb_wav_header_t)) {
        ESP_LOGW(TAG, "Error in writting to file");
        goto END;
    }

    ESP_LOGI(TAG, "Recording start");

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = SAMPLE_RATE,
        .channel = 1,
        .bits_per_sample = 16,
    };
    esp_codec_dev_open(mic_codec_dev, &fs);

    size_t bytes_written_to_spiffs = 0;
    while (bytes_written_to_spiffs < RECORDING_LENGTH * BUFFER_SIZE) {
        ESP_ERROR_CHECK(esp_codec_dev_read(mic_codec_dev, recording_buffer, BUFFER_SIZE));

        /* Write WAV file data */
        size_t data_written = fwrite(recording_buffer, 1, BUFFER_SIZE, record_file);
        bytes_written_to_spiffs += data_written;
    }

    ESP_LOGI(TAG, "Recording stop, length: %i bytes", bytes_written_to_spiffs);

END:
    esp_codec_dev_close(mic_codec_dev);

    if (record_file) {
        fclose(record_file);
    }

    if (recording_buffer) {
        free(recording_buffer);
    }

    if (rec_btn && play1_btn && rec_stop_btn) {
        bsp_display_lock(0);
        lv_obj_clear_state(rec_btn, LV_STATE_DISABLED);
        lv_obj_clear_state(play1_btn, LV_STATE_DISABLED);
        lv_obj_clear_state(rec_stop_btn, LV_STATE_DISABLED);
        bsp_display_unlock();
    }

    vTaskDelete(NULL);
}

/* Stop playing recorded audio file */
static void rec_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED) {
        lv_obj_add_state(obj, LV_STATE_DISABLED);
        if (rec_stop_btn && play1_btn) {
            lv_obj_add_state(play1_btn, LV_STATE_DISABLED);
            lv_obj_add_state(rec_stop_btn, LV_STATE_DISABLED);
        }
        xTaskCreate(rec_file, "rec_file", 4096, e->user_data, 6, NULL);
    }
}

static void app_disp_lvgl_show_record(lv_obj_t *screen, lv_group_t *group)
{
    lv_obj_t *label;

    /* Disable scrolling in this TAB */
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    /* TAB style */
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_bg_color(screen, lv_color_make(0x00, 0x00, 0x00), 0);
    lv_obj_set_style_bg_grad_color(screen, lv_color_make(0x05, 0x05, 0x05), 0);
    lv_obj_set_style_bg_grad_dir(screen, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(screen, 255, 0);

    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Buttons */
    lv_obj_t *cont_row = lv_obj_create(screen);
    lv_obj_set_size(cont_row, BSP_LCD_H_RES - 20, 80);
    lv_obj_align(cont_row, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_flex_flow(cont_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_top(cont_row, 2, 0);
    lv_obj_set_style_pad_bottom(cont_row, 2, 0);
    lv_obj_set_flex_align(cont_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Rec button */
    rec_btn = lv_btn_create(cont_row);
    label = lv_label_create(rec_btn);
    lv_label_set_text_static(label, "REC");
    lv_obj_add_event_cb(rec_btn, rec_event_cb, LV_EVENT_CLICKED, (char *)REC_FILENAME);

    /* Play button */
    play1_btn = lv_btn_create(cont_row);
    label = lv_label_create(play1_btn);
    lv_label_set_text_static(label, LV_SYMBOL_PLAY);
    lv_obj_add_event_cb(play1_btn, rec_play_event_cb, LV_EVENT_CLICKED, (char *)REC_FILENAME);

    /* Stop button */
    rec_stop_btn = lv_btn_create(cont_row);
    label = lv_label_create(rec_stop_btn);
    lv_label_set_text_static(label, LV_SYMBOL_STOP);
    lv_obj_add_event_cb(rec_stop_btn, rec_stop_event_cb, LV_EVENT_CLICKED, NULL);

    if (group) {
        lv_group_add_obj(group, rec_btn);
        lv_group_add_obj(group, play1_btn);
        lv_group_add_obj(group, rec_stop_btn);
    }
}

static void app_disp_lvgl_show_settings(lv_obj_t *screen, lv_group_t *group)
{
    lv_obj_t *cont_row;
    lv_obj_t *slider;

    /* Disable scrolling in this TAB */
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    /* TAB style */
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_bg_color(screen, lv_color_make(0x00, 0x00, 0x00), 0);
    lv_obj_set_style_bg_grad_color(screen, lv_color_make(0x05, 0x05, 0x05), 0);
    lv_obj_set_style_bg_grad_dir(screen, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(screen, 255, 0);

    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Brightness */
    cont_row = lv_obj_create(screen);
    lv_obj_set_size(cont_row, BSP_LCD_H_RES - 20, 80);
    lv_obj_align(cont_row, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_flex_flow(cont_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_top(cont_row, 2, 0);
    lv_obj_set_style_pad_bottom(cont_row, 2, 0);
    lv_obj_set_flex_align(cont_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Label */
    lv_obj_t *lbl = lv_label_create(cont_row);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_label_set_text_static(lbl, "Brightness: ");
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, 0);

    /* Slider */
    slider = lv_slider_create(cont_row);
    lv_obj_set_width(slider, BSP_LCD_H_RES - 180);
    lv_slider_set_range(slider, 10, 100);
    lv_slider_set_value(slider, APP_DISP_DEFAULT_BRIGHTNESS, false);
    lv_obj_center(slider);
    lv_obj_add_event_cb(slider, slider_brightness_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    if (group) {
        lv_group_add_obj(group, slider);
    }
}

static void set_tab_group(void)
{
    lv_indev_t *indev = bsp_display_get_input_dev();
    if (indev && filesystem_group && recording_group && settings_group) {
        uint16_t tab = lv_tabview_get_tab_act(tabview);
        lv_group_set_editing(filesystem_group, false);
        lv_group_set_editing(recording_group, false);
        lv_group_set_editing(settings_group, false);
        switch (tab) {
        case 0:
            lv_indev_set_group(indev, filesystem_group);
            break;
        case 1:
            lv_indev_set_group(indev, recording_group);
            break;
        case 2:
            lv_indev_set_group(indev, settings_group);
            break;
        }
    }
}

static void tab_changed_event(lv_event_t *e)
{
    /* Change scroll time animations. Triggered when a tab button is clicked */
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        set_tab_group();
    }
}

static void scroll_begin_event(lv_event_t *e)
{
    /* Change scroll time animations. Triggered when a tab button is clicked */
    if (lv_event_get_code(e) == LV_EVENT_SCROLL_BEGIN) {
        lv_anim_t *a = lv_event_get_param(e);
        if (a) {
            a->time = 300;
        }
    }
}
