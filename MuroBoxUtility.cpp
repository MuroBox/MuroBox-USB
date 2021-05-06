/* Copyright (C) 2021 Tevofy Technology LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").
 */
#include "MuroBoxUtility.hpp"
#include <Arduino.h>
#include <stdarg.h>
#include <ESPSoftwareSerial.h>

    ESPSoftwareSerial swSer;
ConditionalPrinter printer;

ConditionalPrinter::ConditionalPrinter() : m_enable(false) {
  _MIDI_SERIAL_PORT.setDebugOutput(false);
  _DEBUG_SERIAL_PORT.begin(115200, SWSERIAL_8N1, 16, false, 256);
  _DEBUG_SERIAL_PORT.enableIntTx(true);
}

void ConditionalPrinter::setOut(bool enable) {
  m_enable = enable;
  //_DEBUG_SERIAL_PORT.setDebugOutput(m_enable);
}

// Can be disabled by printer.setOut(false);
int ConditionalPrinter::myPrintf(const char * text) 
{
  if (m_enable == 0)
    return 0;
  auto ret = _DEBUG_SERIAL_PORT.print(text);
  _DEBUG_SERIAL_PORT.flush();
  return ret;
}

int ConditionalPrinter::myPrintf(int32_t num) 
{
  if (m_enable == 0)
    return 0;
  auto ret = _DEBUG_SERIAL_PORT.print(num);
  _DEBUG_SERIAL_PORT.flush();
  return ret;
}

int ConditionalPrinter::myPrintf(uint32_t num) 
{
  if (m_enable == 0)
    return 0;
  auto ret = _DEBUG_SERIAL_PORT.print(num);
  _DEBUG_SERIAL_PORT.flush();
  return ret;
}

int ConditionalPrinter::myPrintHEX(int32_t num) {
  if (m_enable == 0)
    return 0;
  auto ret = _DEBUG_SERIAL_PORT.print(num, HEX);
  _DEBUG_SERIAL_PORT.flush();
  return ret;
}

/* prints hex numbers with leading zeroes */
// copyright, Peter H Anderson, Baltimore, MD, Nov, '07
// source: http://www.phanderson.com/arduino/arduino_display.html
int ConditionalPrinter::myPrintHEX(int32_t v, int32_t num_places)
{
  int mask = 0, n, num_nibbles, digit;

  for (n = 1; n <= num_places; n++) {
    mask = (mask << 1) | 0x0001;
  }
  v = v & mask; // truncate v to specified number of places

  num_nibbles = num_places / 4;
  if ((num_places % 4) != 0) {
    ++num_nibbles;
  }
  do {
    digit = ((v >> (num_nibbles - 1) * 4)) & 0x0f;
    //_DEBUG_SERIAL_PORT.print(digit, HEX);
    printer.myPrintHEX(digit);
  }
  while (--num_nibbles);
  return 0;
}
int ConditionalPrinter::myPrintBIN(int32_t num) {
  if (m_enable == 0)
    return 0;
  auto ret = _DEBUG_SERIAL_PORT.print(num, BIN);
  _DEBUG_SERIAL_PORT.flush();
  return ret;
}

int pack_int_midi_removed(int32_t no_use) {
  digitalWrite(PACK_INT_PIN, LOW);   // Active HIGH
  printer.myPrintf(("PACK_INT LOW\r\n"));
}

int pack_int_midi_insert(int32_t no_us) {
  digitalWrite(PACK_INT_PIN, HIGH);   // Active HIGH
  printer.myPrintf(("PACK_INT HIGH\r\n"));
}

int pack_int_init(){
  // initialize INT (INStalled)GPIO as an output low to indicated that MIDI are pluged in to the motherboard
  pinMode(PACK_INT_PIN, OUTPUT);
  pack_int_midi_removed(NULL);
}
