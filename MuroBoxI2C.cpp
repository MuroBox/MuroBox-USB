/* Copyright (C) 2021 Tevofy Technology LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").
 */
#include "MuroBoxI2C.hpp"
#include <Arduino.h>
#include "MuroBoxUtility.hpp"

#include <Wire.h>

    static const uint32_t SDA_PIN = 5;
static const uint32_t SCL_PIN = 4;


//const int16_t I2C_SLAVE = 0x08;

MuroBoxI2C M_I2C;

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent() {
#if 0
  Wire.write("hello\n"); // respond with message of 6 bytes
  // as expected by master
#endif

  // Read clear the interrupt.
  //digitalWrite(PACK_INT_PIN, HIGH);   // Active LOW
}

void MuroBoxI2C::MuroBoxI2C_Notify_INT() {
  //digitalWrite(PACK_INT_PIN, LOW);   // Active LOW
}

void MuroBoxI2C::MuroBoxI2CSetup() {


  Wire.begin(SDA_PIN, SCL_PIN); // Master
}
