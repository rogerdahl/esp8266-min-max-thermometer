// Driver for the DS18B20 one-wire temperature sensor.
// Based esp826_ds18b20 by Andrej Candale:
// https://github.com/candale/esp826_ds18b20

#include "math.h"

#include "esp8266/rom_functions.h"
#include "nvs.h"
#include "rom/ets_sys.h"
#include "os.h"

#include "int_types.h"
#include "user_config.h"
#include "ds18b20.h"
#include "onewire.h"

#define DS18B20_CONVERT_T 0x44
#define DS18B20_WRITE_SCRATCHPAD 0x4E
#define DS18B20_READ_SCRATCHPAD 0xBE
#define DS18B20_COPY_SCRATCHPAD 0x48
#define DS18B20_READ_EEPROM 0xB8
#define DS18B20_READ_PWRSUPPLY 0xB4

#define DS18B20_TEMP_9_BIT 0x1F  //  9 bit
#define DS18B20_TEMP_10_BIT 0x3F // 10 bit
#define DS18B20_TEMP_11_BIT 0x5F // 11 bit
#define DS18B20_TEMP_12_BIT 0x7F // 12 bit

#define DS18B20_ROM_IDENTIFIER 0x28

#define ADDR_LEN 8
#define DS18B20_GROW_RATIO 2
#define DS18B20_INIT_ADDR_LENGTH 5
#define DS18B20_ADDR_SIZE ADDR_LEN * sizeof(u8)


struct onewire_search_state onewire_state;


int ds18b20_get_all(DS18B20_Sensors *sensors);
void ds18b20_request_temperatures(DS18B20_Sensors *sensors);
float ds18b20_read(DS18B20_Sensors *, u8 target);
u8 ds18b20_set_resolution(DS18B20_Sensors *sensors, u8 target, u8 resolution);
u8 ds18b20_get_resolution(DS18B20_Sensors *sensors, int target);


void ds18b20_setup(DS18B20_Sensors* sensors) {
  onewire_init();

  sensors->addresses = (u8 *)os_zalloc(DS18B20_ADDR_SIZE * DS18B20_INIT_ADDR_LENGTH);
  sensors->count = 0;
  sensors->length = DS18B20_INIT_ADDR_LENGTH;
  sensors->parasite_mode = 0;

  int m = ds18b20_get_all(sensors);
  INFO("Found %d sensors\n", m);
  INFO("Changing resolution to 12 bit\n");

  ds18b20_set_resolution(sensors, 0, DS18B20_TEMP_12_BIT);
}

float ds18b2_get_temperature(DS18B20_Sensors* sensors) {
  // We're not able to generate completely stable onewire signals, so we
  // occasionally get bogus temperature values.
  for (int tries = 0; tries < 10; ++tries) {
    ds18b20_request_temperatures(sensors);
    float tempCelcius = ds18b20_read(sensors, 0);
    if (!isnan(tempCelcius) && -50 <= tempCelcius && tempCelcius <= 50) {
      INFO("Successfully read temperature: %f\n", tempCelcius);
      return tempCelcius;
    }
    INFO("Reading temperature failed. Received \"%f\". Retrying...\n", tempCelcius);
    os_delay_us(1000);
  }
  rom_software_reboot();
  // Just to make the compiler happy.
  return 0.0f;
}

u8 read_scratchpad(u8 *address, u8 *data) {
  u8 i;
  onewire_reset();
  onewire_select(address);
  onewire_write_byte(DS18B20_READ_SCRATCHPAD, 0);

  for (i = 0; i < 9; i++) {
    data[i] = onewire_read_byte();
  }

  return onewire_reset() == 1;
}

u8 is_connected(u8 *address, u8 *data) {
  u8 result = read_scratchpad(address, data);
  return result && crc8(data, 8) == data[8];
}

int ds18b20_get_all(DS18B20_Sensors *sensors) {
  onewire_init_search_state(&onewire_state);

  while (!onewire_state.lastDeviceFlag) {
    u8 search_status = onewire_search(&onewire_state);
    if (search_status == ONEWIRE_SEARCH_NO_DEVICES) {
      return 0;
    }

    if (search_status != ONEWIRE_SEARCH_FOUND) {
      return -1;
    }

    // If it is not a DS18B20
    if (onewire_state.address[0] != DS18B20_ROM_IDENTIFIER) {
      continue;
    }

    os_memcpy(sensors->addresses + (sensors->count) * DS18B20_ADDR_SIZE,
              onewire_state.address, DS18B20_ADDR_SIZE);

    if (sensors->count + 1 >= sensors->length) {
      u8 new_length;
      if (sensors->length * DS18B20_GROW_RATIO > 0xFF) {
        new_length = 0xFF;
      } else {
        new_length = sensors->length * DS18B20_GROW_RATIO;
      }

      sensors->addresses = (u8 *)os_realloc(
          sensors->addresses, sensors->length * DS18B20_ADDR_SIZE +
                                  (new_length * DS18B20_ADDR_SIZE));

      sensors->length = new_length;
    }
    sensors->count++;
  }

  return sensors->count;
}

u8 ds18b20_set_resolution(DS18B20_Sensors *sensors, u8 target, u8 resolution) {
  u8 is_correct =
      (resolution == DS18B20_TEMP_9_BIT || resolution == DS18B20_TEMP_10_BIT ||
       resolution == DS18B20_TEMP_11_BIT || resolution == DS18B20_TEMP_12_BIT);

  if (!is_correct) {
    return 0;
  }

  if (ds18b20_get_resolution(sensors, target) == resolution) {
    return 1;
  }

  u8 *target_addr = sensors->addresses + target * DS18B20_ADDR_SIZE;

  onewire_reset();
  onewire_select(target_addr);

  // Write scratchpad
  onewire_write_byte(DS18B20_WRITE_SCRATCHPAD, sensors->parasite_mode);
  onewire_write_byte(0x00, sensors->parasite_mode); // user byte 1
  onewire_write_byte(0x00, sensors->parasite_mode); // user byte 2
  onewire_write_byte(resolution, sensors->parasite_mode);
  onewire_reset();

  // Persist resolution to EEPROM
  onewire_select(target_addr);
  onewire_write_byte(DS18B20_COPY_SCRATCHPAD, sensors->parasite_mode);
  onewire_reset();

  return 1;
}

u8 ds18b20_get_resolution(DS18B20_Sensors *sensors, int target) {
  u8 data[12];

  u8 *target_addr = sensors->addresses + target * DS18B20_ADDR_SIZE;
  read_scratchpad(target_addr, data);

  return data[4];
}

void ds18b20_request_temperatures(DS18B20_Sensors *sensors) {
  // Tell sensor to prepare data
  onewire_reset();
  onewire_rom_skip();
  onewire_write_byte(DS18B20_CONVERT_T, sensors->parasite_mode);

  // Wait for it to process the data. May take up to 750 ms
  for (int i = 0; i < 790; ++i) {
    //    os_delay_us(790 * 1000);
    os_delay_us(1000);
  }
}

float ds18b20_read(DS18B20_Sensors *sensors, u8 target) {
  u8 *target_addr = sensors->addresses + target * DS18B20_ADDR_SIZE;
  u8 data[12];
  //    u8 i;

  if (!is_connected(target_addr, data)) {
    return NAN;
  }

  read_scratchpad(target_addr, data);

  u32 lsb = data[0];
  u32 msb = data[1];

  // Taken from Arduino DallasTemperature
  s16 raw_temp = (((s16)msb) << 11) | (((s16)lsb) << 3);

  return raw_temp * 0.0078125;
}
