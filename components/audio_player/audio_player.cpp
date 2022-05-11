/**
 * @file
 * @version 0.1
 *
 * @copyright Copyright 2021 Espressif Systems (Shanghai) Co. Ltd.
 * @copyright Copyright 2022 Chris Morgan <chmorgan@gmail.com>
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "audio_player.h"

static const char *TAG = "audio";

#if CONFIG_AUDIO_PLAYER_LOG_LEVEL >= 1
#define LOGI_1(FMT, ...) \
    ESP_LOGI(TAG, "[1] " FMT, ##__VA_ARGS__)
#else
#define LOGI_1(FMT, ...) {}
#endif

#if CONFIG_AUDIO_PLAYER_LOG_LEVEL >= 2
#define LOGI_2(FMT, ...) \
    ESP_LOGI(TAG, "[2] " FMT, ##__VA_ARGS__)
#else
#define LOGI_2(FMT, ...) {}
#endif

#if CONFIG_AUDIO_PLAYER_LOG_LEVEL >= 3
#define LOGI_3(FMT, ...) \
    ESP_LOGI(TAG, "[3] " FMT, ##__VA_ARGS__)
#define COMPILE_3(x) x
#else
#define LOGI_3(FMT, ...) {}
#define COMPILE_3(x) {}
#endif

typedef enum {
    AUDIO_PLAYER_REQUEST_NONE = 0,
    AUDIO_PLAYER_REQUEST_PAUSE,              /**< pause playback */
    AUDIO_PLAYER_REQUEST_RESUME,             /**< resumed paused playback */
    AUDIO_PLAYER_REQUEST_PLAY,               /**< initiate playing a new file */
    AUDIO_PLAYER_REQUEST_STOP,               /**< stop playback */
    AUDIO_PLAYER_REQUEST_MAX
} audio_player_event_type_t;

typedef struct {
    audio_player_event_type_t type;

    // valid if type == AUDIO_PLAYER_EVENT_TYPE_PLAY
    FILE* fp;
} audio_player_event_t;

/* **************** PRIVATE STRUCT **************** */
#if defined(CONFIG_AUDIO_PLAYER_ENABLE_MP3)
#include "mp3dec.h"

typedef struct {
    char header[3];     /*!< Always "TAG" */
    char title[30];     /*!< Audio title */
    char artist[30];    /*!< Audio artist */
    char album[30];     /*!< Album name */
    char year[4];       /*!< Char array of year */
    char comment[30];   /*!< Extra comment */
    char genre;         /*!< See "https://en.wikipedia.org/wiki/ID3" */
} __attribute__((packed)) mp3_id3_header_v1_t;

typedef struct {
    char header[3];     /*!< Always "ID3" */
    char ver;           /*!< Version, equals to3 if ID3V2.3 */
    char revision;      /*!< Revision, should be 0 */
    char flag;          /*!< Flag byte, use Bit[7..5] only */
    char size[4];       /*!< TAG size */
} __attribute__((packed)) mp3_id3_header_v2_t;
#endif

#if defined(CONFIG_AUDIO_PLAYER_ENABLE_WAV)
typedef struct {
    // The "RIFF" chunk descriptor
    uint8_t ChunkID[4];
    int32_t ChunkSize;
    uint8_t Format[4];
    // The "fmt" sub-chunk
    uint8_t Subchunk1ID[4];
    int32_t Subchunk1Size;
    int16_t AudioFormat;
    int16_t NumChannels;
    int32_t SampleRate;
    int32_t ByteRate;
    int16_t BlockAlign;
    int16_t BitsPerSample;
} wav_header_t;

typedef struct {
    // The "data" sub-chunk
    uint8_t SubchunkID[4];
    int32_t SubchunkSize;
} wav_subchunk_header_t;
#endif

/* **************** AUDIO CALLBACK **************** */
static audio_player_cb_t s_audio_cb = NULL;
static void *audio_cb_usrt_ctx = NULL;
static i2s_port_t port;
static audio_player_state_t state = AUDIO_PLAYER_STATE_IDLE;

