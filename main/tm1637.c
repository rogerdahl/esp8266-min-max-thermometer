#include "driver/gpio.h"
#include "esp8266/eagle_soc.h"
#include "rom/ets_sys.h"

#include "int_types.h"
#include "tm1637.h"

void _tm1637Start();
void _tm1637Stop();
void _tm1637ReadResult();
void _tm1637WriteByte(u8 b);
void _tm1637ClkHigh();
void _tm1637ClkLow();
void _tm1637DioHigh();
void _tm1637DioLow();

// Configuration.

// For dev board
//const int TM1637_CLK_PIN = 4;
//const int TM1637_DIO_PIN = 5;
// For deploying on a bare-bones board, like the ESP-01
const int TM1637_CLK_PIN = 1;
const int TM1637_DIO_PIN = 3;

const char segmentMap[] = {
    0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, // 0-7
    0x7f, 0x6f, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71, // 8-9, A-F
    0x00};

void delay_(int d) { os_delay_us(d * 100); }

void tm1637Init() {
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT_OD;
  io_conf.pin_bit_mask = 1 << TM1637_DIO_PIN | 1 << TM1637_CLK_PIN;
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 1;
  gpio_config(&io_conf);

  // In idle state of the bus, CLK and DIO are high via pull-up. Devices on
  // the bus can pull them up or down without causing conflicts.
  _tm1637ClkHigh();
  _tm1637DioHigh();

  tm1637SetBrightness(8);
}

// Display a decimal number between 0000 and 9999
void tm1637DisplayDecimal(int v, int displaySeparator) {
  u8 digitArr[4];
  for (int i = 0; i < 4; ++i) {
    digitArr[i] = v % 10;
    if (i == 2 && displaySeparator) {
      digitArr[i] |= 1 << 7;
    }
    v /= 10;
  }
  tm1637DisplaySegments(digitArr);
}

// Display a string containing hex digits and spaces
// 0x00 = 0
// 0x0f = F
// 0x10 = space
void tm1637DisplaySegments(const u8 *segmentArr) {
  _tm1637Start();
  _tm1637WriteByte(0x40);
  _tm1637ReadResult();
  _tm1637Stop();

  _tm1637Start();
  _tm1637WriteByte(0xc0);
  _tm1637ReadResult();

  for (int i = 0; i < 4; ++i) {
    _tm1637WriteByte((u8)segmentMap[segmentArr[3 - i]]);
    _tm1637ReadResult();
  }

  _tm1637Stop();
}

// Valid brightness values: 0 - 8.
// 0 = display off.
void tm1637SetBrightness(char brightness) {
  // Brightness command:
  // 1000 0XXX = display off
  // 1000 1BBB = display on, brightness 0-7
  // X = don't care
  // B = brightness
  _tm1637Start();
  _tm1637WriteByte(0x87 + brightness);
  _tm1637ReadResult();
  _tm1637Stop();
}

// Input starts when CLK is high and DIO changes from high to low
void _tm1637Start() {
  _tm1637ClkHigh();
  _tm1637DioHigh();
  delay_(2);
  _tm1637DioLow();
  delay_(2);
}

// Input ends when CLK is high and DIO changes from low to high
void _tm1637Stop() {
  _tm1637ClkLow();
  delay_(2);
  _tm1637DioLow();
  delay_(2);
  _tm1637ClkHigh();
  delay_(2);
  _tm1637DioHigh();
}

void _tm1637ReadResult() {
  _tm1637ClkLow();
  delay_(5);
  // We're cheating here and not actually reading back the response.
  _tm1637ClkHigh();
  delay_(2);
  _tm1637ClkLow();
}

// During input, DIO must only change when CLK is low
void _tm1637WriteByte(u8 b) {
  for (int i = 0; i < 8; ++i) {
    _tm1637ClkLow();
    delay_(3);
    if (b & 0x01) {
      _tm1637DioHigh();
    } else {
      _tm1637DioLow();
    }
    delay_(3);
    b >>= 1;
    _tm1637ClkHigh();
    delay_(3);
  }
}

void _tm1637ClkHigh() { gpio_set_level(TM1637_CLK_PIN, 1); }
void _tm1637ClkLow() { gpio_set_level(TM1637_CLK_PIN, 0); }
void _tm1637DioHigh() { gpio_set_level(TM1637_DIO_PIN, 1); }
void _tm1637DioLow() { gpio_set_level(TM1637_DIO_PIN, 0); }
