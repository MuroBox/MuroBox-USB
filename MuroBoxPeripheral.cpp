/*******************************************************************************
*
* Copyright (C) 2006 Maxim Integrated Products, Inc. All Rights Reserved.
* Copyright (C) 2021 Tevofy Technology LTD. All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,  
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL MAXIM
* INTEGRATED PRODUCTS INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
* IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*******************************************************************************/

//Reference: USB Enumeration Code (and More) for the MAX3420E
#include "MuroBoxPeripheral.hpp"
#include "MuroBoxUtility.hpp"
#include "Muro_enum_tables.h"
#include "MuroMAX3421E.h"
#include <Usb.h>

typedef unsigned char BYTE; // these save typing
typedef unsigned short WORD;

MuroBoxPeripheral M_Peripheral;
extern USB Usb;
extern Plugin_State plugin_state;

//Global variables
BYTE SUD[8];            // Local copy of the 8 setup data read from the MAX3420E SUDFIFO
BYTE configval;         // Set/Get_Configuration value
BYTE ep3stall;          // Flag for EP3 Stall, set by Set_Feature, reported back in Get_Status
//

void MuroBoxPeripheral::MuroBoxPeripheralSetup()
{
    ep3stall = 0;
    configval = 0;
    printer.myPrintf("MuroBoxPeripheralSetup\r\n");

    if (Usb.Init_device() != 0)
        printer.myPrintf("OSC did not start. (initUSB failed)\r\n");
    else
        printer.myPrintf("USB.Init_device Success\r\n");
    return;
}

void MuroBoxPeripheral::MuroBoxPeripheralLoop()
{
    device_task();
}


void device_task()
{
    if (plugin_state == Plugin_State::HOST_PLUGIN)
    {
        if (Usb.MAX_Int_Pending()) //If any MAX3420E interrupts are pending
        {
            service_irqs(); //Interrupts Handeler
        }

    } 
}

/*
The possible interrupts for this application are:
a. Setup Data arrived (SUDAVIRQ).
b. The host send midi event data by sending a OUT data to EP1-OUT.
c. The host initiated a bus reset.
d. The host completed bus reset signaling
*/
void service_irqs(void)
{
    // Read out the two IRQ-Register and save the result in variables
    BYTE EPIRQ_reg, USBIRQ_reg, sample;
    EPIRQ_reg = Usb.regRd(rEPIRQ); // Read the EPIRQ bits
    USBIRQ_reg = Usb.regRd(rUSBIRQ); // Read the USBIRQ bits
    if (EPIRQ_reg & bmSUDAVIRQ) // Check for SETUP data
    {
        Usb.regWr(rEPIRQ, bmSUDAVIRQ); // clear the SUDAV IRQ
        do_SETUP();
    }

    if (EPIRQ_reg & bmOUT1DAVIRQ) // Check for Endpoint 1 OUT Data Available Interrupt Request
    {
	//TODO: need a usb midi event parser
        BYTE Data[4];
        BYTE nbytes = Usb.regRd(rEP1OUTBC); //read received byte count

        while (nbytes >= 4) //check if there is any data left to read. Also, midi event packet must be four byte
        {
            for (int j = 0; j < 4; j++)
            {
                sample = Usb.regRd(rEP1OUTFIFO); //read one byte from the FIFO
                Data[j] = sample; //load to local Data array
                nbytes--; //update how many bytes of data are left to read
            }
            if ((Data[1] >= 0x80) && (Data[1] <= 0xe0)) //check: is it a command byte
            {
                // send through Serial to the motherboard. The first byte of a midi event is the Packet Header, ignored here. 
                // See p.16 of the USB MIDI 1.0 specification
                Serial.write(&Data[1], 3); 
                Serial.flush();
            }
        }
        Usb.regWr(rEPIRQ, bmOUT1DAVIRQ); //this is critical.
                                         //page 57 of the MAX3420 programming manual:
                                         // The CPU clears this bit by writing a 1 to it.
                                         // This also arms the endpoint for another transfer.

    }


    // The last two checks handle USB bus reset. Note that MAX3420E code
    // should always include a test for a USB bus reset. This is because the
    // MAX3420E clears most of its interrupt enable register bits during a USB bus
    // reset. Therefore, the code should be alert to a bus reset. When the reset is
    // complete (signaled by the USBRESDN IRQ), it should re-enable the interrupts
    // that it is using for the application.

    if (USBIRQ_reg & bmURESIRQ) 
    {
        Usb.regWr(rUSBIRQ, bmURESIRQ); // clear the IRQ
    }
    if (USBIRQ_reg & bmURESDNIRQ)
    {
        Usb.regWr(rUSBIRQ, bmURESDNIRQ); // clear the IRQ bit
        //ENABLE_IRQS                 // ...because a bus reset clears the IE bits
        Usb.regWr(rEPIEN, (bmSUDAVIE + bmIN0BAVIE + bmOUT1DAVIE));
        Usb.regWr(rUSBIEN, (bmURESIE + bmURESDNIE + bmNOVBUSIE + bmVBUSIE));
    }
}