static audio_player_mute_fn mute_control_fn;

typedef enum {
    FILE_TYPE_UNKNOWN,
#if defined(CONFIG_AUDIO_PLAYER_ENABLE_MP3)
    FILE_TYPE_MP3,
#endif
#if defined(CONFIG_AUDIO_PLAYER_ENABLE_WAV)
    FILE_TYPE_WAV
#endif
} FILE_TYPE;

audio_player_state_t audio_player_get_state() {
    return state;
}

esp_err_t audio_player_callback_register(audio_player_cb_t call_back, void *user_ctx)
{
    ESP_RETURN_ON_FALSE(esp_ptr_executable((void*)call_back), ESP_ERR_INVALID_ARG,
        TAG, "Not a valid call back");

    s_audio_cb = call_back;
    audio_cb_usrt_ctx = user_ctx;

    return ESP_OK;
}

const char* event_to_string(audio_player_callback_event_t event) {
    switch(event) {
    case AUDIO_PLAYER_CALLBACK_EVENT_IDLE:
        return "AUDIO_PLAYER_CALLBACK_EVENT_IDLE";
    case AUDIO_PLAYER_CALLBACK_EVENT_COMPLETED_PLAYING_NEXT:
        return "AUDIO_PLAYER_CALLBACK_EVENT_COMPLETED_PLAYING_NEXT";
    case AUDIO_PLAYER_CALLBACK_EVENT_PLAYING:
        return "AUDIO_PLAYER_CALLBACK_EVENT_PLAYING";
    case AUDIO_PLAYER_CALLBACK_EVENT_PAUSE:
        return "AUDIO_PLAYER_CALLBACK_EVENT_PAUSE";
    case AUDIO_PLAYER_CALLBACK_EVENT_UNKNOWN_FILE_TYPE:
        return "AUDIO_PLAYER_CALLBACK_EVENT_UNKNOWN_FILE_TYPE";
    case AUDIO_PLAYER_CALLBACK_EVENT_UNKNOWN:
        return "AUDIO_PLAYER_CALLBACK_EVENT_UNKNOWN";
    }

    return "unknown event";
}

static audio_player_callback_event_t state_to_event(audio_player_state_t state) {
    audio_player_callback_event_t event = AUDIO_PLAYER_CALLBACK_EVENT_UNKNOWN;

    switch(state) {
        case AUDIO_PLAYER_STATE_IDLE:
            event = AUDIO_PLAYER_CALLBACK_EVENT_IDLE;
            break;
        case AUDIO_PLAYER_STATE_PAUSE:
            event = AUDIO_PLAYER_CALLBACK_EVENT_PAUSE;
            break;
        case AUDIO_PLAYER_STATE_PLAYING:
            event = AUDIO_PLAYER_CALLBACK_EVENT_PLAYING;
            break;
    };

    return event;
}

static void dispatch_callback(audio_player_callback_event_t event) {
    LOGI_1("event '%s'", event_to_string(event));

    if (esp_ptr_executable((void*)s_audio_cb)) {
        audio_player_cb_ctx_t ctx = {
            .audio_event = event,
            .user_ctx = audio_cb_usrt_ctx,
        };
        s_audio_cb(&ctx);
    }
}

static void set_state(audio_player_state_t new_state) {
    if(state != new_state) {
        state = new_state;
        audio_player_callback_event_t event = state_to_event(new_state);
        dispatch_callback(event);
    }
}

/* **************** AUDIO DECODE **************** */
static QueueHandle_t audio_event_queue = NULL;

typedef struct {
    int sample_rate;
    uint32_t bits_per_sample;
    uint32_t channels;
} format;

/**
 * Decoded audio data ready for playback
 *
 * Fields in this structure are expected to be updated
 * upon each cycle of the decoder, as the decoder stores
 * audio data to be played back.
 */
