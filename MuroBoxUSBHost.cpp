/* Copyright (C) 2021 Tevofy Technology LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").
 */
#include <MIDI.h>

#include <MIDI.h>
#include <usbhub.h>

#include "MuroBoxUSBHost.hpp"
#include "MuroBoxUSBDesc.hpp"
#include "MuroBoxUtility.hpp"
#include <SPI.h>
#include "EventLoop.hpp"
#include "Event.hpp"
#include "mbox_defs.h"

    namespace MuroBoxUSBHost
{

  USB *Usb;
  extern USBHub Hub1;
  /* CHEN:TODO: HUB is not not stable. need to debug more in the future  */
  // USBHub  Hub1(&Usb);
  // USBHub  Hub2(&Usb);
  // USBHub  Hub3(&Usb);
  // USBHub  Hub4(&Usb);
  // USBHub  Hub5(&Usb);
  // USBHub  Hub6(&Usb);
  // USBHub  Hub7(&Usb);

  static uint8_t laststate;

  static void usb_print_descriptor()
  {
    uint8_t buf[sizeof(USB_DEVICE_DESCRIPTOR)];
    USB_DEVICE_DESCRIPTOR *udd = reinterpret_cast<USB_DEVICE_DESCRIPTOR *>(buf);
    printer.myPrintf(("\r\nGetting device descriptor"));
    uint8_t rcode;
    rcode = Usb->getDevDescr(1, 0, sizeof(USB_DEVICE_DESCRIPTOR), (uint8_t *)&buf);

    if (rcode)
    {
      printer.myPrintf(("\r\nError reading device descriptor. Error code "));
      printer.myPrintHEX(rcode);
    }
    else
    {
      printer.myPrintf(("\r\nDescriptor Length:\t"));
      printer.myPrintHEX(udd->bLength, 8);
      printer.myPrintf(("\r\nDescriptor type:\t"));
      printer.myPrintHEX(udd->bDescriptorType, 8);
      printer.myPrintf(("\r\nUSB version:\t\t"));
      printer.myPrintHEX(udd->bcdUSB, 16);
      printer.myPrintf(("\r\nDevice class:\t\t"));
      printer.myPrintHEX(udd->bDeviceClass, 8);
      printer.myPrintf(("\r\nDevice Subclass:\t"));
      printer.myPrintHEX(udd->bDeviceSubClass, 8);
      printer.myPrintf(("\r\nDevice Protocol:\t"));
      printer.myPrintHEX(udd->bDeviceProtocol, 8);
      printer.myPrintf(("\r\nMax.packet size:\t"));
      printer.myPrintHEX(udd->bMaxPacketSize0, 8);
      printer.myPrintf(("\r\nVendor  ID:\t\t"));
      printer.myPrintHEX(udd->idVendor, 16);
      printer.myPrintf(("\r\nProduct ID:\t\t"));
      printer.myPrintHEX(udd->idProduct, 16);
      printer.myPrintf(("\r\nRevision ID:\t\t"));
      printer.myPrintHEX(udd->bcdDevice, 16);
      printer.myPrintf(("\r\nMfg.string index:\t"));
      printer.myPrintHEX(udd->iManufacturer, 8);
      printer.myPrintf(("\r\nProd.string index:\t"));
      printer.myPrintHEX(udd->iProduct, 8);
      printer.myPrintf(("\r\nSerial number index:\t"));
      printer.myPrintHEX(udd->iSerialNumber, 8);
      printer.myPrintf(("\r\nNumber of conf.:\t"));
      printer.myPrintHEX(udd->bNumConfigurations, 8);
    }
  }

  static void usb_event_test(Event &event)
  {
    printer.myPrintf("USB Host Event Test Sequence\r\n");
    /* Test Register read */
    uint8_t tmpbyte = Usb->regRd(rREVISION);
    switch (tmpbyte)
    {
    case (0x01): //rev.01
      printer.myPrintf(("01"));
      break;
    case (0x12): //rev.02
      printer.myPrintf(("02"));
      break;
    case (0x13): //rev.03
      printer.myPrintf(("03"));
      break;
    default:
      printer.myPrintf(("invalid. Value returned: "));
      printer.myPrintHEX(tmpbyte);
      //printer.myPrintHEX(0xdeadbeef);
      break;
    } //switch( tmpbyte...
  }

