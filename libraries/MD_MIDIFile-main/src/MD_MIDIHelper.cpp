/*
  MD_MIDIHelper.cpp - An Arduino library for processing Standard MIDI Files (SMF).
  Copyright (C) 2012 Marco Colli
  All rights reserved.

  See MD_MIDIFile.h for complete comments

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <string.h>
#include <MD_MIDIFile.h>
#include "MD_MIDIHelper.h"

/**
 * \file
 * \brief Main file for helper functions implementation
 */

uint32_t readMultiByte(FIL *f, uint8_t nLen)
// read fixed length parameter from input
{
  uint32_t  value = 0L;
  static uint8_t ch[4];
  FRESULT rc;
  uint16_t br;
  rc = f_read(f, ch, nLen, &br);
  if (rc || !br)
  {
    printer.myPrintf("readMultiByte error\r\n");
    return 0;
  }
  for (uint8_t i=0; i<nLen; i++)
  {
    value = (value << 8) + ch[i];
  }
  
  return(value);
}

uint32_t readVarLen(FIL *f)
// read variable length parameter from input
{
  uint32_t  value = 0;
  uint8_t c[2]; //FATFs f_read buffer requires size of power of 2
  FRESULT rc;
  uint16_t br;

  do
  {
    rc = f_read(f, c, 1, &br);
    if (rc || !br)
      printer.myPrintf("readVarLen\r\n");

    value = (value << 7) + (c[0] & 0x7f);
  }  while (c[0] & 0x80);
  
  return(value);
}

#if DUMP_DATA
void dumpBuffer(uint8_t *p, int len)
// Formatted dump of a buffer of data
{
  for (int i=0; i<len; i++, p++)
  {
    if ((i!=0) && ((i & 0x0f) == 0)) // every 16 characters
      DUMPS("\n");

    DUMPS(" ");
    if (*p<=0xf)
      DUMPX("0", *p)
    else
      DUMPX("", *p);
  }
}
#endif