typedef struct {
    /**
     * NOTE: output_samples is flushed each decode cycle
     *
     * NOTE: the decode format determines how to convert samples to frames, ie.
     * whether these samples are stero or mono samples and what the bits per sample are
     */
    uint8_t *samples;

    /** capacity of samples */
    size_t samples_capacity;

    /**
     * 2x samples_capacity to allow for in-place conversion of
     * mono to stereo
     */
    size_t samples_capacity_max;

    /**
     * Number of frames in samples,
     * Note that each frame consists of 'fmt.channels' number of samples,
     * for example for stereo output the number of samples is 2x the
     * frame count.
     */
    size_t frame_count;

    format fmt;
} decode_data;

static decode_data audio_data;

typedef enum {
    DECODE_STATUS_CONTINUE,         /*< data remaining, call decode again */
    DECODE_STATUS_NO_DATA_CONTINUE, /*< data remaining but none in this call */
    DECODE_STATUS_DONE,             /*< no data remaining to decode */
    DECODE_STATUS_ERROR             /*< unrecoverable error */
} DECODE_STATUS;

static const size_t BYTES_IN_WORD = 2;
static const size_t BITS_PER_BYTE = 8;

#if defined(CONFIG_AUDIO_PLAYER_ENABLE_MP3)

typedef struct {
    // Constants below
    uint8_t *data_buf;

    /** number of bytes in data_buf */
    size_t data_buf_size;

    // Values that change at runtime are below

    /**
     * Total bytes in data_buf,
     * not the number of bytes remaining after the read_ptr
     */
    size_t bytes_in_data_buf;

    /** Pointer to read location in data_buf */
    uint8_t *read_ptr;

    // set to true if the end of file has been reached
    bool eof_reached;
} mp3_instance;

static HMP3Decoder mp3_decoder;
static mp3_instance mp3_data;

static bool is_mp3(FILE *fp) {
    bool is_mp3_file = false;

    fseek(fp, 0, SEEK_SET);

    // see https://en.wikipedia.org/wiki/List_of_file_signatures
    uint8_t magic[3];
    if(sizeof(magic) == fread(magic, 1, sizeof(magic), fp)) {
        if((magic[0] == 0xFF) &&
            (magic[1] == 0xFB))
        {
            is_mp3_file = true;
        } else if((magic[0] == 0xFF) &&
                  (magic[1] == 0xF3))
        {
            is_mp3_file = true;
        } else if((magic[0] == 0xFF) &&
                  (magic[1] == 0xF2))
        {
            is_mp3_file = true;
        } else if((magic[0] == 0x49) &&
                  (magic[1] == 0x44) &&
                  (magic[2] == 0x33)) /* 'ID3' */
        {
            fseek(fp, 0, SEEK_SET);

            /* Get ID3 head */
            mp3_id3_header_v2_t tag;
            if (sizeof(mp3_id3_header_v2_t) == fread(&tag, 1, sizeof(mp3_id3_header_v2_t), fp)) {
                if (memcmp("ID3", (const void *) &tag, sizeof(tag.header)) == 0) {
                    is_mp3_file = true;
                }
            }
        }
    }

    // seek back to the start of the file to avoid
    // missing frames upon decode
    fseek(fp, 0, SEEK_SET);

    return is_mp3_file;
}

/**
 * @return true if data remains, false on error or end of file
 */
