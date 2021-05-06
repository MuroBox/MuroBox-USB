/* Copyright (C) 2021 Tevofy Technology LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").
 */
#include "MuroBoxUSBDesc.hpp"
#include "MuroBoxUtility.hpp"
#include "pgmstrings.h"
#include <usbhub.h>

    namespace MuroBoxUSBDesc {
  
void PrintAllDescriptors(UsbDevice *pdev);
void PrintAllAddresses(UsbDevice *pdev);
uint8_t getconfdescr( uint8_t addr, uint8_t conf );
void printconfdescr( uint8_t* descr_ptr );
void printintfdescr( uint8_t* descr_ptr );
void printepdescr( uint8_t* descr_ptr );
void printunkdescr( uint8_t* descr_ptr );
uint8_t getdevdescr( uint8_t addr, uint8_t &num_conf );

USB *Usb_ref;

void USBPrintAllDescAdd(USB &Usb) {
  Usb_ref = &Usb;
  Usb_ref->ForEachUsbDevice(&PrintAllDescriptors);
  Usb_ref->ForEachUsbDevice(&PrintAllAddresses);
}

void PrintAllAddresses(UsbDevice *pdev)
{
  UsbDeviceAddress adr;
  adr.devAddress = pdev->address.devAddress;
  printer.myPrintf("\r\nAddr:");
  printer.myPrintHEX(adr.devAddress);
  printer.myPrintf("(");
  printer.myPrintHEX(adr.bmHub);
  printer.myPrintf(".");
  printer.myPrintHEX(adr.bmParent);
  printer.myPrintf(".");
  printer.myPrintHEX(adr.bmAddress);
  printer.myPrintf(")");
}

void PrintAddress(uint8_t addr)
{
  UsbDeviceAddress adr;
  adr.devAddress = addr;
  printer.myPrintf("\r\nADDR:\t");
  printer.myPrintHEX(adr.devAddress);
  printer.myPrintf("DEV:\t");
  printer.myPrintHEX(adr.bmAddress);
  printer.myPrintf("PRNT:\t");
  printer.myPrintHEX(adr.bmParent);
  printer.myPrintHEX(adr.bmHub);
}


void PrintDescriptors(uint8_t addr)
{
  uint8_t rcode = 0;
  uint8_t num_conf = 0;

  rcode = getdevdescr( (uint8_t)addr, num_conf );
  if ( rcode )
  {
     printer.myPrintf(Gen_Error_str);
    printer.myPrintHEX( rcode, 8 );
  }
  printer.myPrintf("\r\n");

  for (int i = 0; i < num_conf; i++)
  {
    rcode = getconfdescr( addr, i );                 // get configuration descriptor
    if ( rcode )
    {
       printer.myPrintf(Gen_Error_str);
      printer.myPrintHEX(rcode, 8);
    }
    printer.myPrintf("\r\n");
  }
}

void PrintAllDescriptors(UsbDevice *pdev)
{
  printer.myPrintf("\r\n");
  printer.myPrintHEX(pdev->address.devAddress, 8);
  printer.myPrintf("\r\n--");
  PrintDescriptors( pdev->address.devAddress );
}


uint8_t getdevdescr( uint8_t addr, uint8_t &num_conf )
{
  USB_DEVICE_DESCRIPTOR buf;
  uint8_t rcode;
  rcode = Usb_ref->getDevDescr( addr, 0, 0x12, ( uint8_t *)&buf );
  if ( rcode ) {
    return ( rcode );
  }
   printer.myPrintf(Dev_Header_str);
   printer.myPrintf(Dev_Length_str);
  printer.myPrintHEX( buf.bLength, 8 );
   printer.myPrintf(Dev_Type_str);
  printer.myPrintHEX( buf.bDescriptorType, 8 );
   printer.myPrintf(Dev_Version_str);
  printer.myPrintHEX( buf.bcdUSB, 16 );
   printer.myPrintf(Dev_Class_str);
  printer.myPrintHEX( buf.bDeviceClass, 8 );
   printer.myPrintf(Dev_Subclass_str);
  printer.myPrintHEX( buf.bDeviceSubClass, 8 );
   printer.myPrintf(Dev_Protocol_str);
  printer.myPrintHEX( buf.bDeviceProtocol, 8 );
   printer.myPrintf(Dev_Pktsize_str);
  printer.myPrintHEX( buf.bMaxPacketSize0, 8 );
   printer.myPrintf(Dev_Vendor_str);
  printer.myPrintHEX( buf.idVendor, 16 );
   printer.myPrintf(Dev_Product_str);
  printer.myPrintHEX( buf.idProduct, 16 );
   printer.myPrintf(Dev_Revision_str);
  printer.myPrintHEX( buf.bcdDevice, 16 );
   printer.myPrintf(Dev_Mfg_str);
  printer.myPrintHEX( buf.iManufacturer, 8 );
   printer.myPrintf(Dev_Prod_str);
  printer.myPrintHEX( buf.iProduct, 8 );
   printer.myPrintf(Dev_Serial_str);
  printer.myPrintHEX( buf.iSerialNumber, 8 );
   printer.myPrintf(Dev_Nconf_str);
  printer.myPrintHEX( buf.bNumConfigurations, 8 );
  num_conf = buf.bNumConfigurations;
  return ( 0 );
}

void printhubdescr(uint8_t *descrptr, uint8_t addr)
{
  HubDescriptor  *pHub = (HubDescriptor*) descrptr;
  uint8_t        len = *((uint8_t*)descrptr);

   printer.myPrintf(("\r\n\r\nHub Descriptor:\r\n"));
   printer.myPrintf(("bDescLength:\t\t"));
   printer.myPrintHEX(pHub->bDescLength);
   printer.myPrintf("\r\n");

   printer.myPrintf(("bDescriptorType:\t"));
   printer.myPrintHEX(pHub->bDescriptorType);
   printer.myPrintf("\r\n");

   printer.myPrintf(("bNbrPorts:\t\t"));
   printer.myPrintHEX(pHub->bNbrPorts);

   printer.myPrintf(("LogPwrSwitchMode:\t"));
   printer.myPrintBIN(pHub->LogPwrSwitchMode);

   printer.myPrintf(("CompoundDevice:\t\t"));
   printer.myPrintBIN(pHub->CompoundDevice);

   printer.myPrintf(("OverCurrentProtectMode:\t"));
   printer.myPrintBIN(pHub->OverCurrentProtectMode);

   printer.myPrintf(("TTThinkTime:\t\t"));
   printer.myPrintBIN(pHub->TTThinkTime);

   printer.myPrintf(("PortIndicatorsSupported:"));
   printer.myPrintBIN(pHub->PortIndicatorsSupported);

   printer.myPrintf(("Reserved:\t\t"));
   printer.myPrintHEX(pHub->Reserved);

   printer.myPrintf(("bPwrOn2PwrGood:\t\t"));
   printer.myPrintHEX(pHub->bPwrOn2PwrGood);
   printer.myPrintf("\r\n");

   printer.myPrintf(("bHubContrCurrent:\t"));
   printer.myPrintHEX(pHub->bHubContrCurrent);
   printer.myPrintf("\r\n");

  for (uint8_t i = 7; i < len; i++)
    printer.myPrintHEX(descrptr[i], 8);

  //for (uint8_t i=1; i<=pHub->bNbrPorts; i++)
  //    PrintHubPortStatus(Usb_ref, addr, i, 1);
}

uint8_t getconfdescr( uint8_t addr, uint8_t conf )
{
  uint8_t buf[ BUFSIZE ];
  uint8_t* buf_ptr = buf;
  uint8_t rcode;
  uint8_t descr_length;
  uint8_t descr_type;
  uint16_t total_length;
  rcode = Usb_ref->getConfDescr( addr, 0, 4, conf, buf );  //get total length
  LOBYTE( total_length ) = buf[ 2 ];
  HIBYTE( total_length ) = buf[ 3 ];
  if ( total_length > 256 ) {   //check if total length is larger than buffer
     printer.myPrintf(Conf_Trunc_str);
    total_length = 256;
  }
  rcode = Usb_ref->getConfDescr( addr, 0, total_length, conf, buf ); //get the whole descriptor
  while ( buf_ptr < buf + total_length ) { //parsing descriptors
    descr_length = *( buf_ptr );
    descr_type = *( buf_ptr + 1 );
    switch ( descr_type ) {
      case ( USB_DESCRIPTOR_CONFIGURATION ):
        printconfdescr( buf_ptr );
        break;
      case ( USB_DESCRIPTOR_INTERFACE ):
        printintfdescr( buf_ptr );
        break;
      case ( USB_DESCRIPTOR_ENDPOINT ):
        printepdescr( buf_ptr );
        break;
      case 0x29:
        printhubdescr( buf_ptr, addr );
        break;
      default:
        printunkdescr( buf_ptr );
        break;
    }//switch( descr_type
    buf_ptr = ( buf_ptr + descr_length );    //advance buffer pointer
  }//while( buf_ptr <=...
  return ( rcode );
}

/* function to print configuration descriptor */
void printconfdescr( uint8_t* descr_ptr )
{
  USB_CONFIGURATION_DESCRIPTOR* conf_ptr = ( USB_CONFIGURATION_DESCRIPTOR* )descr_ptr;
   printer.myPrintf(Conf_Header_str);
   printer.myPrintf(Conf_Totlen_str);
  printer.myPrintHEX( conf_ptr->wTotalLength, 16 );
   printer.myPrintf(Conf_Nint_str);
  printer.myPrintHEX( conf_ptr->bNumInterfaces, 8 );
   printer.myPrintf(Conf_Value_str);
  printer.myPrintHEX( conf_ptr->bConfigurationValue, 8 );
   printer.myPrintf(Conf_String_str);
  printer.myPrintHEX( conf_ptr->iConfiguration, 8 );
   printer.myPrintf(Conf_Attr_str);
  printer.myPrintHEX( conf_ptr->bmAttributes, 8 );
   printer.myPrintf(Conf_Pwr_str);
  printer.myPrintHEX( conf_ptr->bMaxPower, 8 );
  return;
}
/* function to print interface descriptor */
void printintfdescr( uint8_t* descr_ptr )
{
  USB_INTERFACE_DESCRIPTOR* intf_ptr = ( USB_INTERFACE_DESCRIPTOR* )descr_ptr;
   printer.myPrintf(Int_Header_str);
   printer.myPrintf(Int_Number_str);
  printer.myPrintHEX( intf_ptr->bInterfaceNumber, 8 );
   printer.myPrintf(Int_Alt_str);
  printer.myPrintHEX( intf_ptr->bAlternateSetting, 8 );
   printer.myPrintf(Int_Endpoints_str);
  printer.myPrintHEX( intf_ptr->bNumEndpoints, 8 );
   printer.myPrintf(Int_Class_str);
  printer.myPrintHEX( intf_ptr->bInterfaceClass, 8 );
   printer.myPrintf(Int_Subclass_str);
  printer.myPrintHEX( intf_ptr->bInterfaceSubClass, 8 );
   printer.myPrintf(Int_Protocol_str);
  printer.myPrintHEX( intf_ptr->bInterfaceProtocol, 8 );
   printer.myPrintf(Int_String_str);
  printer.myPrintHEX( intf_ptr->iInterface, 8 );
  return;
}
/* function to print endpoint descriptor */
void printepdescr( uint8_t* descr_ptr )
{
  USB_ENDPOINT_DESCRIPTOR* ep_ptr = ( USB_ENDPOINT_DESCRIPTOR* )descr_ptr;
   printer.myPrintf(End_Header_str);
   printer.myPrintf(End_Address_str);
  printer.myPrintHEX( ep_ptr->bEndpointAddress, 8 );
   printer.myPrintf(End_Attr_str);
  printer.myPrintHEX( ep_ptr->bmAttributes, 8 );
   printer.myPrintf(End_Pktsize_str);
  printer.myPrintHEX( ep_ptr->wMaxPacketSize, 16 );
   printer.myPrintf(End_Interval_str);
  printer.myPrintHEX( ep_ptr->bInterval, 8 );

  return;
}
/*function to print unknown descriptor */
void printunkdescr( uint8_t* descr_ptr )
{
  uint8_t length = *descr_ptr;
  uint8_t i;
   printer.myPrintf(Unk_Header_str);
   printer.myPrintf(Unk_Length_str);
  printer.myPrintHEX( *descr_ptr, 8 );
   printer.myPrintf(Unk_Type_str);
  printer.myPrintHEX( *(descr_ptr + 1 ), 8 );
   printer.myPrintf(Unk_Contents_str);
  descr_ptr += 2;
  for ( i = 0; i < length; i++ ) {
    printer.myPrintHEX( *descr_ptr, 8 );
    descr_ptr++;
  }
}

} //namespace MuroBoxUSBDesc
