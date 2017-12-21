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
  //DEBUG_PRINT("Counter: CMD=");
  //DEBUG_PRINTLN(command);
  //DEBUG_PRINT("   argc=");
  //DEBUG_PRINTLN(argc);

// 0x00 = CHANGE
// 0x01 = RISING
// 0x02 = FALLING
// 0x03 = CHANGE with PULLUP
// 0x04 = RISING with PULLUP
// 0x05 = FALLING with PULLUP

  if (command == COUNTER_CONFIG) {
 
		unsigned long ms = millis();
		
  	if (numCounters == MAX_COUNTERS) return false;	// no more counters left
  	
		#define PIN_IDX     0
		#define CONFIG_IDX  1
		#define MS_HIGH_IDX 2
		#define MS_LOW_IDX  3

		counterPins[numCounters]  = argv[PIN_IDX];
		isrCount[numCounters]     = 0;
		lastCount[numCounters]    = 0;
		msToReset[numCounters]    = argv[MS_HIGH_IDX] << 8 | argv[MS_LOW_IDX];
		lastMs[numCounters]       = ms;
		
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
		
	//	interrupts();

		numCounters++;
		
		// Resync all counters to the same reporting base. This allow us to report
		// counters with the same interval in one update() call.
		for (byte i = 0; i < numCounters; i++) {
 			lastMs[i] = ms;
		}
		
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

// [counterBits][first counter Big-Endian, 4 bytes unsigned integer]([second counter...])]
void CounterFirmata::sendCounters()
{
	if (numCounters == 0) return;
	
	byte counterBit = 1;
	byte counterBits = 0;

	// Check if we need to report something:
	unsigned long ms = millis();
  for (byte i = 0; i < numCounters; i++) {
  	if (ms - lastMs[i] > msToReset[i]) counterBits |= counterBit;
		counterBit <<= 1;
  }
  if (counterBits == 0) return;	// nothing to report
  
  Firmata.write(START_SYSEX);
  Firmata.write(COUNTER_RESPONSE);
  Firmata.write(counterBits);
	
	counterBit = 1;
  for (byte i = 0; i < numCounters; i++) {
  	if (counterBits & counterBit) {	// Faster than below if statement
  	//if (ms - lastMs[i] > msToReset[i]) {
  			lastMs[i] += msToReset[i];

				noInterrupts();
				unsigned long c = isrCount[i];
				isrCount[i] = 0;
				interrupts();

				lastCount[i] = c;

        Firmata.write(  (c >> 24) & 0xff);
        Firmata.write(  (c >> 16) & 0xff);
        Firmata.write(  (c >>  8) & 0xff);
        Firmata.write(   c        & 0xff );
  	}
  	
  	counterBit <<= 1;
  }	// for
  
  Firmata.write(END_SYSEX);

}

/*==============================================================================
 * LOOP()
 *============================================================================*/
void CounterFirmata::update()
{
  sendCounters();
}
