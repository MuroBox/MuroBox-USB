/* Copyright (C) 2021 Tevofy Technology LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").
 */
#pragma once
#include "MuroBoxUtility.hpp"
    class MuroBoxMIDIPlayer {
public:
    MuroBoxMIDIPlayer() {}
    void MuroBoxMIDIPlayerSetup();
    void MuroBoxMIDIPlayerLoop();
private:
};

extern MuroBoxMIDIPlayer M_MIDIPlayer;

struct SongList
{
    short num_taken;
    short num_songs;
    short name_length;
    std::vector<wchar_t *> songlist_vector;
};