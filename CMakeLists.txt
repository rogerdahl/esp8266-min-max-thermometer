cmake_minimum_required(VERSION 3.17)
project(esp8266_min_max_thermometer)

set(CMAKE_CXX_STANDARD 14)

include_directories(main)

add_executable(esp8266_min_max_thermometer
  main/CMakeLists.txt
  main/component.mk
  main/ds18b20.c
  main/ds18b20.h
  main/http.c
  main/http.h
  main/int_types.h
  main/Kconfig.projbuild
  main/Kconfig.projbuild.bak
  main/main.c
  main/ntp.c
  main/ntp.h
  main/onewire.c
  main/onewire.h
  main/temperature_tracker.cpp
  main/temperature_tracker.h
  main/tm1637.c
  main/tm1637.h
  main/user_config.h)
