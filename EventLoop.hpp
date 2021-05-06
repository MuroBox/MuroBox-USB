/* Copyright (C) 2021 Tevofy Technology LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").
 */
#pragma once

#include "Event.hpp"

    namespace EventLoop {

typedef void (*Function)(Event &event);

void initialize();
void addDelayedEvent(Function call, uint32_t delay, void *data = NULL);
void addEvent(Function call, void *data = NULL);
void loop();
  
}
