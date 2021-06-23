// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
// #include "freertos/semphr.h"

// #include "lwip/sockets.h"
// #include "lwip/err.h"
// #include "lwip/sys.h"

#include "esp_log.h"
#include "lock.h"
#include <stdio.h>
#include <stdlib.h>

#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#define LOCK_TAG  "MUTEX_LOCK"


/* @brief: create a new mutex
 * @param:
 * @return: 0 -- successed; 1 -- failed;
 */
// xSemaphoreHandle mutex_init(void)
// {
//     xSemaphoreHandle xReturn = xSemaphoreCreateMutex();
    
//     configASSERT(xReturn);
//     return xReturn;
// }
/* @brief: create a new Binary
 * @param:
 * @return: 0 -- successed; 1 -- failed;
 */
// xSemaphoreHandle semaphore_init(void)
// {
//     xSemaphoreHandle xReturn = NULL;
//     vSemaphoreCreateBinary(xReturn);
    
//     configASSERT(xReturn);
//     return xReturn;
// }

// /* @brief: lock a mutex
//  * @param: mutex -- the mutex to lock
//  * @return: none
//  */
// void mutex_lock(xSemaphoreHandle pxMutex)
// {
//    // ESP_LOGI(LOCK_TAG, "Taking lock [[[%p]]] %s\r\n", pxMutex, __func__);
//     while (xSemaphoreTake(pxMutex, portMAX_DELAY) != pdPASS);
//    // ESP_LOGI(LOCK_TAG, "[[[%p]]] taken %s\r\n", pxMutex, __func__);
// }

// /* @brief: unlock a mutex
//  * @param: mutex -- the mutex to unlock
//  * @return: none
//  */
// void mutex_unlock(xSemaphoreHandle pxMutex)
// {
//     xSemaphoreGive(pxMutex);
//    // ESP_LOGI(LOCK_TAG, "Release lock [[[%p]]] %s\r\n", pxMutex, __func__);
// }

// /* @brief: delete a semaphore
//  * @param: mutex -- the mutex to delete
//  * @return: none
//  */
// void mutex_destroy(xSemaphoreHandle pxMutex)
// {
//     vQueueDelete(pxMutex);
// }
