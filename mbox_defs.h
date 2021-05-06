/* Copyright (C) 2021 Tevofy Technology LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").
 */
#pragma once
#ifndef MBOX_DEFS_H
#define MBOX_DEFS_H
    static const uint32_t PACK_INT_PIN = 15;

//If you want handle System Exclusive message, enable this #define otherwise comment out it.
//#define USBH_MIDI_SYSEX_ENABLE

/* The physical UART port */
#define _MIDI_SERIAL_PORT Serial

/* The debug serial port */
#define _DEBUG_SERIAL_PORT swSer

/* No midi out from murobox (serial side) */
#define MBOX_NO_MIDI_OUT 0

/* No Serial MIDI, so debug message and be printed. Used for debug */
#define MBOX_NO_SERIAL_MIDI 0

#define HOST_ONLY 0

#define MIDI_PLAYER 1

#define PRINT_MIDI_LIST 0

#define USE_MIDI 1

#define MD_MIDI_DEBUG 1

#define PRINT_NOTE 0

#define USBH_MIDI_SYSEX_ENABLE 1

#define USE_WIFI 1

#define MUROUSBFAT 1

#endif
