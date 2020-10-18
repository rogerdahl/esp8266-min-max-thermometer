#pragma once

#include "rom/ets_sys.h"
#include "driver/gpio.h"

#include "int_types.h"

#define ONEWIRE_SEARCH_ROM 0xF0
#define ONEWIRE_READ_ROM 0x33
#define ONEWIRE_MATCH_ROM 0x55
#define ONEWIRE_SKIP_ROM 0xCC
#define ONEWIRE_ALARMSEARCH 0xEC

#define ROM_BYTES 8
#define BITS_PER_BYTE 8

#define ONEWIRE_SEARCH_FOUND 0x00
#define ONEWIRE_SEARCH_NO_DEVICES 0x01
#define ONEWIRE_SEARCH_CRC_INVALID 0x02

// State for a 1wire bus search.
struct onewire_search_state
{
  u8 address[8];
  s32 lastDiscrepancy;
  u32 lastDeviceFlag;
};

void onewire_init();
void onewire_init_search_state(struct onewire_search_state* state);

u32 onewire_reset();

void onewire_write_byte(u8 v, u32 power);
void onewire_write_bit(u32 v);
u8 onewire_read_byte();
u32 onewire_read_bit();

u32 onewire_search(struct onewire_search_state* state);
void onewire_select(const u8 rom[8]);
void onewire_rom_skip();

u8 crc8(const u8* addr, u8 len);