#define P_STALL_EP0 Usb.regWr(rEPSTALLS, 0x23); // Set all three EP0 stall bits--data stage IN/OUT and status stage

void do_SETUP(void)
{
    //printer.myPrintf("do_SETUP\r\n");
    Usb.bytesRd(rSUDFIFO, 8, SUD);            // got a SETUP packet. Read 8 SETUP bytes
    if ((SUD[bmRequestType] & 0x60) == 0) // handle standard requests only
	{
        switch (SUD[bRequest])
		{
		case	SR_GET_DESCRIPTOR:	send_descriptor();    break;
		// not used in midi device case (from enum example for hid keyboard)
		// case	SR_SET_FEATURE:		feature(1);           break;
		// case	SR_CLEAR_FEATURE:	feature(0);           break; 
		case	SR_GET_STATUS:		get_status();         break;
        // not used in midi device case (from enum example for hid keyboard)
        // case	SR_SET_INTERFACE:	set_interface();      break;
		// case	SR_GET_INTERFACE:	get_interface();      break;
		case	SR_GET_CONFIGURATION:   get_configuration();  break;
		case	SR_SET_CONFIGURATION:   set_configuration();  break;
		case	SR_SET_ADDRESS:	  set_address();  break;
		default:  P_STALL_EP0
        }
	}
else	P_STALL_EP0				// We dont' recognize the request
}

void set_address(void)
{
    //printer.myPrintf("set_address\r\n");
    uint8_t sample;
    sample = Usb.regRd((rFNADDR | 0x01)); // dummy read to set the ACKSTAT bit
}

//*******************
void set_configuration(void)
{
    //printer.myPrintf("set_configuration\r\n");
    configval = SUD[wValueL]; // Store the config value
    Usb.regRd(rFNADDR | 0x01);       //PrregAS(rFNADDR);// dummy read to set the ACKSTAT bit
    //Usb.regWr((rEPSTALLS), bmACKSTAT);
}

void get_configuration(void)
{
    //printer.myPrintf("get_configuration\r\n");
    Usb.regWr(rEP0FIFO, configval); // Send the config value
    //PwregAS(rEP0BC, 1);
    Usb.regWr((rEP0BC + 0x01), 1);
}

//**********************
void set_interface(void)	// All we accept are Interface=0 and AlternateSetting=0, otherwise send STALL
{
    //printer.myPrintf("set_interface\r\n");
    if ((SUD[wValueL] == 0)          // wValueL=Alternate Setting index
        && (SUD[wIndexL] == 0))      // wIndexL=Interface index
        {
            Usb.regRd((rFNADDR + 0x01)); //PrregAS(rFNADDR);		// dummy read to set the ACKSTAT bit
            //Usb.regWr((rEPSTALLS), bmACKSTAT);
        }    
    else
        P_STALL_EP0
}

