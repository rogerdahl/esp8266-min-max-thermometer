// Based on various snippets in the public domain.

#include "driver/gpio.h"
#include "os.h"
#include "rom/ets_sys.h"
#include "xtensa/xtruntime.h"

#include "onewire.h"

// For dev board
//#define ONEWIRE_PIN 14ULL
// For deploying on a bare-bones board, like the ESP-01
#define ONEWIRE_PIN 2ULL


void release();
void lowPulse(u16 lengthUsec);

// Configure a GPIO pin for use in the onewire protocol. The pin is open
// drain with pull-up.
void onewire_init() {
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT_OD;
  io_conf.pin_bit_mask = 1ULL << ONEWIRE_PIN;
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 1;
  gpio_config(&io_conf);

  release();
}

// Release the output. If nothing on the bus is pulling it down, it will be high
// due to pull-up.
void release() {
  //  INFO("Release\n");
  gpio_set_direction(ONEWIRE_PIN, GPIO_MODE_OUTPUT_OD);
  gpio_set_level(ONEWIRE_PIN, 1);
}

void lowPulse(u16 lengthUsec) {
  gpio_set_direction(ONEWIRE_PIN, GPIO_MODE_OUTPUT_OD);
  gpio_set_level(ONEWIRE_PIN, 0);
  os_delay_us(lengthUsec);
  release();
}

int getLevel() {
  //  INFO("getLevel: %d\n", level);
  gpio_set_direction(ONEWIRE_PIN, GPIO_MODE_INPUT);
  int level = gpio_get_level(ONEWIRE_PIN);
  return level;
}

// Reset the search state
void onewire_init_search_state(struct onewire_search_state *state) {
  state->lastDiscrepancy = -1;
  state->lastDeviceFlag = false;
  os_memset(state->address, 0, sizeof(state->address));
}

// Perform the onewire reset function.  We will wait up to 250uS for
// the bus to come high, if it doesn't then it is broken or shorted
// and we return a 0;
// Returns 1 if a device asserted a presence pulse, 0 otherwise.
u32 onewire_reset() {
  //  INFO("onewire_reset()\n");
  u32 result;
  u8 retries = 125;
  release();
  // Wait for the bus to get high (which it should because of the pull-up
  // resistor).
  do {
    if (--retries == 0) {
      return 0;
    }
    os_delay_us(2);
  } while (!getLevel());
  // Transmit the reset pulse by pulling the bus low for at least 480 us.
  lowPulse(500);
  // Release the bus, and wait, then check for a presence pulse, which should
  // start 15-60 us after the reset, and will last 60-240 us.
  // So 65us after the reset the bus must be high.
  os_delay_us(65);
  result = !getLevel();
  //  INFO("Reset result: %d\n", result);
  // After sending the reset pulse, the master (we) must wait at least another
  // 480 us.
  os_delay_us(490);
  return result;
}

// Write a byte. The writing code uses the active drivers to raise the
// pin high, if you need power after the write (e.g. DS18S20 in
// parasite power mode) then set 'power' to 1, otherwise the pin will
// go tri-state at the end of the write to avoid heating in a short or
// other mishap.
void onewire_write_byte(u8 v, u32 power) {
  uint32_t savedLevel = XTOS_DISABLE_ALL_INTERRUPTS;
  u8 bitMask;
  for (bitMask = 0x01; bitMask; bitMask <<= 1) {
    onewire_write_bit((bitMask & v) ? 1 : 0);
  }
  if (power) {
    release();
  }
  XTOS_RESTORE_INTLEVEL(savedLevel);
}

// Write a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
void onewire_write_bit(u32 v) {
  uint32_t savedLevel = XTOS_DISABLE_ALL_INTERRUPTS;
  if (v) {
    lowPulse(10);
    os_delay_us(55);
  } else {
    lowPulse(65);
    os_delay_us(5);
  }
  XTOS_RESTORE_INTLEVEL(savedLevel);
}

u8 onewire_read_byte() {
  uint32_t savedLevel = XTOS_DISABLE_ALL_INTERRUPTS;
  u8 bitMask;
  u8 r = 0;
  for (bitMask = 0x01; bitMask; bitMask <<= 1) {
    if (onewire_read_bit())
      r |= bitMask;
  }
  XTOS_RESTORE_INTLEVEL(savedLevel);
  return r;
}

