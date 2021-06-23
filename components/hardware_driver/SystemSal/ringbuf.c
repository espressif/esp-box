// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

/**
* \file
*   Ring Buffer library
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "ringbuf.h"
#include "esp_log.h"
#include "esp_err.h"
#include "EspAudioAlloc.h"

#define RB_TAG "RINGBUF"

/* function signatures */
static void rb_releaseReadWait(RingBuf *rb);

/**
* \brief init a RingBuf object
* \param r pointer to a RingBuf object
* \param buf pointer to a byte array
* \param size size of buf
* \param block_size is size of data as block
* \return 0 if successfull, otherwise failed
*/
RingBuf *rb_init(BufferTag tag, int32_t size, int32_t block_size, xSemaphoreHandle share_mux)
{
    RingBuf *r;
    unsigned char *buf;
    if (size < 2) return NULL;

    if (size % block_size != 0) return NULL;
    r = malloc(sizeof(RingBuf));
    configASSERT(r);
    buf = EspAudioAlloc(1, size);
    configASSERT(buf);

    r->p_o = r->p_r = r->p_w = buf;
    r->fill_cnt = 0;
    r->size = size;
    r->block_size = block_size;
    r->tag = tag;
    r->_doneWrite = 0;
    vSemaphoreCreateBinary(r->can_read);
    vSemaphoreCreateBinary(r->can_write);
    if (share_mux == NULL) {
        share_mux = xSemaphoreCreateMutex();
    }
    r->mux = share_mux;

    r->total_sz = 0;
    r->abort = 0;

    r->releaseReadWait = rb_releaseReadWait;
    return r;
}

void rb_unint(RingBuf *rb)
{
    free(rb->p_o);
    rb->p_o = NULL;
    vSemaphoreDelete(rb->can_read);
    rb->can_read = NULL;
    vSemaphoreDelete(rb->can_write);
    rb->can_write = NULL;
    vSemaphoreDelete(rb->mux);
    rb->mux = NULL;
    free(rb);
    rb = NULL;
}

void rb_reset(RingBuf *rb)
{
    if (rb == NULL)
        return;
    rb->p_r = rb->p_w = rb->p_o;
    rb->fill_cnt = 0;
    rb->_doneWrite = 0;
    rb_abort(rb, 0);
}

/*
 * @brief: get the number of empty bytes available in the buffer
 */
int32_t rb_available(RingBuf *r)
{
    return (r->size - r->fill_cnt);
}

int rb_read(RingBuf *r, uint8_t *buf, int buf_len, TickType_t ticks_to_wait)
{
    int read_size, remainder = 0;
    int total_read_size = 0;

    xSemaphoreTake(r->mux, portMAX_DELAY);

    while (buf_len) {
        if (r->fill_cnt < buf_len) {
            if (r->_doneWrite == 0) {
                remainder = r->fill_cnt % 4;
            } else {
                remainder = 0;
            }
            read_size = r->fill_cnt - remainder;
        } else {
            read_size = buf_len;
        }
        if ((r->p_r + read_size) > (r->p_o + r->size)) {
            int rlen1 = r->p_o + r->size - r->p_r;
            int rlen2 = read_size - rlen1;
            memcpy(buf, r->p_r, rlen1);
            memcpy(buf + rlen1, r->p_o, rlen2);
            r->p_r = r->p_o + rlen2;
        } else {
            memcpy(buf, r->p_r, read_size);
            r->p_r = r->p_r + read_size;
        }

        buf_len -= read_size;
        r->fill_cnt -= read_size;
        total_read_size += read_size;
        buf += read_size;

        if (buf_len == 0) {
            break;
        }

        xSemaphoreGive(r->mux);
        if (!r->_doneWrite && !r->abort) {
            if (xSemaphoreTake(r->can_read, ticks_to_wait) != pdTRUE) {
                goto out;
            }
        }
        if (r->abort == 1) {
            total_read_size = -1;
            goto out;
        }
        if ((r->_doneWrite == 1)
            && (r->fill_cnt == 0)) {
            goto out;
        }
        xSemaphoreTake(r->mux, portMAX_DELAY);
    }

    xSemaphoreGive(r->mux);
out:
    if (total_read_size > 0) {
        xSemaphoreGive(r->can_write);
    }
    if (r->_doneWrite == 1 && total_read_size == 0) {
        total_read_size = -2;
    }
    return total_read_size;
}

int rb_write(RingBuf *r, uint8_t *buf, int buf_len, TickType_t ticks_to_wait)
{
    int write_size = 0;
    int total_write_size = 0;

    xSemaphoreTake(r->mux, portMAX_DELAY);

    while (buf_len) {
        if ((r->size - r->fill_cnt) < buf_len) {
            write_size = r->size - r->fill_cnt;
        } else {
            write_size = buf_len;
        }
        if ((r->p_w + write_size) > (r->p_o + r->size)) {
            int wlen1 = r->p_o + r->size - r->p_w;
            int wlen2 = write_size - wlen1;
            memcpy(r->p_w, buf, wlen1);
            memcpy(r->p_o, buf + wlen1, wlen2);
            r->p_w = r->p_o + wlen2;
        } else {
            memcpy(r->p_w, buf, write_size);
            r->p_w = r->p_w + write_size;
        }

        buf_len -= write_size;
        r->fill_cnt += write_size;
        total_write_size += write_size;
        buf += write_size;

        if (buf_len == 0) {
            break;
        }

        xSemaphoreGive(r->mux);
        // if (r->_doneWrite) {
        //     return write_size > 0 ? write_size : -2;
        // }
        if (r->abort == 1) {
            goto out;
        }
        if (xSemaphoreTake(r->can_write, ticks_to_wait) != pdTRUE) {
            goto out;
        }

        xSemaphoreTake(r->mux, portMAX_DELAY);
    }

    xSemaphoreGive(r->mux);
out:
    if (total_write_size != 0 ) {
        xSemaphoreGive(r->can_read);
    }
    if (r->_doneWrite) {
        return -2;
    }
    return total_write_size;
}

void rb_abort(RingBuf *rb, int val)
{
    if (rb == NULL)
        return;
    rb->abort = val;
    xSemaphoreGive(rb->can_read);
    xSemaphoreGive(rb->can_write);
    xSemaphoreGive(rb->mux);
}

/*
 * @brief: Notify the buffer reader to give up reading if the buffer is empty
 * @param:
 * @return: none
 * @note: setting the _doneWrite flag is not enough to release the wait
 *        as the reader may have already been waiting when the flag was not set
 * @note: the major difference between rb_abort and rb_releaseReadWait is that
 *        the reader will give up reading once upon buffer aborted while it will
 *        read until the buffer becomes empty if only releaseReadWait
 */
static void rb_releaseReadWait(struct RingBuf *rb)
{
    if (rb == NULL)
        return;
    rb->_doneWrite = 1;
    xSemaphoreGive(rb->can_read);
}

uint8_t *rb_getBufHead(struct RingBuf *rb)
{
    if (rb == NULL)
        return NULL;
    return rb->p_o;
}

int32_t rb_isFull(struct RingBuf *rb)
{
    if (rb == NULL)
        return 0;
    return (rb->size == rb->fill_cnt);
}

int isDoneWrite(struct RingBuf *rb)
{
    if (rb == NULL)
        return -1;
    return (rb->_doneWrite);
}
