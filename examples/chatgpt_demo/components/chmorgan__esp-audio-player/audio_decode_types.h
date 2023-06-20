#pragma once

#include <stdint.h>
#include <stddef.h>

typedef enum {
    DECODE_STATUS_CONTINUE,         /*< data remaining, call decode again */
    DECODE_STATUS_NO_DATA_CONTINUE, /*< data remaining but none in this call */
    DECODE_STATUS_DONE,             /*< no data remaining to decode */
    DECODE_STATUS_ERROR             /*< unrecoverable error */
} DECODE_STATUS;

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


#define BYTES_IN_WORD       2
#define BITS_PER_BYTE       8
