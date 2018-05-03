# ESP8266 Based Load Controller
This project is a sample load controller that uses MQTT and end-to-end AES-128 encryption to allow a Processing (Processing 3) application to control a set of relays to manage control for an attached load.  This design is implemented using an ESP8266 on a NodeMCU breakout Board and a hall-gate sensor for current measurement.

Please note, all new CalPlug designs use the ADE7953 Wattmeter - this uses a legacy measurement device.  Please see: https://github.com/CalPlug/ADE7953-Wattmeter

Other than the ones included in this project, the additional libraries referenced are part of the following: https://github.com/esp8266/Arduino/