DECODE_STATUS decode_mp3(FILE *fp, decode_data *pData, mp3_instance *pInstance) {
    MP3FrameInfo frame_info;

    size_t unread_bytes = pInstance->bytes_in_data_buf - (pInstance->read_ptr - pInstance->data_buf);

    /* somewhat arbitrary trigger to refill buffer - should always be enough for a full frame */
    if (unread_bytes < 1.25 * MAINBUF_SIZE && !pInstance->eof_reached) {
        uint8_t *write_ptr = pInstance->data_buf + unread_bytes;
        size_t free_space = pInstance->data_buf_size - unread_bytes;

    	/* move last, small chunk from end of buffer to start,
           then fill with new data */
        memmove(pInstance->data_buf, pInstance->read_ptr, unread_bytes);

        size_t nRead = fread(write_ptr, 1, free_space, fp);

        pInstance->bytes_in_data_buf = unread_bytes + nRead;
        pInstance->read_ptr = pInstance->data_buf;

        if (nRead == 0)
        {
            pInstance->eof_reached = true;
        }

        LOGI_2("pos %ld, nRead %d, eof %d", ftell(fp), nRead, pInstance->eof_reached);

        unread_bytes = pInstance->bytes_in_data_buf;
    }

    LOGI_3("data_buf 0x%p, read 0x%p", pInstance->data_buf, pInstance->read_ptr);

    if(unread_bytes == 0) {
        LOGI_1("unread_bytes == 0, status done");
        return DECODE_STATUS_DONE;
    }

    /* Find MP3 sync word from read buffer */
    int offset = MP3FindSyncWord(pInstance->read_ptr, unread_bytes);

    LOGI_2("unread %d, total %d, offset %d",
            unread_bytes, pInstance->bytes_in_data_buf, offset);

    if (offset >= 0) {
        COMPILE_3(int starting_unread_bytes = unread_bytes);
        uint8_t *read_ptr = pInstance->read_ptr + offset; /*!< Data start point */
        unread_bytes -= offset;
        LOGI_3("read 0x%p, unread %d", read_ptr, unread_bytes);
        int mp3_dec_err = MP3Decode(mp3_decoder, &read_ptr, (int*)&unread_bytes, (int16_t *) pData->samples, 0);

        pInstance->read_ptr = read_ptr;

        if(mp3_dec_err == ERR_MP3_NONE) {
            /* Get MP3 frame info */
            MP3GetLastFrameInfo(mp3_decoder, &frame_info);

            pData->fmt.sample_rate = frame_info.samprate;
            pData->fmt.bits_per_sample = frame_info.bitsPerSample;
            pData->fmt.channels = frame_info.nChans;

            pData->frame_count = (frame_info.outputSamps / frame_info.nChans);

            LOGI_3("mp3: channels %d, sr %d, bps %d, frame_count %d, processed %d",
                pData->fmt.channels,
                pData->fmt.sample_rate,
                pData->fmt.bits_per_sample,
                frame_info.outputSamps,
                starting_unread_bytes - unread_bytes);
        } else {
            if(mp3_dec_err == ERR_MP3_MAINDATA_UNDERFLOW)
            {
                // underflow indicates MP3Decode should be called again
                LOGI_1("underflow read ptr is 0x%p", read_ptr);
                return DECODE_STATUS_NO_DATA_CONTINUE;
            } else {
                LOGI_1("status error %d", mp3_dec_err);
                return DECODE_STATUS_ERROR;
            }
        }
    } else {
        // drop an even count of words
        size_t words_to_drop = unread_bytes / BYTES_IN_WORD;
        size_t bytes_to_drop = words_to_drop * BYTES_IN_WORD;

        // shift the read_ptr to drop the bytes in the buffer
        pInstance->read_ptr += bytes_to_drop;

        /* Sync word not found in frame. Drop data that was read until a word boundary */
        ESP_LOGE(TAG, "MP3 sync word not found, dropping %d bytes", bytes_to_drop);
    }

    return DECODE_STATUS_CONTINUE;
}
#endif

#if defined(CONFIG_AUDIO_PLAYER_ENABLE_WAV)

typedef struct {
    wav_header_t header;
} wav_instance;

static wav_instance wav_data;

/**
 * @param fp
 * @param pInstance - Values can be considered valid if true is returned
 * @return true if file is a wav file
 */
