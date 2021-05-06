/* Copyright (C) 2021 Tevofy Technology LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").
 */
#pragma once

#include <Arduino.h>

    class Event
{
public:
  typedef void (*Function)(Event &event);
public:
  Event();
  Event(Function call, uint32_t next, void* data);
  Event(const Event&) = default;
  Event& operator=(const Event&) = default;
public:
  bool isValid() const;
  bool isReady(uint32_t currentTime) const;
  Function getCall() const;
  uint32_t getTime() const;
  void clear();
private:
  Function _call;
  void* _data;
  uint32_t _next; 
};
