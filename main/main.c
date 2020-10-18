#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp8266/rom_functions.h"
#include "rom/ets_sys.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "nvs_flash.h"

#include <time.h>

#include "int_types.h"
#include "user_config.h"
#include "ds18b20.h"
#include "http.h"
#include "ntp.h"
#include "tm1637.h"
#include "temperature_tracker.h"

const int LED = 2;

os_timer_t read_timer;
uint8_t ucDisplayTaskParams;
DS18B20_Sensors sensors;


void blinkLedOnce();
void displayTemp(float tempCelcius);
void tempDisplayTask(void *pvParameters);


void app_main() {
  ds18b20_setup(&sensors);
  service_init();
  init_ntp();

  tm1637Init();
  tm1637SetBrightness(8);

  TaskHandle_t xHandle = NULL;
  xTaskCreate(tempDisplayTask, "tempDisplayTask", 4096, &ucDisplayTaskParams,
              tskIDLE_PRIORITY, &xHandle);
  configASSERT(xHandle);

  //  while (true) {
  //    float tempCelcius = ds18b2_get_temperature();
  //    INFO("Temp: %f\n", tempCelcius);
  //    displayTemp(tempCelcius);
  ////    vTaskDelay(pdMS_TO_TICKS(1000));
  //  }

  //  for (;;) {
  //  }

  //  while (true) {
  //    tm1637SetBrightness(8);
  //    u8 c[4] = {0};
  //    tm1637DisplaySegments(c);
  //
  //    ds18b20_request_temperatures(&sensors);
  //    float tmp = ds18b20_read(&sensors, 0);
  //    INFO("Temp %d\n", (int)(tmp * 10000));
  //  }
}

void tempDisplayTask(void *pvParameters) {
//  int q = 0;
//  u32 timePeriodBufSize = 32;
//  s8 timePeriodBuf[timePeriodBufSize];
//
  while (true) {
//    q += 1;
//    snprintf(timePeriodBuf, timePeriodBufSize, "%d", q );
    float tempCelcius = ds18b2_get_temperature(&sensors);
    registerTemp(tempCelcius);
    displayTemp(tempCelcius);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void displayTemp(float tempCelcius) {
  u8 s[4];
  int v = (int)(tempCelcius * 10);

  for (int i = 0; i < 4; ++i) {
    if (i == 1) {
      s[i] = 0x10; // space
    } else {
      s[i] = (u8)(v % 10);
      v /= 10;
    }
  }
  tm1637DisplaySegments(s);
}

// LED

void blinkLedOnce() {
  //  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO5);
  //  PIN_PULLDWN_DIS(PERIPHS_IO_MUX_GPIO5_U);
  // PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO5_U);

  gpio_set_level(LED, 1);
  os_delay_us(0xffff); // 100000
  gpio_set_level(LED, 0);

  //  gpio_set_level(BIT2, 0, BIT2, 0);
  //  os_delay_us(100000);
  //  gpio_output_set(0, BIT2, 0, BIT2);
}
