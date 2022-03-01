#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif


void boot_animate_start(void (*fn)(void));


#ifdef __cplusplus
}
#endif