u32 onewire_read_bit() {
  // Nested XTOS_DISABLE_ALL_INTERRUPTS appears to work as expected.
  uint32_t savedLevel = XTOS_DISABLE_ALL_INTERRUPTS;
  u32 r;
  lowPulse(3);
  os_delay_us(10);
  r = getLevel();
  os_delay_us(53);
  XTOS_RESTORE_INTLEVEL(savedLevel);
  return r;
}

/* pass array of 8 bytes in */
u32 onewire_search(struct onewire_search_state *state) {
  //  INFO("onewire_search() %d\n", 0);
  // If last search returned the last device (no conflicts).
  if (state->lastDeviceFlag) {
    onewire_init_search_state(state);
    return false;
  }

  if (!onewire_reset()) {
    // Reset the search and return fault code.
    onewire_init_search_state(state);
    return ONEWIRE_SEARCH_NO_DEVICES;
  }

  // issue the search command
  onewire_write_byte(ONEWIRE_SEARCH_ROM, 0);

  u8 search_direction;
  s32 last_zero = -1;

  // Loop through all 8 bytes = 64 bits
  s32 id_bit_index;
  for (id_bit_index = 0; id_bit_index < 8 * ROM_BYTES; id_bit_index++) {
    const u32 rom_byte_number = id_bit_index / BITS_PER_BYTE;
    const u32 rom_byte_mask = 1 << (id_bit_index % BITS_PER_BYTE);

    // Read a bit and its complement
    const u32 id_bit = onewire_read_bit();
    const u32 cmp_id_bit = onewire_read_bit();

    // Line high for both reads means there are no slaves on the 1wire bus.
    if (id_bit == 1 && cmp_id_bit == 1) {
      // Reset the search and return fault code.
      onewire_init_search_state(state);
      return ONEWIRE_SEARCH_NO_DEVICES;
    }

    // No conflict for current bit: all devices coupled have 0 or 1
    if (id_bit != cmp_id_bit) {
      // Obviously, we continue the search using the same bit as all the devices
      // have.
      search_direction = id_bit;
    } else {
      // if this discrepancy is before the Last Discrepancy
      // on a previous next then pick the same as last time
      if (id_bit_index < state->lastDiscrepancy) {
        search_direction =
            ((state->address[rom_byte_number] & rom_byte_mask) > 0);
      } else {
        // if equal to last pick 1, if not then pick 0
        search_direction = (id_bit_index == state->lastDiscrepancy);
      }

      // if 0 was picked then record its position in LastZero
      if (search_direction == 0) {
        last_zero = id_bit_index;
      }
    }

    // set or clear the bit in the ROM byte rom_byte_number with mask
    // rom_byte_mask
    if (search_direction == 1) {
      state->address[rom_byte_number] |= rom_byte_mask;
    } else {
      state->address[rom_byte_number] &= ~rom_byte_mask;
    }

    // For the current bit position, write the bit we choose to use in the
    // current search.
    // Any devices that don't match are disabled.
    onewire_write_bit(search_direction);
  }

  state->lastDiscrepancy = last_zero;

  // check for last device
  if (state->lastDiscrepancy == -1) {
    state->lastDeviceFlag = true;
  }

  if (crc8(state->address, 7) != state->address[7]) {
    // Reset the search and return fault code.
    onewire_init_search_state(state);
    return ONEWIRE_SEARCH_CRC_INVALID;
  }

  return ONEWIRE_SEARCH_FOUND;
}

// Do a ROM select
void onewire_select(const u8 *rom) {
  u8 i = 0;
  onewire_write_byte(ONEWIRE_MATCH_ROM, 0); // Choose ROM
  for (i = 0; i < 8; i++) {
    onewire_write_byte(rom[i], 0);
  }
}

void onewire_rom_skip() {
onewire_write_byte(ONEWIRE_SKIP_ROM, 0); // Skip ROM
}

// Compute a Dallas Semiconductor 8 bit CRC directly.
// this is much slower, but much smaller, than the lookup table.
u8 crc8(const u8 *addr, u8 len) {
  u8 crc = 0;
  u8 i;
  while (len--) {
    u8 inbyte = *addr++;
    for (i = 8; i; i--) {
      u8 mix = (crc ^ inbyte) & 0x01;
      crc >>= 1;
      if (mix)
        crc ^= 0x8C;
      inbyte >>= 1;
    }
  }
  return crc;
}