static bool is_wav(FILE *fp, wav_instance *pInstance) {
    fseek(fp, 0, SEEK_SET);

    size_t bytes_read = fread(&pInstance->header, 1, sizeof(wav_header_t), fp);
    if(bytes_read != sizeof(wav_header_t)) {
        return false;
    }

    wav_header_t *wav_head = &pInstance->header;
    if((NULL == strstr((char *)wav_head->ChunkID, "RIFF")) ||
        (NULL == strstr((char*)wav_head->Format, "WAVE"))
      )
    {
        return false;
    }

    // decode chunks until we find the 'data' one
    wav_subchunk_header_t subchunk;
    while(true) {
        bytes_read = fread(&subchunk, 1, sizeof(wav_subchunk_header_t), fp);
        if(bytes_read != sizeof(wav_subchunk_header_t)) {
            return false;
        }

        if(memcmp(subchunk.SubchunkID, "data", 4) == 0)
        {
            break;
        } else {
            // advance beyond this subchunk, it could be a 'LIST' chunk with file info or some other unhandled subchunk
            fseek(fp, subchunk.SubchunkSize, SEEK_CUR);
        }
    }

    LOGI_2("sample_rate=%d, channels=%d, bps=%d",
            wav_head->SampleRate,
            wav_head->NumChannels,
            wav_head->BitsPerSample);

    return true;
}

/**
 * @return true if data remains, false on error or end of file
 */
DECODE_STATUS decode_wav(FILE *fp, decode_data *pData, wav_instance *pInstance) {
    // read an even multiple of frames that can fit into output_samples buffer, otherwise
    // we would have to manage what happens with partial frames in the output buffer
    size_t bytes_per_frame = (pInstance->header.BitsPerSample / BITS_PER_BYTE) * pInstance->header.NumChannels;
    size_t frames_to_read = pData->samples_capacity / bytes_per_frame;
    size_t bytes_to_read = frames_to_read * bytes_per_frame;

    size_t bytes_read = fread(pData->samples, 1, bytes_to_read, fp);

    pData->fmt.channels = pInstance->header.NumChannels;
    pData->fmt.bits_per_sample = pInstance->header.BitsPerSample;
    pData->fmt.sample_rate = pInstance->header.SampleRate;

    if(bytes_read != 0)
    {
        pData->frame_count = (bytes_read / (pInstance->header.BitsPerSample / BITS_PER_BYTE)) / pInstance->header.NumChannels;
    } else {
        pData->frame_count = 0;
    }

    LOGI_2("bytes_per_frame %d, bytes_to_read %d, bytes_read %d, frame_count %d",
            bytes_per_frame, bytes_to_read, bytes_read,
            pData->frame_count);

    return (bytes_read == 0) ? DECODE_STATUS_DONE : DECODE_STATUS_CONTINUE;
}
#endif

static esp_err_t mono_to_stereo(decode_data &adata)
{
    size_t data = adata.frame_count * (audio_data.fmt.bits_per_sample / 8);
    data *= 2;
    // do we have enough space in the output buffer to convert mono to stereo?
    if(data > adata.samples_capacity_max) {
        ESP_LOGE(TAG, "insufficient space in audio_data.samples to convert mono to stereo, need %d, have %d", data, audio_data.samples_capacity);
        return ESP_ERR_NO_MEM;
    }

    size_t new_sample_count = adata.frame_count * 2;

    // convert from back to front to allow conversion in-place
    int16_t *out = (int16_t*)adata.samples + new_sample_count;
    int16_t *in = (int16_t*)adata.samples + adata.frame_count;
    size_t samples = adata.frame_count;
    while(samples) {
        // write right channel
        *out = *in;
        out--;

        // write left channel
        *out = *in;
        out--;

        // move input buffer back and decrement samples
        in--;
        samples--;
    }

    // adjust channels to 2
    adata.fmt.channels = 2;

    return ESP_OK;
}

