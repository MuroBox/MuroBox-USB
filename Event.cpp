/* Copyright (C) 2021 Tevofy Technology LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").
 */
#include "Event.hpp"

    Event::Event()
    : _call(nullptr), _next(0)
{
}

Event::Event(Function call, uint32_t next, void *data)
    : _call(call), _next(next), _data(data)
{
}

bool Event::isValid() const
{
  return _call != nullptr;
}

bool Event::isReady(uint32_t currentTime) const
{
  //const auto delta = static_cast<int32_t>(_next - currentTime);
  //return delta <= 0;
  return (_next <= currentTime);
}

Event::Function Event::getCall() const
{
  return _call;
}

uint32_t Event::getTime() const
{
  return _next;
}

void Event::clear()
{
  _call = nullptr;
}
