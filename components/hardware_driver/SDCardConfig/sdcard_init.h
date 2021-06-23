#ifndef _GET_SDCARD_H_
#define _GET_SDCARD_H_

#include "stdio.h"
#include "stdlib.h"

int sd_card_mount(const char* basePath);

int FatfsComboWrite(const void* buffer, int size, int count, FILE* stream);

#endif
