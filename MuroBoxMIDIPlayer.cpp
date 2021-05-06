/* Copyright (C) 2021 Tevofy Technology LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").
 */
#include "MuroBoxMIDIPlayer.hpp"
#include <masstorage.h>
#include <Storage.h>
#include <string.h>
#include <stdio.h>
#include <MD_MIDIFile.h>
#include <usbh_midi.h>
#include <queue>

    /* Create one Muro Box UsbFAT Instance */
    MuroBoxMIDIPlayer M_MIDIPlayer;
MD_MIDIFile SMF;
SongList *songlist = NULL;
extern USBH_MIDI USB_Midi;
extern USB Usb;
extern std::queue<int> knob_event_queue;
extern volatile bool fatready;
const uint16_t WAIT_DELAY = 500; // ms
volatile uint8_t scan_files_fail = 0;
volatile uint8_t current_state_player = 1;
volatile uint8_t smf_closed = 1;
volatile uint8_t cleared = 1;
volatile uint8_t midi_inserted = 0;
int heapCheck = 0;

/* To clear the event of knob being pressed or rotated by the user*/
void clear_queue(std::queue<int> &q)
{
   std::queue<int> empty;
   std::swap( q, empty );
}

/* To retrieve the event of knob being pressed or rotated by the user*/
int getPlayerEvent()
{
    if (!knob_event_queue.empty())
    {
        int event = knob_event_queue.front();
        knob_event_queue.pop();
        return event;
    }
    return 0;
}

/* free allocated SongList*/
void freeTable(SongList *songlist)
{
    if (songlist != NULL)
    {
        int indrow;
        for (indrow = 0; indrow < (songlist->num_taken); indrow++)
        {
            free((songlist->songlist_vector)[indrow]);
            (songlist->songlist_vector)[indrow] = NULL;
        }
        songlist->songlist_vector.clear();
        delete songlist;
    }
    return;
}

void printTable(SongList *songlist)
{
    if (songlist != NULL)
    {
        for (int i = 0; i < songlist->num_taken; i++)
        {
            printer.myPrintf("songname\r\n");
            wprintf(L"%ls \n", songlist->songlist_vector[i]);
        }
    }
}

//
bool CreateSonglist(SongList *songlist)
{
    songlist->num_taken = 0; //number of songs already in the list
    songlist->num_songs = 100; // total number of songs allowed to add
    return true;
}

bool match(TCHAR *a, TCHAR *b) //check if string "b" contains a patter of string "a"
{
    int c;
    int position = 0;
    TCHAR *x, *y;

    x = a;
    y = b;

    while (*a)
    {
        while (*x == *y)
        {
            x++;
            y++;
            if (*x == '\0' || *y == '\0')
                break;
        }
        if (*y == '\0')
            break;

        a++;
        position++;
        x = a;
        y = b;
    }
    if (*a)
        return true;
    else
        return false;
}

TCHAR *concat(TCHAR *s1, TCHAR *s2, TCHAR *result) //concat two strings to one
{
#if _LFN_UNICODE
    // in real code you would check for errors in malloc here
    wcscpy(result, s1); // path
    wcscat(result, L"/"); // '/' added infront of the filename
    wcscat(result, s2); //filename
#else
    strcpy(result, s1);
    strcat(result, "/"); // '/' added infront of the filename
    strcat(result, s2);
#endif
}