static esp_err_t aplay_file(FILE *fp)
{
    ESP_LOGI(TAG, "start to decode");

    format i2s_format;
    memset(&i2s_format, 0, sizeof(i2s_format));

    esp_err_t ret = ESP_OK;
    audio_player_event_t audio_event = { .type = AUDIO_PLAYER_REQUEST_NONE, .fp = NULL };

    FILE_TYPE file_type = FILE_TYPE_UNKNOWN;

#if defined(CONFIG_AUDIO_PLAYER_ENABLE_MP3)
    if(is_mp3(fp)) {
        file_type = FILE_TYPE_MP3;
        ESP_LOGI(TAG, "file is mp3");

        // initialize mp3_instance
        mp3_data.bytes_in_data_buf = 0;
        mp3_data.read_ptr = mp3_data.data_buf;
        mp3_data.eof_reached = false;
    }
#endif

#if defined(CONFIG_AUDIO_PLAYER_ENABLE_WAV)
    if(file_type == FILE_TYPE_UNKNOWN)
    {
        if(is_wav(fp, &wav_data)) {
            file_type = FILE_TYPE_WAV;
            ESP_LOGI(TAG, "file is wav");
        }
    }
#endif

    if(file_type == FILE_TYPE_UNKNOWN) {
        ESP_LOGE(TAG, "unknown file type, cleaning up");
        dispatch_callback(AUDIO_PLAYER_CALLBACK_EVENT_UNKNOWN_FILE_TYPE);
        goto clean_up;
    }

    do {
        /* Process audio event sent from other task */
        if (pdPASS == xQueuePeek(audio_event_queue, &audio_event, 0)) {
            LOGI_2("event in queue");
            if (AUDIO_PLAYER_REQUEST_PAUSE == audio_event.type) {
                // receive the pause event to take it off of the queue
                xQueueReceive(audio_event_queue, &audio_event, 0);

                set_state(AUDIO_PLAYER_STATE_PAUSE);

                // wait until an event is received that will cause playback to resume,
                // stop, or change file
                while(1) {
                    xQueuePeek(audio_event_queue, &audio_event, portMAX_DELAY);

                    if((AUDIO_PLAYER_REQUEST_PLAY != audio_event.type) &&
                       (AUDIO_PLAYER_REQUEST_STOP != audio_event.type) &&
                       (AUDIO_PLAYER_REQUEST_RESUME != audio_event.type))
                    {
                        // receive to discard the event
                        xQueueReceive(audio_event_queue, &audio_event, 0);
                    } else {
                        break;
                    }
                }

                if(AUDIO_PLAYER_REQUEST_RESUME == audio_event.type) {
                    // receive to discard the event
                    xQueueReceive(audio_event_queue, &audio_event, 0);
                    continue;
                }

                // else fall out of this condition and let the below logic
                // handle the other event types
            }

            if ((AUDIO_PLAYER_REQUEST_STOP == audio_event.type) ||
                (AUDIO_PLAYER_REQUEST_PLAY == audio_event.type)) {
                ret = ESP_OK;
                goto clean_up;
            } else {
                // receive to discard the event, this event has no
                // impact on the state of playback
                xQueueReceive(audio_event_queue, &audio_event, 0);
                continue;
            }
        }

        set_state(AUDIO_PLAYER_STATE_PLAYING);

        DECODE_STATUS decode_status = DECODE_STATUS_ERROR;

        switch(file_type) {
#if defined(CONFIG_AUDIO_PLAYER_ENABLE_MP3)
            case FILE_TYPE_MP3:
                decode_status = decode_mp3(fp, &audio_data, &mp3_data);
                break;
#endif
#if defined(CONFIG_AUDIO_PLAYER_ENABLE_WAV)
            case FILE_TYPE_WAV:
                decode_status = decode_wav(fp, &audio_data, &wav_data);
                break;
#endif
            case FILE_TYPE_UNKNOWN:
                ESP_LOGE(TAG, "unexpected unknown file type when decoding");
                break;
        }

        // break out and exit if we aren't supposed to continue decoding
        if(decode_status == DECODE_STATUS_CONTINUE)
        {
            // if mono, convert to stereo as es8311 requires stereo input
            // even though it is mono output
            if(audio_data.fmt.channels ==  1) {
                LOGI_3("c == 1, mono -> stereo");
                ret = mono_to_stereo(audio_data);
                if(ret != ESP_OK) {
                    goto clean_up;
                }
            }

            /* Configure I2S clock if the output format changed */
            if ((i2s_format.sample_rate != audio_data.fmt.sample_rate) ||
                    (i2s_format.channels != audio_data.fmt.channels) ||
                    (i2s_format.bits_per_sample != audio_data.fmt.bits_per_sample)) {
                i2s_format = audio_data.fmt;
                LOGI_1("format change: sr=%d, bit=%d, ch=%d",
                        i2s_format.sample_rate,
                        i2s_format.bits_per_sample,
                        i2s_format.channels);
                i2s_channel_t channel_setting = (i2s_format.channels == 1) ? I2S_CHANNEL_MONO : I2S_CHANNEL_STEREO;
                ret = i2s_set_clk(port,
                            i2s_format.sample_rate,
                            i2s_format.bits_per_sample,
                            channel_setting);
                ESP_GOTO_ON_ERROR(ret, clean_up, TAG, "i2s_set_clk");
            }

            /**
             * Block until all data has been accepted into the i2s driver, however
             * the i2s driver has been configured with a buffer to allow for the next round of
             * audio decoding to occur while the previous set of samples is finishing playback, in order
             * to ensure playback without interruption.
             */
            size_t i2s_bytes_written = 0;
            size_t bytes_to_write = audio_data.frame_count * audio_data.fmt.channels * (i2s_format.bits_per_sample / 8);
            LOGI_2("c %d, bps %d, bytes %d, frame_count %d",
                audio_data.fmt.channels,
                i2s_format.bits_per_sample,
                bytes_to_write,
                audio_data.frame_count);

            i2s_write(port, audio_data.samples, bytes_to_write, &i2s_bytes_written, portMAX_DELAY);
            if(bytes_to_write != i2s_bytes_written) {
                ESP_LOGE(TAG, "to write %d != written %d", bytes_to_write, i2s_bytes_written);
            }
        } else if(decode_status == DECODE_STATUS_NO_DATA_CONTINUE)
        {
            LOGI_2("no data");
        } else { // DECODE_STATUS_DONE || DECODE_STATUS_ERROR
            LOGI_1("breaking out of playback");
            break;
        }
    } while (true);

clean_up:
    return ret;
}

