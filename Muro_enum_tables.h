/*******************************************************************************
*
* Copyright (C) 2006 Maxim Integrated Products, Inc. All Rights Reserved.
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
// HIDKB_enum_tables.h
//
unsigned char DD[] =   // DEVICE Descriptor  //before STRING descriptor 2
    {0x12,             // bLength = 18d
     0x01,             // bDescriptorType = Device (1)
     0x00, 0x01,       // bcdUSB(L/H) USB spec rev (BCD)
     0x00, 0x00, 0x00, // bDeviceClass, bDeviceSubClass, bDeviceProtocol
     0x40,             // bMaxPacketSize0 EP0 is 64 bytes
     0x6A, 0x0B,       // idVendor(L/H)--Maxim is 0B6A
     0x46, 0x53,       // idProduct(L/H)--5346
     0x34, 0x12,       // bcdDevice--1234
     1, 2, 3,          // iManufacturer, iProduct, iSerialNumber
     1};               // bNumConfigurations

unsigned char CD[] = // CONFIGURATION Descriptor
    {
        0x09, 0x02, 0x65, 0x00, 0x02, 0x01, 0x00, 0x00, 0x32, // Config
        0x09, 0x04, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, // Interface 0
        0x09, 0x24, 0x01, 0x00, 0x01, 0x09, 0x00, 0x01, 0x01, // CS Interface (audio)
        0x09, 0x04, 0x01, 0x00, 0x02, 0x01, 0x03, 0x00, 0x00, // Interface 1
        0x07, 0x24, 0x01, 0x00, 0x01, 0x41, 0x00,             // CS Interface (midi)
        0x06, 0x24, 0x02, 0x01, 0x01, 0x00,                   //   IN  Jack 1 (emb)
        0x06, 0x24, 0x02, 0x02, 0x02, 0x00,                   //   IN  Jack 2 (ext)
        0x09, 0x24, 0x03, 0x01, 0x03, 0x01, 0x02, 0x01, 0x00, //   OUT Jack 3 (emb)
        
        0x09, 0x24, 0x03, 0x02, 0x04, 0x01, 0x01, 0x01, 0x00, //   OUT Jack 4 (ext)
        0x09, 0x05, 0x01, 0x02, 0x40, 0x00, 0x00, 0x00, 0x00, // Endpoint OUT
        0x05, 0x25, 0x01, 0x01, 0x01,                         //   CS EP IN  Jack
        0x09, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00, 0x00, 0x00, // Endpoint IN
        0x05, 0x25, 0x01, 0x01, 0x03                          //   CS EP OUT Jack

};

// STRING descriptors. An array of string arrays
unsigned char strDesc[][64] =
    {
        // STRING descriptor 0--Language string
        {
            0x04,      // bLength
            0x03,      // bDescriptorType = string
            0x09, 0x04 // wLANGID(L/H) = English-United Sates
        },

        // STRING descriptor 1--Manufacturer ID //before STRING descriptor 3
        {
            12,                                    // bLength
            0x03,                                  // bDescriptorType = string
            'M', 0, 'a', 0, 'x', 0, 'i', 0, 'm', 0 // text in Unicode
        },

        // STRING descriptor 2 - Product ID //before STRING descriptor 1
        {26,   // bLength
         0x03, // bDescriptorType = string
         'M', 0, 'U', 0, 'R', 0, 'O', 0, 'B', 0, 'O', 0, 'X', 0, '-', 0, 'M', 0, 'I', 0,
         'D', 0, 'I', 0},

        // STRING descriptor 3 - Serial Number ID //before config descrip
        {20,   // bLength
         0x03, // bDescriptorType = string
         'S', 0, '/', 0, 'N', 0, ' ', 0, '3', 0, '4', 0, '2', 0, '0', 0, 'E', 0},
        // STRING descriptor 4 - Configuration string
        {52,   // bLength
         0x03, // bDescriptorType = string
         'M', 0, 'A', 0, 'X', 0, '3', 0, '4', 0, '2', 0, '0', 0, 'E', 0, '/', 0,
         '3', 0, '4', 0, '2', 0, '1', 0, 'E', 0, ' ', 0, 'T', 0, 'e', 0, 's', 0, 't', 0,
         ' ', 0, 'B', 0, 'o', 0, 'a', 0, 'r', 0, 'd', 0}
    };
