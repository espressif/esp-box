// Copyright 2018 Espressif Systems (Shanghai) PTE LTD 
// All rights reserved.

#ifndef _RING_BUF_H_
#define _RING_BUF_H_
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include <stdint.h>

typedef enum {
    BUFFER_INPUT,
    BUFFER_PROCESS,
    BUFFER_OUTPUT
} BufferTag;

typedef struct RingBuf {
    uint8_t* p_o;        /**< Original pointer */
    uint8_t* volatile p_r;   /**< Read pointer */
    uint8_t* volatile p_w;   /**< Write pointer */
    volatile int32_t fill_cnt;  /**< Number of filled slots */
    int32_t size;       /**< Buffer size */
    int32_t block_size;
    uint8_t use_ready;  /* availability of data in buffer, depends on fill_cnt */
    BufferTag tag;
    xSemaphoreHandle can_read;
    xSemaphoreHandle can_write;
    xSemaphoreHandle mux;
    size_t total_sz;
    int abort;
    int _doneWrite;  //to prevent infinite blocking for buffer read

    void (*releaseReadWait) (struct RingBuf *rb);
} RingBuf;

//TODO: make the ringbuf methods as ringbuf "class member function" to prevent abuse
struct RingBuf *rb_init(BufferTag tag, int32_t size, int32_t block_size, xSemaphoreHandle share_mux);
void rb_abort(struct RingBuf *rb, int val);
void rb_reset(struct RingBuf *rb);
int32_t rb_available(struct RingBuf *r);
int  rb_read(struct RingBuf *r, uint8_t *buf, int len, TickType_t ticks_to_wait);
int rb_write(struct RingBuf *r, uint8_t *buf, int len, TickType_t ticks_to_wait);
int isDoneWrite(struct RingBuf *rb);
void rb_unint(RingBuf *rb);
#endif
