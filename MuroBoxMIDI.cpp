/* Copyright (C) 2021 Tevofy Technology LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").
 */
#include "MuroBoxMIDI.hpp"
#include <Arduino.h>
#include <MIDI.h>
#include <usbh_midi.h>
#include "MuroBoxUtility.hpp"
#include "MuroBoxUSBHost.hpp"
#include "mbox_defs.h"
#include <queue>
#include <string>
#include <ArduinoJson.h>

    static const uint8_t rxChannel = MIDI_CHANNEL_OMNI;
static const uint8_t txChannel = 1;

/* Create one Muro Box Instance */
MuroBoxMIDI M_MIDI;

#if !MBOX_NO_SERIAL_MIDI
/* This is non-usb (serial) midi drive */
MIDI_CREATE_DEFAULT_INSTANCE();
#endif //!MBOX_NO_SERIAL_MIDI

/* For the MIDI in the USB Host */
extern USB Usb;
//take out namesapce
extern USBH_MIDI USB_Midi;

// This function will be automatically called when a NoteOn is received.
// It must be a void-returning function with the correct parameters,
// see documentation here:
// https://github.com/FortySevenEffects/arduino_midi_library/wiki/Using-Callbacks
#ifdef USBH_MIDI_SYSEX_ENABLE
//SysEx:
std::queue<int> knob_event_queue;
/* Add the event of knob being pressed or rotated by the user to the queue*/
void update_knob_event(int player_state)
{
  knob_event_queue.push(player_state);
}

/* This function handles the sysex messages from the serial poart. The motherboard 
sends sysex messages through serial when there is an event of knob being pressed 
or rotated by the user. In such event, a json string is wrapped inside the sysex messege. 
The json string is being parsed and push the respective action to queue. */

/*
  NOTE: MIDI Universal Real Time SysEx Message Format — SysEx messages start with (hexadecimal) F0, followed by FD, and end with F7.
        Jason Format:
        release: {"type":"knob","action":"release"}
        pressed: {"type":"knob","action":"pressed"}
        lockwise: {"type":"knob","action":"clockwise"}
        c_clockwise: {"type":"knob","action":"c_clockwise"}
*/
static void handle_sysex(byte *sysexmsg, unsigned sizeofsysex)
{
  StaticJsonDocument<256> doc;
  char *concatmsg = (char *)malloc(sizeof(char) * (sizeofsysex - 3 + 1)); //take out 3 control byte(F0,7F,F7) plus '/0'
  if (!concatmsg)
  {
    return;
  }
  if ((sizeofsysex - 3) <= 0)
  {
    free(concatmsg);
    concatmsg = NULL;
    return;
  }

  memcpy(concatmsg, sysexmsg + 2, sizeofsysex - 3);
  concatmsg[sizeofsysex - 3] = '\0';

  auto error = deserializeJson(doc, (const char *)concatmsg);
  if (error)
  {
    return;
  }
  const char *sensor = doc["type"];
  if (sensor == NULL)
  {
    printer.myPrintf("\n doc fail \n");
    return;
  }
  const char *action = doc["action"];
  if (action == NULL)
  {
    printer.myPrintf("\n doc fail \n");
    return;
  }

  if (strcmp(sensor, "knob") == 0)
  {
    if (strcmp(action, "release") == 0)
    {
#if MD_MIDI_DEBUG
      printer.myPrintf("release \r\n");
#endif
      update_knob_event(RELEASE);
    }
    else if (strcmp(action, "pressed") == 0)
    {
#if MD_MIDI_DEBUG
      printer.myPrintf("pressed \n\r");
#endif
      update_knob_event(PRESSED);
    }
    else if (strcmp(action, "clockwise") == 0)
    {
#if MD_MIDI_DEBUG
      printer.myPrintf("clockwise \n\r");
#endif
      update_knob_event(CLOCKWISE);
    }
    else if (strcmp(action, "c_clockwise") == 0)
    {
#if MD_MIDI_DEBUG
      printer.myPrintf("c_clockwise \n\r");
#endif
      update_knob_event(C_CLOCKWISE);
    }
  }
  free(concatmsg);
  concatmsg = NULL;
  return;
}
#endif

