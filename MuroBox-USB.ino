/* Copyright (C) 2021 Tevofy Technology LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").
 */
#include "EventLoop.hpp"
#include "MuroBoxI2C.hpp"
#include "MuroBoxMIDI.hpp"
#include "MuroBoxUtility.hpp"
#include "MuroBoxUSBHost.hpp"
#include "MuroBoxMIDIPlayer.hpp"
#include "MuroBoxPeripheral.hpp"

#include <user_interface.h>

#include <MD_MIDIFile.h>
#include <usbh_midi.h>
#include <queue>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
/*
 * GPIOs 4 and 5 are the only ones that are always high impedance. All others do have internal pull-ups or are even driven low/high during boot.
 * GPIOs 3, 12, 13 and 14 pulled HIGH during boot. Their actual state does not influence the boot process.
 * GPIOs 0, 1, 2 and 15 are pulled HIGH during boot and also driven LOW for short periods.
 * The device will not boot if 0, 1 or 2 is driven LOW during start-up.
 * GPIO 16 is driven HIGH during boot, don't short to GND.
 *
 * 
 */
#include "mbox_defs.h"
#if MUROUSBFAT
#include "MuroBoxUsbFAT.hpp"
//Replace RTClib with exTime for NTP integration and ESP8266 compatibility
#include <ezTime.h>
#define LOCALTZ_POSIX "CET-1CEST,M3.4.0/2,M10.4.0/3" // Time in temp_timezone
    Timezone temp_timezone;
#else
#endif

#ifndef APSSID
#define APSSID "MuroBox-Addon-Webupdate"
#define APPSK ""
#endif

/* Set these to your desired credentials. */
const char *host = "MuroBox-Addon-Webupdate";
const char *ssid = APSSID;
const char *password = APPSK;

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

USB Usb;
/* why*/
// USBHub Hub1(&Usb);

USBH_MIDI USB_Midi(&Usb);
Plugin_State plugin_state = Plugin_State::DETACHED;
Plugin_State last_plugin_state = Plugin_State::DETACHED;

void testOnEvent(Event &event)
{
  EventLoop::addDelayedEvent(&testOffEvent, 800);
  printer.myPrintf("Test Event\r\n");
}

void testOffEvent(Event &event)
{
  EventLoop::addDelayedEvent(&testOnEvent, 600);
}

void setup()
{
#if MUROUSBFAT
  temp_timezone.setPosix(LOCALTZ_POSIX);
  temp_timezone.setTime(compileTime());
#else
#endif
  // Enabled for debug. It is disabled by default
  printer.setOut(true);
  
#if USE_WIFI
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid, password);
  WiFi.softAP(ssid); //No password

  IPAddress myIP = WiFi.softAPIP();
  printer.myPrintf("AP IP address: ");

  server.begin();
  printer.myPrintf("HTTP server started");

  MDNS.begin(host);

  httpUpdater.setup(&server);
  server.begin();

  MDNS.addService("http", "tcp", 80);
  printer.myPrintf("HTTPUpdateServer ready! Open http://MuroBox-Addon-Webupdate.local/update in your browser\n\r");
#else
  // Lower Power without the WiFi library (Forced Modem Sleep)
  wifi_set_opmode_current(NULL_MODE);     // set Wi-Fi working mode to unconfigured, don't save to flash
  wifi_fpm_set_sleep_type(MODEM_SLEEP_T); // set the sleep type to modem sleep
  wifi_fpm_open();                        // enable Forced Modem Sleep
  wifi_fpm_do_sleep(0xFFFFFFF);           // force the modem to enter sleep mode
  //yield(); //give OS control to wake up wifi. delay(1) works also
  delay(10); // without a minimum of delay(1) here it doesn't reliably enter sleep
#endif

  EventLoop::initialize();

  //M_I2C.MuroBoxI2CSetup();
  /* Must setup before all USB Host drivers
     BUG: the cs pin P15 is rising during setup here?
     It seems that spi still use p15 as cs at least during the initialization phase */
#if MUROUSBFAT
  M_UsbFAT.MuroBoxUsbFATSetup(); //need to Init_Generic_Storage() before USB starts.
#endif
  /* Seperate the SPI init code so we can control host/Peripheral setting outside
     MuroBoxUSBHostSetup. Only need once. This will cause PACK_INT 0-1 trigger motherboard into UART mode except the first time.  */
  Usb.spi_init();
  MuroBoxUSBHost::MuroBoxUSBHostSetup(&Usb);
  // initialize INT (INStalled)GPIO as an output low to indicated that MIDI are pluged in to the motherboard
  pack_int_init();
  M_MIDI.MuroBoxMIDISetup();
#if MIDI_PLAYER
  M_MIDIPlayer.MuroBoxMIDIPlayerSetup();
#endif
  // Not test now
  //EventLoop::addDelayedEvent(&testOnEvent, 600);
}

// the loop function runs over and over again forever
void loop()
{
#if USE_WIFI
  server.handleClient();
  MDNS.update();
#endif
///////////////////////////////////////////////////////////////////////////////
// Max Peripheral mode disenable
#if HOST_ONLY
  MuroBoxUSBHost::MuroBoxUSBHostLoop();
  M_MIDI.MuroBoxMIDILoop();
#if MUROUSBFAT
  M_UsbFAT.MuroBoxUsbFATLoop();
#endif
#if MIDI_PLAYER
  M_MIDIPlayer.MuroBoxMIDIPlayerLoop();
#endif
#else 
///////////////////////////////////////////////////////////////////////////////
// Max Host and Peripheral enable
  PluginDetect(); // detect device connection activities to port

  if (plugin_state != last_plugin_state)  //Change of Plugin State
  {
    last_plugin_state = plugin_state; //Update
    if (plugin_state == Plugin_State::HOST_PLUGIN) // max change to peripheral mode
    {
      M_Peripheral.MuroBoxPeripheralSetup();
    }
    if (plugin_state == Plugin_State::DEVICE_PLUGIN) // max change to host mode
    {
      MuroBoxUSBHost::MuroBoxUSBHostSetup(&Usb);
    }
    if (plugin_state == Plugin_State::DETACHED) // divice unpluged, set mex to peripheral mode as default mode
    {
      pack_int_midi_removed(0); // pull pack_int pin to low, exit UART mode on motherboard
      MuroBoxUSBHost::MuroBoxUSBHostSetup(&Usb); 
    }
  }
  else //Plugin Running 
  {
    if (plugin_state == Plugin_State::HOST_PLUGIN) //connected to PC
    {
      M_Peripheral.MuroBoxPeripheralLoop();
    }
    if (plugin_state == Plugin_State::DEVICE_PLUGIN) //connected to MIDI key board or USB stick
    {
      MuroBoxUSBHost::MuroBoxUSBHostLoop();
      M_MIDI.MuroBoxMIDILoop();
#if MUROUSBFAT
      M_UsbFAT.MuroBoxUsbFATLoop();
#endif
#if MIDI_PLAYER
      M_MIDIPlayer.MuroBoxMIDIPlayerLoop();
#endif
    }
    
  }

#endif
#ifdef ESP8266
  yield(); // needed in order to reset the watchdog timer on the ESP8266
#endif
}
