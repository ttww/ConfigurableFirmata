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
*/

#ifndef CounterFirmata_h
#define CounterFirmata_h

#include <ConfigurableFirmata.h>
#include "FirmataFeature.h"

#define MAX_COUNTERS 2 // maximal 8
#define COUNTER_CONFIG 0
#define STEPPER_STEP 1

// CHANGE to trigger the interrupt whenever the pin changes value
// RISING to trigger when the pin goes from low to high,
// FALLING for when the pin goes from high to low.

// Config Command (COUNTER_CONFIG):
// ==================================================================
// [pin][config][sumTimeHighByte][sumTimeLowByte]([pin][config]...)
// Config:
// 0x00 = OFF
// 0x01 = CHANGE
// 0x02 = RISING
// 0x03 = FALLING
// 0x11 = CHANGE with PULLUP
// 0x12 = RISING with PULLUP
// 0x13 = FALLING with PULLUP
//
// [sumTimeHighByte][sumTimeLowByte]
// Time in ms before report and reset the counter
//
// Query Command (COUNTER_QUERY)
// ==================================================================
// [counterBits][resetBits]
// counterBits:  Bit 0 = report counter 0, bit 1 for counter 1....
// resetBits:    Reset after reporting. Next automatic report after
//               sumTime ms
//
// Reply Message (COUNTER_RESPONSE)
// ==================================================================
// [counterBits][first counter Big-Endian]([second counter...])]
// If more than one counter is reported, we start with the lowest counter.
//
class CounterFirmata: public FirmataFeature
{
  public:
    boolean handlePinMode(byte pin, int mode);
    void handleCapability(byte pin);
    boolean handleSysex(byte command, byte argc, byte *argv);
    void update();
    void reset();
  private:
    unsigned long count[MAX_COUNTERS];	// 4 bytes
    byte counterPins[MAX_COUNTERS];
    byte numCounters;
};

#endif
