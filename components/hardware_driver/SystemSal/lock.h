// Copyright 2018 Espressif Systems (Shanghai) PTE LTD 
// All rights reserved.

#ifndef _LOCK_H_
#define _LOCK_H_

typedef void * xSemaphoreHandle;

xSemaphoreHandle mutex_init(void);

xSemaphoreHandle semaphore_init(void);

void mutex_lock(xSemaphoreHandle pxMutex);

void mutex_unlock(xSemaphoreHandle pxMutex);

void mutex_destroy(xSemaphoreHandle pxMutex);

#endif