/* Scan through all files in the flash drive looking for all files 
with a file extension of ".mid". When a valid file is found the name 
and the path of the file is added to a list. */
FRESULT scan_files(
    TCHAR *path /* Start node to be scanned (also used as work area) */, int layer, SongList *songlist)
{
    layer++;
    yield();
    FRESULT res;
    FILINFO fno;
    DIR dir;
    int i;
    int n;
    TCHAR *fn;

    TCHAR *patt;
    TCHAR *patt2;
    patt = (TCHAR *)malloc(10);
    if (patt == NULL)
        return FR_INT_ERR;
    patt2 = (TCHAR *)malloc(10);
    if (patt2 == NULL)
        return FR_INT_ERR;

#if _LFN_UNICODE
    wcscpy(patt, L".mid"); 
    wcscpy(patt2, L".MID");
#else
    strcpy(patt, ".mid");
    strcpy(patt2, ".MID");
#endif

#if _USE_LFN
    static TCHAR lfn[_MAX_LFN + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof lfn;
#endif

    res = f_opendir(&dir, path); /* Open the directory */
    if (res == FR_OK)
    {
#if _LFN_UNICODE
        i = wcslen(path);
#else
        i = strlen(path);
#endif
        for (;;)
        {
            res = f_readdir(&dir, &fno); /* Read a directory item */

#if _USE_LFN
            fn = *fno.lfname ? fno.lfname : fno.fname;
#else
            fn = fno.fname;
#endif
            if (res != FR_OK || (fn[0] == 0))
            {
                break; /* Break on error or end of dir */
            }
#if _LFN_UNICODE
            if (fn[0] == L'.')
#else
            if (fn[0] == '.')
#endif
            {
                continue; /* Ignore dot entry */
            }
            if (fno.fattrib & AM_DIR)
            { /* It is a directory */
#if _LFN_UNICODE
                n = swprintf(&path[i], _MAX_LFN, L"/%ls", fn);
#else
                n = snprintf(&path[i], _MAX_LFN, "/%s", fn);
#endif
                if (n < 0) //error handle
                {
                    res = FR_INT_ERR;
                    printer.myPrintf("swprintf fail");
                    break;
                }

                res = scan_files(path, layer, songlist);
                if (res != FR_OK) //error handle
                {
                    printer.myPrintf("scan_files fail:");
                    printer.myPrintf(res);
                    break;
                }
                path[i] = 0;
            }
            else
            { /* It is a file. */
                // the filename matches the string ".mid" or ".MID", and the songlist has not reach the maximum number of songs allowed
                if ((match(fn, patt) || match(fn, patt2)) && (songlist->num_taken < songlist->num_songs)) 
                {
                    TCHAR *temp = (TCHAR *)malloc(sizeof(TCHAR) * (wcslen(fn) + wcslen(path) + 2)); // +2 for the null-terminator and '/'
                    if (temp)
                    {
                        concat(path, fn, temp); // concatenate the filename and the path (path/filename)
                        songlist->songlist_vector.push_back(temp); // push to the list
                        songlist->num_taken++;                     // update the number of songs already in the list
                    } 
                    else
                    {
                        printer.myPrintf("malloc failed for result\r\n");
                        res = FR_INT_ERR;
                    }
                }
            }
        }
    }
    free(patt);
    free(patt2);
    return res;
}

void midiCallback(midi_event *pev)
// Called by the MIDIFile library when a file event needs to be processed
// thru the midi communications interface.
// This callback is set up in the setup() function.
{
#if USE_MIDI
    if ((pev->data[0] >= 0x80) && (pev->data[0] <= 0xe0))
    {
        Serial.write(pev->data[0] | pev->channel);
        Serial.write(&pev->data[1], pev->size - 1);
    }
    else
        Serial.write(pev->data, pev->size);
#endif
#if PRINT_NOTE
    USBTRACE("\n");
    USBTRACE2("millis: ", millis());
    USBTRACE2("track: ", pev->track);
    USBTRACE2("channel: ", pev->channel + 1);
    USBTRACE("data: ");
    for (uint8_t i = 0; i < pev->size; i++)
    {
        USBTRACE2("d: ", pev->data[i]);
    }
#endif
}

void sysexCallback(sysex_event *pev)
// Called by the MIDIFile library when a system Exclusive (sysex) file event needs
// to be processed through the midi communications interface. Most sysex events cannot
// really be processed, so we just ignore it here.
// This callback is set up in the setup() function.
{
    USBTRACE("sysexCallback\n");
    //DEBUG("\nS T");
    //DEBUG(pev->track);
    //DEBUG(": Data ");
    for (uint8_t i = 0; i < pev->size; i++)
    {
        // DEBUGX(pev->data[i]);
        // DEBUG(' ');
    }
}

void midiSilence(void)
// Turn everything off on every channel.
// Some midi files are badly behaved and leave notes hanging, so between songs turn
// off all the notes and sound
{
    midi_event ev;

    // All sound off
    // When All Sound Off is received all oscillators will turn off, and their volume
    // envelopes are set to zero as soon as possible.
    ev.size = 0;
    ev.data[ev.size++] = 0xb0;
    ev.data[ev.size++] = 120;
    ev.data[ev.size++] = 0;

    for (ev.channel = 0; ev.channel < 16; ev.channel++)
        midiCallback(&ev);
}

void die(int rc) //error handling
{
    printer.myPrintf("Failed with rc= \r\n");
    printer.myPrintf(rc);
    printer.myPrintf("\r\n");
}

//
void MuroBoxMIDIPlayer::MuroBoxMIDIPlayerSetup()
{
    //please reference to the definition of the functions
    SMF.setMidiHandler(midiCallback);
    SMF.setSysexHandler(sysexCallback);
}

/* A state machine allowing users to scan, play, pause and 
traverse MIDI music files stored in a USB stick with ease. */
void MuroBoxMIDIPlayer::MuroBoxMIDIPlayerLoop()
{
    static uint32_t timeStart;
    static int song_index = 0;
    static enum { S_SCAN,
                  S_IDLE,
                  S_LOAD,
                  S_PLAYING,
                  S_END,
                  S_WAIT_BETWEEN,
                  S_PAUSE } state = S_SCAN;

    current_state_player = Usb.getUsbTaskState();
    if (current_state_player == USB_STATE_RUNNING)
    {
        switch (state)
        {
        /* Scan through all files in the flash drive looking for all files 
        with a file extension of ".mid". When a valid file is found the name 
        and the path of the file is added to a list. */
        case S_SCAN:
        {
            //Only start after the MuroBoxUsbFATLoop() has successfully enable file handling fuctionalities with the mass storage device
            if ((fatready == true) && (scan_files_fail != 1))
            {
#if MD_MIDI_DEBUG
                printer.myPrintf("S_SCAN \n\r");
                printer.myPrintf("memcheck \n\r");
                int heapCheck = ESP.getFreeHeap();
                printer.myPrintf(heapCheck);
                printer.myPrintf("\n\r");
#endif
                cleared = 0;
                FRESULT rc;
                songlist = new SongList();
                if (songlist == NULL)
                {
                    printer.myPrintf("malloc failed songlist\r\n");
                    return;
                }

                CreateSonglist(songlist);

                int layer = 0;
                TCHAR *path;
                path = (TCHAR *)malloc(512);
                if (path == NULL)
                {
                    scan_files_fail = 1;
                    return;
                }
#if _LFN_UNICODE
                wcscpy(path, L"0:");
#else
                strcpy(path, "0:");
#endif
                /* Scan through all files in the flash drive looking for all files 
                with a file extension of ".mid". When a valid file is found the name 
                and the path of the file is added to a list. */
                rc = scan_files(path, layer, songlist);
                if (rc)
                {
                    scan_files_fail = 1;
                    die(rc);
                    return;
                }
                else //After successfully created the playlist, signal the motherboard entering UART mode.
                {
                    if (!midi_inserted)
                    {
                        if (pack_int_midi_insert)
                        {
                            pack_int_midi_insert(0);
                            midi_inserted = 1;
                        }
                    }
#if PRINT_MIDI_LIST
                    printTable(songlist);
#endif
                    //Sort the entire path name in the playlist into alphabetical order
                    std::sort(songlist->songlist_vector.begin(), songlist->songlist_vector.begin() + songlist->num_taken, [](const wchar_t *strA, const wchar_t *strB) {
                        return std::wcscmp(strA, strB) < 0;
                    });

#if PRINT_MIDI_LIST
                    printTable(songlist);
#endif

                    free(path);
                    //clean queue; no action before scan file.
                    clear_queue(knob_event_queue);
                    state = S_IDLE;
#if MD_MIDI_DEBUG
                    printer.myPrintf("memcheck \n\r");
                    int heapCheck = ESP.getFreeHeap();
                    printer.myPrintf(heapCheck);
                    printer.myPrintf("\n\r");
                    printer.myPrintf("S_IDLE \n\r");
#endif
                }
            }
            else
            {
                state = S_SCAN;
                return;
            }
        }
        // Wait for knob event
        case S_IDLE:
        {
            switch (getPlayerEvent())
            {
                case RESERVED:                                  break;//RESERVED
                case RELEASE: state = S_LOAD;                   return;//RELEASE
                case PRESSED: state = S_IDLE;                   return;//PRESSED
                case CLOCKWISE: state = S_LOAD;                 return;//CLOCKWISE
                case C_CLOCKWISE: state = S_LOAD;               return;//C_CLOCKWISE
                default: state = S_IDLE;                        return;
            }
            break;
        }
        // Prepare the song to play 
        case S_LOAD:
        {
#if MD_MIDI_DEBUG
            printer.myPrintf("S_LOAD \n\r");
            heapCheck = ESP.getFreeHeap();
            printer.myPrintf(heapCheck);
            printer.myPrintf("\n\r");
#endif
            if (song_index >= songlist->num_taken) //check bounds for the total number of songs in the playlist
            {
                song_index = 0; //return to the first song
                state = S_IDLE; //stop traversing
                return;
            }else if (song_index < 0)
            {
                song_index = 0; //return to the first song
            }

            int err;
            // Load the file path from the playlist to the MIDI file object
            err = SMF.load(songlist->songlist_vector[song_index]);
            song_index++;
            smf_closed = 0;
            if (err != MD_MIDIFile::E_OK) //error handling
            {
                smf_closed = 1;
                printer.myPrintf(" - SMF load Error: ");
                printer.myPrintf(err);
                timeStart = millis();
                state = S_WAIT_BETWEEN;
#if MD_MIDI_DEBUG
                printer.myPrintf("WAIT_BETWEEN \n\r");
#endif
            }
            else
            {
                //Ready to play the song
                state = S_PLAYING; 
#if MD_MIDI_DEBUG
                printer.myPrintf("S_PLAYING \n\r");
                printer.myPrintf("memcheck \n\r");
                int heapCheck = ESP.getFreeHeap();
                printer.myPrintf(heapCheck);
                printer.myPrintf("\n\r");
#endif
            }
            break;
        }
        case S_PLAYING: // play the song
        {
            if (!SMF.isEOF()) //not the end of the midi file
            {
                SMF.getNextEvent(); //read the file and parse into midi event
            }
            else
                state = S_END;

            switch (getPlayerEvent()) //check for knob event
            {
                case RESERVED:                                                      break;//RESERVED
                case RELEASE: SMF.pause(true); midiSilence(); state = S_PAUSE;      return;//RELEASE
                case PRESSED:                                                       return;//PRESSED
                case CLOCKWISE: state = S_END;                                      return;//CLOCKWISE
                case C_CLOCKWISE: song_index-=2; state = S_END;                     return;//C_CLOCKWISE
                default: state = S_PLAYING;                                         return;
            }
            break;
        }
        case S_PAUSE:
        {
            switch (getPlayerEvent()) //check for knob event
            {
                case RESERVED:                                          break;//RESERVED
                case RELEASE: SMF.pause(false); state = S_PLAYING;      return;//RELEASE
                case PRESSED:                                           return;//PRESSED
                case CLOCKWISE: state = S_END;                          return;//CLOCKWISE
                case C_CLOCKWISE: song_index-=2; state = S_END;         return;//C_CLOCKWISE
                default: state = S_PAUSE;                               return;
            }
            break;
        }
        case S_END: // done with this one
        {
#if MD_MIDI_DEBUG
            printer.myPrintf("S_END \n\r");
#endif
            if (smf_closed == 0)
            {
                SMF.close(); //close midi file 
                smf_closed = 1;
            }
            midiSilence(); // Turn everything off on every channel.
            timeStart = millis(); // start timmer for wait between the next song
#if MD_MIDI_DEBUG
            printer.myPrintf("S_WAIT_BETWEEN \n\r");
#endif
            state = S_WAIT_BETWEEN;
            break;
        }
        case S_WAIT_BETWEEN: // signal finished with a dignified pause
        {
            if (millis() - timeStart >= WAIT_DELAY)
                state = S_LOAD;
            break;
        }
        default:
            state = S_LOAD; //going to load the next song
            break;
        }
        return;
    }
    else if (!cleared) //usb flash drive dtached; reset the player 
    {
#if MD_MIDI_DEBUG
        printer.myPrintf("USB removed\r\n");
#endif
        freeTable(songlist); //free the dynamic allocated playlist 
        songlist = NULL;
        if (smf_closed == 0)
        {
            SMF.close(); //close the midi file
            smf_closed = 1;
        }
        midiSilence();
        //reset flags
        cleared = 1;
        song_index = 0;
        scan_files_fail = 0;
        midi_inserted = 0;
        state = S_SCAN;
#if MD_MIDI_DEBUG
        printer.myPrintf("memcheck \n\r");
        heapCheck = ESP.getFreeHeap();
        printer.myPrintf(heapCheck);
        printer.myPrintf("\n\r");
#endif
        return;
    }
    else
    {
        return;
    }
}
