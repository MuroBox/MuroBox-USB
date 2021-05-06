/* Copyright (C) 2021 Tevofy Technology LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").
 */
#ifndef MUROBOX_UTILITY_H
#define MUROBOX_UTILITY_H
#include <Arduino.h>
#include "mbox_defs.h"

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

    class ConditionalPrinter {
public:
  ConditionalPrinter();
  void setOut(bool enable);
  int myPrintf(const char *text);
  int myPrintf(int32_t num);
  int myPrintf(uint32_t num);
  int myPrintHEX(int32_t num);
  int myPrintHEX(int32_t v, int32_t num_places);
  int myPrintBIN(int32_t num);
private:
  bool m_enable;
};

int pack_int_midi_removed(int32_t no_us);
int pack_int_midi_insert(int32_t no_us);
int pack_int_init();

extern ConditionalPrinter printer;

enum player_state
{
  RESERVED,
  RELEASE,
  PRESSED,
  CLOCKWISE,
  C_CLOCKWISE
};

#endif