static void audio_task(void *pvParam)
{
    audio_player_event_t audio_event;

    while (true) {
        // pull items off of the queue until we run into a PLAY request
        while(true) {
            // zero delay in the case where we are playing as we want to
            // send an event indicating either
            // PLAYING -> IDLE (IDLE) or PLAYING -> PLAYING (COMPLETED PLAYING NEXT)
            // and thus don't want to block until the next request comes in
            // in the case when there are no further requests pending
            int delay = (state == AUDIO_PLAYER_STATE_PLAYING) ? 0 : portMAX_DELAY;

            int retval = xQueuePeek(audio_event_queue, &audio_event, delay);
            if (pdPASS == retval) { // item on the queue, process it
                xQueueReceive(audio_event_queue, &audio_event, 0);

                // if the item is a play request, process it
                if(AUDIO_PLAYER_REQUEST_PLAY == audio_event.type) {
                    if(state == AUDIO_PLAYER_STATE_PLAYING) {
                        dispatch_callback(AUDIO_PLAYER_CALLBACK_EVENT_COMPLETED_PLAYING_NEXT);
                    } else {
                        set_state(AUDIO_PLAYER_STATE_PLAYING);
                    }

                    break;
                }
            } else { // no items on the queue
                // if we are playing transition to idle and indicate the transition via callback
                if(state == AUDIO_PLAYER_STATE_PLAYING) {
                    set_state(AUDIO_PLAYER_STATE_IDLE);
                }
            }
        }

        mute_control_fn(AUDIO_PLAYER_UNMUTE);
        esp_err_t ret_val = aplay_file(audio_event.fp);
        if(ret_val != ESP_OK)
        {
            ESP_LOGE(TAG, "aplay_file() %d", ret_val);
        }
        mute_control_fn(AUDIO_PLAYER_MUTE);

        if(audio_event.fp) fclose(audio_event.fp);
    }
}

/* **************** AUDIO PLAY CONTROL **************** */
static esp_err_t audio_send_event(audio_player_event_t event) {
    ESP_RETURN_ON_FALSE(NULL != audio_event_queue, ESP_ERR_INVALID_STATE,
        TAG, "Audio task not started yet");

    BaseType_t ret_val = xQueueSend(audio_event_queue, &event, 0);

    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_ERR_INVALID_STATE,
        TAG, "The last event has not been processed yet");

    return ESP_OK;
}

