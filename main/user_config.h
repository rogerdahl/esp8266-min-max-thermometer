# pragma once

#define DEBUG_ON
// #define MQTT_DEBUG_ON

// DEBUG OPTIONS
#if defined(DEBUG_ON)
#define INFO(format, ...) os_printf(format, ##__VA_ARGS__)
#else
#define INFO(format, ...)
#endif
