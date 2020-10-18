#pragma once

#include "int_types.h"

void tm1637Init();
void tm1637DisplayDecimal(int v, int displaySeparator);
void tm1637DisplaySegments(const u8 *segmentArr);
void tm1637SetBrightness(char brightness);
