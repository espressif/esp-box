/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include "string.h"
#include "userconfig.h"
#include "esp_system.h"
#include "esp_heap_caps.h"

#include "esp_log.h"

void *EspAudioAlloc(int n, int size)
{
    void *data =  NULL;

#if IDF_3_0
#if CONFIG_SPIRAM_BOOT_INIT
//    ESP_LOGI("ALLOC", "PSRAM %d", n * size);
    data = heap_caps_malloc(n * size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (data) {
        memset(data, 0, n * size);
    }
#else
//    ESP_LOGI("ALLOC", "D-RAM %d", n * size);
    data = calloc(n, size);
#endif
#else
#if CONFIG_MEMMAP_SPIRAM_ENABLE_MALLOC
//    ESP_LOGI("ALLOC", "PSRAM %d", n * size);
    data = pvPortMallocCaps(n * size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (data) {
        memset(data, 0, n * size);
    }
#else
//    ESP_LOGI("ALLOC", "D-RAM %d", n * size);
    data = calloc(n, size);
#endif
#endif

    return data;
}

void *EspAudioAllocInner(int n, int size)
{
    void *data =  NULL;
//    ESP_LOGI("ALLOC", "INTERNAL %d", n * size);
#if IDF_3_0
    data = heap_caps_malloc(n * size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#else
    data = pvPortMallocCaps(n * size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#endif
    if (data) {
        memset(data, 0, n * size);
    }

    return data;
}

void EspAudioPrintMemory(const char *Tag)
{
#if IDF_3_0
#ifdef CONFIG_SPIRAM_BOOT_INIT
    ESP_LOGI(Tag, "Memory,total:%d, inter:%d, spram:%d, Dram:%d", esp_get_free_heap_size(),
                   heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_32BIT),
                   heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
                   heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
#else
    ESP_LOGI(Tag, "Memory, total:%d, dram:%d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), esp_get_free_heap_size());
#endif
#else
#ifdef CONFIG_MEMMAP_SPIRAM_ENABLE_MALLOC
    ESP_LOGI(Tag, "Memory,total:%d, inter:%d, Dram:%d", xPortGetFreeHeapSizeCaps(MALLOC_CAP_SPIRAM),
                   xPortGetFreeHeapSizeCaps(MALLOC_CAP_INTERNAL), xPortGetFreeHeapSizeCaps(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
#else
    ESP_LOGI(Tag, "Memory,total:%d", esp_get_free_heap_size());
#endif
#endif
}


void EspAudioMemoryShow(const char *Tag, const char *Info, int Line)
{
    ESP_LOGI(Tag, "%d %s Memory,total:%d, inter:%d, spram:%d, Dram:%d", Line, Info, esp_get_free_heap_size(),
                   heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_32BIT),
                   heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
                   heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
}