//**********************
void get_interface(void)	// Check for Interface=0, always report AlternateSetting=0
{
    //printer.myPrintf("get_interface\r\n");
    if (SUD[wIndexL] == 0) // wIndexL=Interface index
    {
        Usb.regWr(rEP0FIFO, 0);        // AS=0
        Usb.regWr((rEP0BC + 0x01), 1); //PwregAS(rEP0BC,1);		// send one byte, ACKSTAT
  }
else P_STALL_EP0
}

//*******************
void get_status(void)
{
    //printer.myPrintf("get_status\r\n");
    BYTE testbyte;
    testbyte = SUD[bmRequestType];
    switch (testbyte)	
	{
	case 0x80: 			// directed to DEVICE
        Usb.regWr(rEP0FIFO, (1));   // first byte is 000000rs where r=enabled for RWU (not) and s=self-powered.
        Usb.regWr(rEP0FIFO, 0x00);  // second byte is always 0
        Usb.regWr((rEP0BC + 0x01), 2); //PwregAS(rEP0BC,2); 		// load byte count, arm the IN transfer, ACK the status stage of the CTL transfer
        break; 				
	case 0x81: 			// directed to INTERFACE
        Usb.regWr(rEP0FIFO, 0x00); // this one is easy--two zero bytes
        Usb.regWr(rEP0FIFO, 0x00);
        Usb.regWr((rEP0BC + 0x01), 2); //PwregAS(rEP0BC,2); 		// load byte count, arm the IN transfer, ACK the status stage of the CTL transfer
        break; 				
	case 0x82: 			// directed to ENDPOINT
		if(SUD[wIndexL]==0x83)		// We only reported ep3, so it's the only one the host can stall IN3=83
                  {
                      Usb.regWr(rEP0FIFO, ep3stall); // first byte is 0000000h where h is the halt (stall) bit
                      Usb.regWr(rEP0FIFO, 0x00);     // second byte is always 0
                      Usb.regWr((rEP0BC + 0x01), 2); //PwregAS(rEP0BC, 2);            // load byte count, arm the IN transfer, ACK the status stage of the CTL transfer
                      break;
                  }
		else  P_STALL_EP0		// Host tried to stall an invalid endpoint (not 3)				
	default:      P_STALL_EP0		// don't recognize the request
	}
}

// **********************************************************************************************
// FUNCTION: Set/Get_Feature. Call as feature(1) for Set_Feature or feature(0) for Clear_Feature.
// There are two set/clear feature requests:
//	To a DEVICE: 	Remote Wakeup (RWU). 
//  	To an ENDPOINT:	Stall (EP3 only for this app)
//
void feature(BYTE sc)
{
    printer.myPrintf("feature\r\n");
    BYTE mask;
    if ((SUD[bmRequestType] == 0x02) // dir=h->p, recipient = ENDPOINT
        && (SUD[wValueL] == 0x00)    // wValueL is feature selector, 00 is EP Halt
        && (SUD[wIndexL] == 0x83))   // wIndexL is endpoint number IN3=83
    {
        mask = Usb.regRd(rEPSTALLS); // read existing bits
        if (sc == 1)                 // set_feature
        {
            mask += bmSTLEP3IN; // Halt EP3IN
            ep3stall = 1;
        }
      else                        // clear_feature
        {
        mask &= ~bmSTLEP3IN;      // UnHalt EP3IN
        ep3stall=0;
        Usb.regWr(rCLRTOGS, bmCTGEP3IN); // clear the EP3 data toggle
        }
        Usb.regWr(rEPSTALLS, (mask | bmACKSTAT)); // Don't use PwregAS for this--directly writing the ACKSTAT bit
      }
  else P_STALL_EP0
}

