# Geiger Counter Cajoe

## Overview
This repo contains the ardunio code for an ESP8266 with an SSD1306 OLED display connected to a CAJOE Geiger Counter module. The readings from the module are displayed in the OLED display as well as on a website available via WiFi.

## Configuration
The WiFi manager SSID and KEY can be changed in wifi.h

## Compilation and Installation
To compile, additional libraries are needed. Compilation and upload are done using the Arduino IDE

## Usage
When starting the first time, a wifi manager is launched on the esp8266 that starts an access point and shows the SSID name and KEY in the OLED display. Once connected to the SSID, enter the IP address shown in the DISPLAY in a browser.
On the shown website, select your local wifi network, enter the key for it and save the configuration. The Geiger Counter Cajoe will now restart and connect to your local WiFi. The IP adress and port number is now displayed inthe display and can be used to view the readings in a browser.

## Media
Photos: https://pixel.tchncs.de/p/greygoo/586695111360716092
Video: tba

## Note
The code was hacked together in a rush, and will be changed to compile with Arduino-Makefile eventually