  /* return 0 in case of success */
  static int32_t usb_verify_host_version()
  {
    int32_t host_version = 0;
    /* Test Register read */
    printer.myPrintf("USB Host rREVISION: ");
    uint8_t tmpbyte = Usb->regRd(rREVISION);
    host_version = tmpbyte;
    switch (tmpbyte)
    {
    case (0x01): //rev.01
      printer.myPrintf("01");
      break;
    case (0x12): //rev.02
      printer.myPrintf("02");
      break;
    case (0x13): //rev.03
      printer.myPrintf("03");
      break;
    default:
      printer.myPrintf("invalid. Value returned: ");
      printer.myPrintHEX(tmpbyte);
      //printer.myPrintHEX(0xdeadbeef);
      break;
    } //switch( tmpbyte...
    printer.myPrintf("\r\n");
    if (host_version != 0x13)
    {
      return 1;
    }

    /* Finally success */
    return 0;
  }
  static int32_t USB_HOST_TEST()
  {
    printer.myPrintf("USB Host Test Sequence\r\n");

    /*  GPIO read write if needed in the future
   *  Usb.gpioWr(sample_gpio);
      tmpbyte = Usb.gpioRd();
   */

#if 0 // PLL test.
   /* PLL test. Stops/starts MAX3421E oscillator several times */
   printer.myPrintf(("\r\nPLL test. 100 chip resets will be performed"));
   /* check current state of the oscillator */
   if(!(Usb->regRd(rUSBIRQ) & bmOSCOKIRQ)) { //wrong state - should be on
     printer.myPrintf(("\r\nCurrent oscillator state unexpected."));
   }
   /* Restart oscillator */
   printer.myPrintf(("\r\nResetting oscillator\r\n"));
   for(uint16_t i = 0; i < 100; i++) {
#ifdef ESP8266
     yield(); // needed in order to reset the watchdog timer on the ESP8266
#endif
     printer.myPrintf(("\rReset number "));
     printer.myPrintf(i);
     Usb->regWr(rUSBCTL, bmCHIPRES); //reset
     if(Usb->regRd(rUSBIRQ) & bmOSCOKIRQ) { //wrong state - should be off
       //printer.myPrintBIN(Usb.regRd(rUSBIRQ));
       printer.myPrintf(("\r\nCurrent oscillator state unexpected."));
     }
     Usb->regWr(rUSBCTL, 0x00); //release from reset
     uint16_t j = 0;
     for(j = 1; j < 65535; j++) { //tracking off to on time
       if(Usb->regRd(rUSBIRQ) & bmOSCOKIRQ) {
         printer.myPrintf((" Time to stabilize - "));
         printer.myPrintf(j);
         printer.myPrintf((" cycles\r\n"));
         break;
       }
    }//for( uint16_t j = 0; j < 65535; j++
    if(j == 0) {
      printer.myPrintf(("PLL failed to stabilize"));
    }
  }//for( uint8_t i = 0; i < 255; i++
#endif // PLL test.

    printer.myPrintf("Finish USB_HOST_TEST, Re-Initialize USB Host\r\n");

    /* initializing USB stack */
    if (Usb->Init_host() == -1)
    {
      printer.myPrintf(("\r\nOSCOKIRQ failed to assert"));
    }
    delay(200);

    //EventLoop::addDelayedEvent(&MuroBoxUSBHost::usb_event_test, 600);
  }

  void MuroBoxUSBHostSetup(USB *Usb_from_main)
  {
    Usb = Usb_from_main;
    //Usb->spi_init();//take out spi
    laststate = 0;
    /* USB Init initialize SPI */
    if (Usb->Init_host() == -1)
      printer.myPrintf("OSC did not start. (initUSB failed)\r\n");
    else
      printer.myPrintf("USB.Init_host Success\r\n");

    while (usb_verify_host_version())
    {
#ifdef ESP8266
      yield(); // needed in order to reset the watchdog timer on the ESP8266
#endif
      Usb->regWr(rUSBCTL, bmCHIPRES); //reset
      if (Usb->regRd(rUSBIRQ) & bmOSCOKIRQ)
      { //wrong state - should be off
        printer.myPrintBIN(Usb->regRd(rUSBIRQ));
        printer.myPrintf(("\r\nCurrent oscillator state unexpected."));
      }
      Usb->regWr(rUSBCTL, 0x00); //release from reset
      uint16_t j = 0;
      for (j = 1; j < 65535; j++)
      { //tracking off to on time
        if (Usb->regRd(rUSBIRQ) & bmOSCOKIRQ)
        {
          printer.myPrintf((" Time to stabilize - "));
          printer.myPrintf(j);
          printer.myPrintf((" cycles\r\n"));
          break;
        }
      } //for( uint16_t j = 0; j < 65535; j++
      if (j == 0)
      {
        printer.myPrintf(("PLL failed to stabilize\r\n"));
        return;
      }
      if (Usb->Init_host() == -1)
        printer.myPrintf("OSC did not start. (initUSB failed)\r\n");
      delay(200);
    }
    printer.myPrintf("USB Host Setup Complete\r\n");

    //USB_HOST_TEST();
  }
  //int counter_clock = 0;
  void MuroBoxUSBHostLoop()
  {
    uint8_t usbstate;
    uint8_t rcode;

    Usb->Task();
    usbstate = Usb->getUsbTaskState();
    if (usbstate != laststate)
    {
      laststate = usbstate;
      switch (usbstate)
      {
      case (USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE):
        printer.myPrintf("USB STATE:Waiting for device...\r\n");
        break;
      case (USB_ATTACHED_SUBSTATE_RESET_DEVICE):
        printer.myPrintf("USB STATE:Device connected. Resetting...\r\n");
        break;
      case (USB_ATTACHED_SUBSTATE_WAIT_SOF):
        printer.myPrintf(("USB STATE:Reset complete. Waiting for the first SOF...\r\n"));
        break;
      case (USB_ATTACHED_SUBSTATE_GET_DEVICE_DESCRIPTOR_SIZE):
        printer.myPrintf(("USB STATE:SOF generation started. Enumerating device...\r\n"));
        break;
      case (USB_STATE_ADDRESSING):
        printer.myPrintf(("USB STATE:Setting device address...\r\n"));
        break;
      case (USB_STATE_RUNNING):
      {
        printer.myPrintf(("USB STATE:Running\r\n"));
#if 0 // Print Discriptor
        usb_print_descriptor();
#endif
        break;
      }
      case (USB_STATE_ERROR):
        printer.myPrintf(("USB STATE:state machine reached error state\r\n"));
        break;
      default:
        break;
      } //switch( usbstate...
    }

#if 0
    /* For testing USB device */
    if (Usb->getUsbTaskState() == USB_STATE_RUNNING)
    {

      MuroBoxUSBDesc::USBPrintAllDescAdd(Usb);
      while (1)
      { // stop
#ifdef ESP8266
        yield(); // needed in order to reset the watchdog timer on the ESP8266
#endif
      }
    }
#endif
  }

} //namespace MuroBoxUSBHost