esp_err_t audio_player_play(FILE *fp)
{
    ESP_LOGI(TAG, "%s", __FUNCTION__);
    audio_player_event_t event = { .type = AUDIO_PLAYER_REQUEST_PLAY, .fp = fp };
    return audio_send_event(event);
}

esp_err_t audio_player_pause(void)
{
    ESP_LOGI(TAG, "%s", __FUNCTION__);
    audio_player_event_t event = { .type = AUDIO_PLAYER_REQUEST_PAUSE, .fp = NULL };
    return audio_send_event(event);
}

esp_err_t audio_player_resume(void)
{
    ESP_LOGI(TAG, "%s", __FUNCTION__);
    audio_player_event_t event = { .type = AUDIO_PLAYER_REQUEST_RESUME, .fp = NULL };
    return audio_send_event(event);
}

esp_err_t audio_player_stop(void)
{
    ESP_LOGI(TAG, "%s", __FUNCTION__);
    audio_player_event_t event = { .type = AUDIO_PLAYER_REQUEST_STOP, .fp = NULL };
    return audio_send_event(event);
}

/* **************** START AUDIO PLAYER **************** */
esp_err_t audio_player_init(i2s_port_t _port,
                            audio_player_mute_fn mute_fn)
{
    BaseType_t task_val;
    port = _port;
    mute_control_fn = mute_fn;

    /* Audio control event queue */
    audio_event_queue = xQueueCreate(4, sizeof(audio_player_event_t));
    ESP_RETURN_ON_FALSE(NULL != audio_event_queue, -1, TAG, "xQueueCreate");

    /** See https://github.com/ultraembedded/libhelix-mp3/blob/0a0e0673f82bc6804e5a3ddb15fb6efdcde747cd/testwrap/main.c#L74 */
    audio_data.samples_capacity = MAX_NCHAN * MAX_NGRAN * MAX_NSAMP;
    audio_data.samples_capacity_max = audio_data.samples_capacity * 2;
    audio_data.samples = (uint8_t*)malloc(audio_data.samples_capacity_max);
    ESP_LOGI(TAG, "samples_capacity %d bytes", audio_data.samples_capacity_max);
    int ret;
    ESP_GOTO_ON_FALSE(NULL != audio_data.samples, ESP_ERR_NO_MEM, cleanup,
        TAG, "Failed allocate output buffer");

#if defined(CONFIG_AUDIO_PLAYER_ENABLE_MP3)
    mp3_data.data_buf_size = MAINBUF_SIZE * 3;
    mp3_data.data_buf = (uint8_t*)malloc(mp3_data.data_buf_size);
    ESP_GOTO_ON_FALSE(NULL != mp3_data.data_buf, ESP_ERR_NO_MEM, cleanup,
        TAG, "Failed allocate mp3 data buffer");

    mp3_decoder = MP3InitDecoder();
    ESP_GOTO_ON_FALSE(NULL != mp3_decoder, ESP_ERR_NO_MEM, cleanup,
        TAG, "Failed create MP3 decoder");
#endif

    task_val = xTaskCreatePinnedToCore(
        (TaskFunction_t)        audio_task,
                                "Audio Task",
                                4 * 1024,
        NULL,
        (UBaseType_t)           configMAX_PRIORITIES - 1,
        (TaskHandle_t * const)  NULL,
                                0);

    ESP_GOTO_ON_FALSE(pdPASS == task_val, ESP_ERR_NO_MEM, cleanup,
        TAG, "Failed create audio task");

    // start muted
    mute_control_fn(AUDIO_PLAYER_MUTE);

    return ESP_OK;

cleanup:
#if defined(CONFIG_AUDIO_PLAYER_ENABLE_MP3)
    if(mp3_decoder) MP3FreeDecoder(mp3_decoder);
    if(mp3_data.data_buf) free(mp3_data.data_buf);
#endif
    if(audio_data.samples) free(audio_data.samples);

    return ret;
}
