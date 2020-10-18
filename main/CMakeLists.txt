set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_VERBOSE_MAKEFILE on)

set(configSUPPORT_DYNAMIC_ALLOCATION 1)

set(COMPONENT_ADD_INCLUDEDIRS
  .
  /home/dahl/esp/ESP8266_RTOS_SDK/components/esp8266/include/
  /home/dahl/esp/ESP8266_RTOS_SDK/components/lwip/lwip/src/include/
)

set(
  COMPONENT_SRCS
  main.c
  http.c
  ntp.c
  tm1637.c
  ds18b20.c
  onewire.c
  temperature_tracker.cpp
)

register_component()