void MuroBoxMIDI::MuroBoxMIDISetup()
{
  /* Register the MIDI insert/remove event */
  USB_Midi.MBOX_reg_midi_insert_hook(pack_int_midi_insert);
  USB_Midi.MBOX_reg_midi_remove_hook(pack_int_midi_removed);
#if !MBOX_NO_SERIAL_MIDI
  /* All the supported handler */
#ifdef USBH_MIDI_SYSEX_ENABLE
  MIDI.setHandleSystemExclusive(handle_sysex);
#endif
#if !MBOX_NO_MIDI_OUT
  /* Enable MIDI Output from Muro Box*/
  MIDI.begin(rxChannel); // Launch MIDI and listen to channel rxChannel
#endif                   //!MBOX_NO_MIDI_OUT
#if !MBOX_NO_SERIAL_MIDI
  MIDI.turnThruOff(); // Need to be after MIDI.begin
#endif
#endif //!MBOX_NO_SERIAL_MIDI
}

// Poll USB MIDI Controler and send to serial MIDI
static void USB_MIDI_poll()
{
  uint8_t size;
#if USBH_MIDI_SYSEX_ENABLE
  uint8_t recvBuf[MIDI_EVENT_PACKET_SIZE];
  uint8_t rcode = 0; //return code
  uint8_t readPtr = 0;

  rcode = USB_Midi.RecvData(recvBuf, true);

  //data check
  if (rcode == 0)
    return;
  if (recvBuf[0] == 0 && recvBuf[1] == 0 && recvBuf[2] == 0 && recvBuf[3] == 0)
  {
    return;
  }

  uint8_t *p = recvBuf;
  while (readPtr < MIDI_EVENT_PACKET_SIZE)
  {
    if (*p == 0 && *(p + 1) == 0)
      break; //data end

    uint8_t outbuf[3];
    uint8_t rc = USB_Midi.extractSysExData(p, outbuf);
    if (rc == 0)
    {
      /* Normal event */
      p++;
      size = USB_Midi.lookupMsgSize(*p);
      _MIDI_SERIAL_PORT.write(p, size);
      p += 3;
    }
    else
    {
#if !MBOX_NO_SERIAL_MIDI
      /* System Exclusive event with size rc bytes */
      _MIDI_SERIAL_PORT.write(outbuf, rc);
#endif //!MBOX_NO_SERIAL_MIDI
      /* Add handler and not forwarding? */
      p += 4;
    }
    readPtr += 4;
  }
#else
  uint8_t outBuf[3];
  do
  {
    if ((size = USB_Midi.RecvData(outBuf)) > 0)
    {
#if !MBOX_NO_SERIAL_MIDI
      //MIDI Output
      _MIDI_SERIAL_PORT.write(outBuf, size);
#endif //!MBOX_NO_SERIAL_MIDI
    }
  } while (size > 0);
#endif
}

/* Called every tick cycle */
void MuroBoxMIDI::MuroBoxMIDILoop()
{
  // If we have received a message from USB (External)
  USB_MIDI_poll();
#if !MBOX_NO_SERIAL_MIDI
#if !MBOX_NO_MIDI_OUT
  // If we have received a message from UART (Muro Box)
  // Need repeatedly called every tick
  if (MIDI.read())
  {
    uint8_t msg[4];
    msg[0] = MIDI.getType();
    switch (msg[0])
    {
    case midi::ActiveSensing:
      break;
    case midi::SystemExclusive:
      //SysEx from UART is handled by event.
      break;
    default:
      /* Default farward to USB MIDI */
      msg[1] = MIDI.getData1();
      msg[2] = MIDI.getData2();
      USB_Midi.SendData(msg, 0);
      break;
    }
  }
#endif //!MBOX_NO_MIDI_OUT
#endif //!MBOX_NO_SERIAL_MIDI
}

//MIDI.sendNoteOn(42, 127, txChannel);    // Send a Note (pitch 42, velo 127 on channel txChannel)
//MIDI.sendNoteOff(42, 0, txChannel);     // Stop the note
