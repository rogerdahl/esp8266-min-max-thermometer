#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "nvs_flash.h"

#include <time.h>

#include "int_types.h"
#include "user_config.h"
#include "ntp.h"

time_t now = 0;
s8 strftime_buf[64];
uint8_t ucNtpUpdateTaskParams;
void getLocalNow(struct tm *timeinfo);
void NtpUpdateTask(void *pvParameters);
void time_sync_notification_cb(struct timeval *tv);

/*
  Unable to get timezone working. The basic issue is that `setenv` crashes.
  I'm guessing that the env is not writable due to some setting in menuconfig.

  Crashes: setenv(<anything>);

  Tried:

  // Set timezone to Eastern Standard Time and print local time

  setenv("TZ", "America/Denver", 1);
  tzset();

  if (esp8266::coreVersionNumeric() >= 20700000)
  {
    configTime("CET-1CEST,M3.5.0/02,M10.5.0/03", "pool.ntp.org"); // check
  TZ.h, find your location
  }
  else
  {
    setenv("TZ", "CET-1CEST,M3.5.0/02,M10.5.0/03" , 1);
    configTime(0, 0, "pool.ntp.org");
  }

  ~~

  void getCurrentLocalDateTime(s8* buf, u32 n) {
  struct tm timeinfo = {0};
  localtime_r(&now, &timeinfo);
  strftime(buf, n, "%c", &timeinfo);
  //  return strftime_buf;
  //  INFO( "The current date/time in New York is: %s", strftime_buf);
  }

  ~~

  struct timezone tz={0,0};
  struct timeval tv={0,0};
  settimeofday(&tv, &tz);
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 0);
  tzset();

  ~~

  Also tried doing the setting in main.c.
*/

void init_ntp() {
  INFO("Initializing SNTP\n");

  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "pool.ntp.org");
  sntp_set_time_sync_notification_cb(time_sync_notification_cb);
  sntp_init();

  TaskHandle_t xHandle = NULL;
  xTaskCreate(NtpUpdateTask, "NtpUpdateTask", 2048, &ucNtpUpdateTaskParams,
              tskIDLE_PRIORITY, &xHandle);
  configASSERT(xHandle);
}

bool haveTime() {
  return now != 0;
}

s8 *getCurrentLocalDateTime() {
  // Was unable to get timezone support working (see other notes in this file).
  // So, just doing a quick-n-dirty subtract of 6 hours from UTC to get MDT
  // (which is correct while in daylight saving).
  struct tm timeinfo;
  getLocalNow(&timeinfo);
  strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return strftime_buf;
}

s8 *getCurrentLocalDate() {
  struct tm timeinfo;
  getLocalNow(&timeinfo);
  strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d", &timeinfo);
  return strftime_buf;
}

s8 *getCurrentLocalTime() {
  struct tm timeinfo;
  getLocalNow(&timeinfo);
  strftime(strftime_buf, sizeof(strftime_buf), "%H:%M:%S", &timeinfo);
  return strftime_buf;
}

void NtpUpdateTask(void *pvParameters) {
  while (true) {
    // wait for time to be set
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET &&
           ++retry < retry_count) {
      INFO("Waiting for system time to be set... (%d/%d)\n", retry,
           retry_count);
      vTaskDelay(pdMS_TO_TICKS(1000));
    }

    time(&now);
    localtime_r(&now, &timeinfo);

    INFO("Time received from NTP server: %s\n", getCurrentLocalDateTime());
    vTaskDelay(pdMS_TO_TICKS(60 * 60 * 1000));
  }
}

void getLocalNow(struct tm *timeinfo) {
  time_t mdt = now - 6 * 60 * 60;
  localtime_r(&mdt, timeinfo);
}

void time_sync_notification_cb(struct timeval *tv) {
  INFO("Received notification of time sync event\n");
}
