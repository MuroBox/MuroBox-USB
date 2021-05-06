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
#pragma once
#include <Usb.h>

enum class Plugin_State { DETACHED,
              DEVICE_PLUGIN,
              HOST_PLUGIN
            };

class MuroBoxPeripheral
{
public:
    MuroBoxPeripheral() {}
    void MuroBoxPeripheralSetup();
    void MuroBoxPeripheralLoop();

private:
};

extern MuroBoxPeripheral M_Peripheral;
// USB functions
void set_address(void);
void send_descriptor(void);
void get_status(void);
void set_interface(void);
void get_interface(void);
void set_configuration(void);
void get_configuration(void);
void do_SETUP(void); // Handle a USB SETUP transfer
void service_irqs(void);
void device_task();
void PluginDetect(void);
