#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void registerTemp(float tempCelcius);
size_t getMinMaxCount();
void getMinMaxLine(s8 *lineBuf, size_t maxLen, size_t lineIdx);

#ifdef __cplusplus
} // extern "C"
#endif
