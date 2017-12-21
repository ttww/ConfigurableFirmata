/*
  Copyright (C) 2006-2008 Hans-Christoph Steiner.  All rights reserved.
  Copyright (C) 2010-2011 Paul Stoffregen.  All rights reserved.
  Copyright (C) 2009 Shigeru Kobayashi.  All rights reserved.
  Copyright (C) 2013 Norbert Truchsess. All rights reserved.
  Copyright (C) 2009-2016 Jeff Hoefs.  All rights reserved.
  Copyright (C) 2017-2018 Thomas Welsch.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  See file LICENSE.txt for further informations on licensing terms.

  Last updated by Thomas Welsch: December 19rd, 2017
*/

#define SERIAL_DEBUG
#include "utility/firmataDebug.h"


#include <ConfigurableFirmata.h>
#include "CounterFirmata.h"
//#include "utility/FirmataStepper.h"

boolean CounterFirmata::handlePinMode(byte pin, int mode)
{
  if (mode == PIN_MODE_COUNTER) {
      if (IS_PIN_INTERRUPT(pin)) {
      	// The final initialisation is done via the config command below:
        pinMode(PIN_TO_DIGITAL(pin), INPUT);
	      return true;
  	  }
  }
  return false;
}

void CounterFirmata::handleCapability(byte pin)
{
  if (IS_PIN_INTERRUPT(pin)) {
    Firmata.write(PIN_MODE_COUNTER);
    Firmata.write(MAX_COUNTERS);
  }
}

/*==============================================================================
 * SYSEX-BASED commands
 *============================================================================*/

volatile unsigned long isrCount[MAX_COUNTERS];	// 4 bytes
//unsigned long          lastCount[MAX_COUNTERS];	// 4 bytes
//unsigned long          lastMs[MAX_COUNTERS];	// 4 bytes
//unsigned short         msToReset[MAX_COUNTERS];	// 4 bytes
//byte                   counterPins[MAX_COUNTERS];
//byte                   numCounters;

void counterIRQ0() { isrCount[0]++; }
void counterIRQ1() { isrCount[1]++; }
void counterIRQ2() { isrCount[2]++; }
void counterIRQ3() { isrCount[3]++; }
void counterIRQ4() { isrCount[4]++; }
void counterIRQ5() { isrCount[5]++; }
void counterIRQ6() { isrCount[6]++; }
void counterIRQ7() { isrCount[7]++; }

boolean CounterFirmata::handleSysex(byte command, byte argc, byte *argv)
{
  DEBUG_PRINT("Counter: CMD=");
  DEBUG_PRINTLN(command);
  DEBUG_PRINT("   argc=");
  DEBUG_PRINTLN(argc);

// 0x00 = CHANGE
// 0x01 = RISING
// 0x02 = FALLING
// 0x03 = CHANGE with PULLUP
// 0x04 = RISING with PULLUP
// 0x05 = FALLING with PULLUP

  if (command == COUNTER_CONFIG) {
 
  	if (numCounters == MAX_COUNTERS) return false;	// no more counters left
  	
    DEBUG_PRINTLN(" Config !");
    for (byte i=0; i<argc; i++) {
		  DEBUG_PRINT(i);
		  DEBUG_PRINT(": ");
		  DEBUG_PRINTLN(argv[i]);
		}
		
		#define PIN_IDX     0
		#define CONFIG_IDX  1
		#define MS_HIGH_IDX 2
		#define MS_LOW_IDX  3

		counterPins[numCounters]  = argv[PIN_IDX];
		isrCount[numCounters]     = 0;
		lastCount[numCounters]    = 0;
		msToReset[numCounters]    = argv[MS_HIGH_IDX] << 8 | argv[MS_LOW_IDX];
		lastMs[numCounters]       = millis();
		

		DEBUG_PRINT("Pin     =  ");
		DEBUG_PRINTLN(counterPins[numCounters] );
		DEBUG_PRINT("MS     =  ");
		DEBUG_PRINTLN(msToReset[numCounters] );

		void (*irqFunc)(void) = NULL;
		switch (numCounters) {
			case 0: irqFunc = &counterIRQ0;		break;
			case 1: irqFunc = &counterIRQ1;		break;
			case 2: irqFunc = &counterIRQ2;		break;
			case 3: irqFunc = &counterIRQ3;		break;
			case 4: irqFunc = &counterIRQ4;		break;
			case 5: irqFunc = &counterIRQ5;		break;
			case 6: irqFunc = &counterIRQ6;		break;
			case 7: irqFunc = &counterIRQ7;		break;
		}
		
		byte mappedPin = PIN_TO_DIGITAL(argv[PIN_IDX]);
		
		switch (argv[CONFIG_IDX]) {
			case 0x00:	// CHANGE
						pinMode(mappedPin, INPUT);
						attachInterrupt(mappedPin, irqFunc, CHANGE);
						break;
			case 0x01:	// RISING
						pinMode(mappedPin, INPUT);
						attachInterrupt(mappedPin, irqFunc, RISING);
						break;
			case 0x02:	// FALLING
						pinMode(mappedPin, INPUT);
						attachInterrupt(mappedPin, irqFunc, FALLING);
						break;
			case 0x03:	// CHANGE with PULLUP
						pinMode(mappedPin, INPUT_PULLUP);
						attachInterrupt(mappedPin, irqFunc, CHANGE);
						break;
			case 0x04:	// RISING with PULLUP
						pinMode(mappedPin, INPUT_PULLUP);
						attachInterrupt(mappedPin, irqFunc, RISING);
						break;
			case 0x05:	// FALLING with PULLUP
						pinMode(mappedPin, INPUT_PULLUP);
						attachInterrupt(mappedPin, irqFunc, FALLING);
						break;
		}	// switch
		
		interrupts();

		numCounters++;
		
    return true;
  }

  if (command == COUNTER_QUERY) {
	}
	
  DEBUG_PRINTLN(" Counter: Wrong command!");
  return false;
}

/*==============================================================================
 * SETUP()
 *============================================================================*/

void CounterFirmata::reset()
{
  for (byte i = 0; i < numCounters; i++) {
  	detachInterrupt(PIN_TO_DIGITAL(counterPins[numCounters]));
  }
  numCounters = 0;
}
/*==============================================================================
 * LOOP()
 *============================================================================*/
void CounterFirmata::update()
{
	if (numCounters == 0) return;
	
  for (byte i = 0; i < numCounters; i++) {
  	if (millis() - lastMs[i] > msToReset[i]) {
  			lastMs[i] += msToReset[i];
  DEBUG_PRINT("CounterUpdate ");
  DEBUG_PRINTLN(isrCount[i]);
  	}
  }
  
//  if (numSteppers > 0) {
//    // if one or more stepper motors are used, update their position
//    for (byte i = 0; i < MAX_STEPPERS; i++) {
//      if (stepper[i]) {
//        bool done = stepper[i]->update();
//        // send command to client application when stepping is complete
//        if (done) {
//          Firmata.write(START_SYSEX);
//          Firmata.write(STEPPER_DATA);
//          Firmata.write(i);
//          Firmata.write(END_SYSEX);
//        }
//      }
//    }
//  }
}
