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
$ cd esp8266-min-max-thermometer
```

Proper timezone support isn't working. Currently must be hard coded by modifying the calculation done in `main/ntp.c`, `getLocalNow()`:

```shell script
$ editor ./main/ntp.c
```

The default GPIO configuration is set up for the ESP-01 board, using the UART RX/TX pins for communicating with the LCD display. So `idf.py monitor` won't work. This just builds and flashes the module, which can then moved over to the hardware board. For development work, use a board that breaks out more pins and modify the settings for `ONEWIRE_PIN` in `onewire.c`, and `TM1637_CLK_PIN` / `TM1637_DIO_PIN` in `tm1637.c`.

```shell script
$ . ~/esp/ESP8266_RTOS_SDK/export.sh 

$ idf.py menuconfig

    > Example Connection Information
      > WiFi SSID
      > WiFi Password

$ idf.py fullclean build flash
```
