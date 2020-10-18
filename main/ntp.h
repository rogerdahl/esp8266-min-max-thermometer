#pragma once

#include "esp_sntp.h"

#ifdef __cplusplus
extern "C" {
#endif

void init_ntp();
bool haveTime();
s8* getCurrentLocalDateTime();
s8* getCurrentLocalDate();
s8* getCurrentLocalTime();

#ifdef __cplusplus
} // extern "C"
#endif
