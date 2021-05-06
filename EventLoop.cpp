/* Copyright (C) 2021 Tevofy Technology LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").
 */
#include "EventLoop.hpp"
#include "Event.hpp"
#include <map>
#include <iterator>
#include <algorithm>
#include "MuroBoxUtility.hpp"

    namespace EventLoop
{

  static std::multimap<uint32_t, Event> gEvents;

  static const uint8_t cEventMapSize = 8;
  static uint8_t cEventMapUpdated = false;

  static uint32_t gCurrentTime = 0; // In million seconds

  void initialize()
  {
    gCurrentTime = millis(); //  the number of milliseconds passed since the Arduino board began running the current program
  }

  void eventAdd(const Event &newEvent)
  {
    if (gEvents.size() >= cEventMapSize)
    {
      printer.myPrintf("Event FULL, Droping Event\n");
      return;
    }
    if (false == newEvent.isValid())
    {
      printer.myPrintf("Event invalud, Droping Event\n");
      return;
    }
    cEventMapUpdated = true;
    gEvents.insert(std::pair<uint32_t, Event>(newEvent.getTime(), newEvent));
  }

  void addDelayedEvent(Function call, uint32_t delay, void *data)
  {
    eventAdd(Event(call, gCurrentTime + delay, data));
  }

  void addEvent(Function call, void *data)
  {
    eventAdd(Event(call, gCurrentTime, data));
  }

  void processEvents()
  {
    /* Reset the event flag */
    cEventMapUpdated = false;
    /* Iterate all the events to find event expiring */
    for (std::multimap<uint32_t, Event>::iterator it = gEvents.begin();
         it != gEvents.end(); it++)
    {
      Event event = it->second;
      if (event.isReady(gCurrentTime))
      {
        const auto call = event.getCall();
        call(event);
        event.clear(); // Clear time stamp
        gEvents.erase(it);
      }
      else
      {
        /* We use ordered map, so all events from now are future events */
        break;
      }
    }
  }

  void loop()
  {
    const auto currentTime = millis();
    if (gCurrentTime != currentTime)
    {
      gCurrentTime = currentTime;
      /* Continue process event if there is any new events in the map */
      do
      {
        /* Non stop if there is new event　*/
        processEvents();
      } while (cEventMapUpdated);
    }
  }

} //namespace EventLoop