//************************
void send_descriptor(void)
{
    //printer.myPrintf("send_descriptor\r\n");
    WORD reqlen, sendlen, desclen;
    unsigned char *pDdata; // pointer to ROM Descriptor data to send
    //
    // NOTE This function assumes all descriptors are 64 or fewer bytes and can be sent in a single packet
    //
    desclen = 0;                                  // check for zero as error condition (no case statements satisfied)
    reqlen = SUD[wLengthL] + 256 * SUD[wLengthH]; // 16-bit
    switch (SUD[wValueH])			// wValueH is descriptor type
	{
	case  GD_DEVICE:
              desclen = DD[0];	// descriptor length
              pDdata = DD;
              break;	
	case  GD_CONFIGURATION:
              desclen = CD[2];	// Config descriptor includes interface, HID, report and ep descriptors
              pDdata = CD;
              pack_int_midi_insert(0); //Almost complete enumeration, signal motherboard to UART mode 
              break;
	case  GD_STRING:
              desclen = strDesc[SUD[wValueL]][0];   // wValueL=string index, array[0] is the length
              pDdata = strDesc[SUD[wValueL]];       // point to first array element
              break;
    default: 
            P_STALL_EP0 // none of the descriptor types match
		    break;
	}	// end switch on descriptor type

    sendlen = (reqlen <= desclen) ? reqlen : desclen; // send the smaller of requested and avaiable
    reqlen = sendlen;
    BYTE mask;
    do
    {
        sendlen = (reqlen <= 64) ? reqlen : 64; //Valid values are 0-64 bytes. If greater than 64 bytes, split data into successive data packets in the same transfer.
        do
        {
            mask = Usb.regRd(rEPIRQ);
        } while (!(mask & bmIN0BAVIRQ)); //check if EP0FIFO is again available for loading.
        Usb.bytesWr(rEP0FIFO, sendlen, pDdata); //write a series of bytes to FIFO to fill it with IN data
        if (reqlen < 64)
        {
            Usb.regWr((rEP0BC | 0x01), sendlen); // load EP0BC to arm the EP0-IN transfer & ACKSTAT
        }
        else
        {
            Usb.regWr(rEP0BC, sendlen); // load EP0BC to arm the EP0-IN transfer but without ACKSTAT
        }

        reqlen -= sendlen;
        pDdata += sendlen;
    } while (reqlen > 0); //check if any bytes are left to send

}

/* Logic: Vbus is wired to a Vbus comparator in MAX3421. Vbus makes a 0-1 transition either a host or 
peripheral device plugin. GPIO6 of MAX3421 is connected to an output pin(ID) of TUSB320. TUSB320 uses 
the CC pins to determine port attach and detach, cable orientation, role detection, and port control 
for Type-C current mode. A Vbus transition with GPIO6 in high voltage level indicates a peripheral 
device is plugged in. On the other hand, a Vbus transition without GPIO6 in high voltage level 
indicates a host device is plugged in. If a device detached, Vbus makes a 1-0 transition.

|------|------|-------|
|      | VBUS | GPIO6 |
|------|------|-------|
|DEVICE|  0-1 |   1   |
|------|------|-------|
| HOST |  0-1 |   0   |
|------|------|-------|
|DETACH|  1-0 |   0   |
|------|------|-------| */

void PluginDetect(void)
{
    uint8_t vbus_read;
    uint8_t gpio_read;
    vbus_read = Usb.regRd(rUSBIRQ); //Read USBIRQ register

    if (vbus_read & bmVBUSIRQ) //check VBUS comparator makes a 0-1 transition.
    {
        delay(100); //wait for gpio to make the 0-1 transition.
        gpio_read = Usb.regRd(rIOPINS2);

        if (gpio_read & bmGPIN6) //check GPIO6 in high state
        {
            printer.myPrintf("DEVICE_PLUGIN:\r\n");
            plugin_state = Plugin_State::DEVICE_PLUGIN;
            Usb.regWr(rGPINIRQ, bmGPINIRQ6); //clear irq bit
        }
        else //GPIO6 Low
        {
            printer.myPrintf("HOST_PLUGIN:\r\n");
            plugin_state = Plugin_State::HOST_PLUGIN;
        }
        Usb.regWr(rUSBIRQ, bmVBUSIRQ); //clear irq bit
    }

    if (vbus_read & bmNOVBUSIRQ) //check VBUS pin drops below the VBUS detect threshold.
    {
        printer.myPrintf("DETACHED:\r\n");
        plugin_state = Plugin_State::DETACHED;
        Usb.regWr(rUSBIRQ, bmNOVBUSIRQ); //clear irq bit
    }
}