# esp8266_dehumidifier
Dehumidifier controller with ESP8266

# Introduction

This embedded project is aiming to add control logic to a primitive dehumidifier like Hauser DH-400. The circuit can be mounted inside the housing of the Dehumidifier and implements the following functionalities:
* Measures temperature and humidity
* Can control the ooperation of the unit manuallly or automatically via setting a maximum desired humidity level.
* It provides visual indication and allows control of the operation via a JQUERY based web GUI. 


# Build instructions

This is an Arduino sketch that is supposed to be compiled and downloaded by Arduino for ESP8266. See installation instructions here: https://github.com/esp8266/Arduino

The sketch is using two additional libraries. The SimpleTimer is used to trigger periodic events and Adafruit's HTU21DF library is used for commmunicating with the humifity sensor. It is important that the HTU21DF library had to be extended, because the SDA and SCL pins are configurable in ESP8266. So use the fork of the library from the location referred in the git submodules.

## Configuration

1. The sketch contains an array of SSID/password pairs that you shall update to match your Wifi network setup.
2. You can change the IO pins easily via the SDA_PIN, SCL_PIN, RELAY_PIN  and CONTAINER_PIN macros.


