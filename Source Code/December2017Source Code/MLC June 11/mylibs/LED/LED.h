/*
||
|| @file LED.h
|| @version 1.1
|| @author Alexander Brevig
|| @contact alexanderbrevig@gmail.com
||
|| @description
|| | Provide an easy way of making/using LEDs
|| #
||
|| @license
|| | Copyright (c) 2009 Alexander Brevig. All rights reserved.
|| | This code is subject to AlphaLicence.txt
|| | alphabeta.alexanderbrevig.com/AlphaLicense.txt
---------------------( COPY )---------------------
This libraries .h file has been updated by Terry King to make it compatible with Ardino 1.0x 

(Example 1.03) and also earlier versions like 0023

#include "WProgram.h"  
...has been replaced by:

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

terry@yourduino.com
02/07/2012
-----------------( END COPY )----------------------
|| #
||
*/

#ifndef LED_H
#define LED_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

class LED{
  public:
    LED(uint8_t ledPin);
	bool getState();
    void on();
	void off();
	void toggle();
	void blink(unsigned int time, byte times=1);
	void setValue(byte val);
	void fadeIn(unsigned int time);
	void fadeOut(unsigned int time);
  private:
	bool status;
	uint8_t pin;
};

extern LED DEBUG_LED;

#endif

/*
|| @changelog
|| | 1.1 2009-05-07 - Alexander Brevig : Added blink(uint,byte), requested by: Josiah Ritchie - josiah@josiahritchie.com
|| | 1.1 2009-04-07 - Alexander Brevig : Altered API
|| | 1.0 2009-04-17 - Alexander Brevig : Initial Release
|| #
*/