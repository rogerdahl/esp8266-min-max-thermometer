#pragma once

#include "stddef.h"
#include "int_types.h"

typedef struct _sensors {
  u8 *addresses;
  u8 count;
  size_t length;
  // not currently used
  u8 parasite_mode;
} DS18B20_Sensors;

void ds18b20_setup(DS18B20_Sensors*);
float ds18b2_get_temperature(DS18B20_Sensors*);
