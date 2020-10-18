//#include "freertos/FreeRTOS.h"
#include "os.h"
#include <esp_heap_caps.h>

#include <cfloat>
#include <string>
#include <vector>

// Including iostream causes some kind of infinite recursion stack allocation havoc.
//#include <iostream>

#include "int_types.h"
#include "user_config.h"
#include "ntp.h"
#include "temperature_tracker.h"

class MinMaxTemp {
public:
  explicit MinMaxTemp(std::string &s) { periodStr = s; }
  //  s8 TEST[1024];
  std::string periodStr;
  float minTemp = FLT_MAX_EXP;
  std::string minTime;
  float maxTemp = FLT_MIN_EXP;
  std::string maxTime;
};

std::vector<MinMaxTemp> minMaxVec;

// Update max/min temp for current period if current is higher/lower.
// Period names don't have to be unique. A new period is created if the
// one provided in the call in different from the period that was added last.
void registerTemp(float tempCelcius) {
  if (!haveTime()) {
    INFO("Ignored temperature registration. Don't have an NTP time yet\n");
    return;
  }

  auto periodStr = std::string(getCurrentLocalDate());
  size_t freeSize = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  INFO("Current free heap: %d bytes\n", freeSize);

//  if (minMaxVec.empty() || minMaxVec.back().periodStr != periodStr) {
    INFO("Adding new MinMaxTemp. periodStr=\"%s\"\n", periodStr.c_str());
    minMaxVec.push_back(MinMaxTemp(periodStr));
//  }

  auto &cur = minMaxVec.back();

  while (freeSize < 5 * 1024 and !minMaxVec.empty()) {
    INFO("Getting low on heap. Dropping oldest MinMaxTemp. curSize=%d\n",
         minMaxVec.size());
    // pop_front()
    minMaxVec.erase(minMaxVec.begin());
    freeSize = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  }

  if (tempCelcius < cur.minTemp) {
    INFO("New minTemp: %f -> %f\n", cur.minTemp, tempCelcius);
    cur.minTemp = tempCelcius;
    cur.minTime = getCurrentLocalTime();
  }
  if (tempCelcius > cur.maxTemp) {
    INFO("New maxTemp: %f -> %f\n", cur.maxTemp, tempCelcius);
    cur.maxTemp = tempCelcius;
    cur.maxTime = getCurrentLocalTime();
  }
}

size_t getMinMaxCount() { return minMaxVec.size(); }

// Get the min and max values for a period as JSON.
void getMinMaxLine(s8 *lineBuf, size_t maxLen, size_t lineIdx) {
  auto &mm = minMaxVec[lineIdx];
  snprintf(lineBuf, maxLen,
           "{ "
           "\"period\": \"%s\", "
           "\"minTime\": \"%s\", "
           "\"minTemp\": \"%.02f\", "
           "\"maxTime\": \"%s\", "
           "\"maxTemp\": \"%.02f\""
           " }",
           mm.periodStr.c_str(), mm.minTime.c_str(), mm.minTemp,
           mm.maxTime.c_str(), mm.maxTemp);
}
