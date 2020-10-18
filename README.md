# ESP8266 thermometer

- Current temperature shows in large LED display
- Min and max temperatures, date and time of day recorded for the last several months
- Temperature records downloadable as JSON over HTTP for display or further processing
- Date and time synchronized from online time servers (NTP)

## Parts

- ESP-01 board
- DS18B20 temperature sensor
- TM1637 based 4-digit 7-segment display
- 3.3V LDO

## Build and flash 

```shell script
$ git clone https://github.com/rogerdahl/esp8266-min-max-thermometer.git

$ . ~/esp/ESP8266_RTOS_SDK/export.sh 

$ idf.py menuconfig

    > Example Connection Information
      > WiFi SSID
      > WiFi Password

$ idf.py fullclean build flash monitor
```
