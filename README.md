# Arduino Project for Muro Box USB Interface
# Introduction


This project is dedicated to the development of the Muro Box USB interface, providing additional means of MIDI input to the Muro Box. As of now, the USB interface supports USB MIDI Keyboard, USB flash drive, and PC USB MIDI output. 

Muro Box is the world’s first App-controlled music box. For more information on Muro Box: [Muro Box official website](https://murobox.com/en/)  

# Overview of the Project
This is an Arduino project that runs on an ESP8266 chip to serve as an add-on board to the motherboard of the Muro Box. 

The add-on features can be deconstructed into two main subsystems. First, Muro Box acts as a host device supporting peripheral devices: USB MIDI Keyboard and USB flash drive. Second, Muro Box acts as a peripheral device connecting to a host device, for USB MIDI output from a PC (Windows /macOS). 

This project is heavily based on [USB Host Shileld](https://chome.nerpa.tech/arduino_usb_host_shield_projects/) project, a MAX3421E-based USB Host Shield Library.

## Schematic and PCB Layout 
[MuroBox-USB-schematic](hardware/MuroBox-USB-schematic.pdf)  
[MuroBox-USB-pcb](hardware/MuroBox-USB-pcb.pdf)  

The add-on board is connected to the motherboard with only a few wire connections.  
- UART is used for serial communication (MIDI) between ESP8266 and the chip on the motherboard.
- PACK_INT is used for signaling the motherboard entering add-on UART mode.
- PACK_N_RESET is used for asserting the RES Pin on ESP8266 and MAX3421E
- 3.3 and 5-volt power supply and GND
- Rotary Encoder PINs for the knob (bypass ESP8266)

## Detection and Transition between Host Mode and Peripheral Mode

The MAX3421E adds USB host or peripheral capability to any system with an SPI interface. A mechanism is developed to automatically identify peripheral or host external devices and initialize the MAX3421E accordingly. The logic behind the mechanism is explained down below:

Vbus is wired to a Vbus comparator in MAX3421. Vbus makes a 0-1 transition when either a host or peripheral device plugs in. GPIO6 of MAX3421 is connected to an output pin (ID) of TUSB320. TUSB320 uses the CC pins to determine port attach and detach, cable orientation, role detection, and port control for Type-C current mode. A Vbus transition with GPIO6 in high voltage level indicates a peripheral device is plugged in. On the other hand, a Vbus transition without GPIO6 in a high voltage level indicates a host device is plugged in. When a device is detached, Vbus makes a 1-0 transition.
            
|      | VBUS | GPIO6 |
|------|------|-------|
|DEVICE|  0-1 |   1   |
| HOST |  0-1 |   0   |
|DETACH|  1-0 |   0   |

Once the device is being detected and identified, the corresponding subsystem will be executed.
## Host Mode

Four major components are running in host mode.

-   **MuroBoxUSBHostLoop**
The MuroBoxUSBHostLoop performs connectivity detection and enumeration for peripheral devices.

-   **MuroBoxMIDILoop**
The MuroBoxMIDILoop supports functionalities of MIDI devices (MIDI keyboard) and parsing MIDI SysEx messages received from the motherboard. 

-   **MuroBoxUsbFATLoop**
The MuroBoxUsbFATLoop supports mass storage devices (USB flash drive) and file handling.

-   **MuroBoxMIDIPlayerLoop**
The MuroBoxMIDIPlayerLoop contains a state machine allowing users to scan, play, pause and traverse MIDI music files stored in a USB flash drive with ease.

### External Dependencies
Multiple libraries are integrated into this subsystem with modifications. Changes are listed below:
- [MD_MIDIFile-main](https://github.com/MajicDesigns/MD_MIDIFile): replace filesystem library from SdFat to [FatFs](http://elm-chan.org/fsw/ff/00index_e.html) for compatibility with [generic_storage](https://github.com/xxxajk/generic_storage).
- [MIDI_Library-5.0.2](https://www.arduino.cc/reference/en/libraries/midi-library/): change settings to allow multiple bytes parsing for SysEx messages and allow 0xFD as a defined command byte.
- [USB_Host_Shield_Library_2.0](https://github.com/felis/USB_Host_Shield_2.0): replace RTClib with ezTime, remove xmem2, take out unused source files, and handel watchdog reset (WDT) specifically for ESP8266. (Version: Feb 29, 2020. Please check its [GitHub page](https://github.com/felis/USB_Host_Shield_2.0)  for  newer commits)

## Peripheral Mode
**MuroBoxPeripheralLoop**
The process for enumerating as a MIDI device is similar to enumerating an HID keyboard demonstrated in the application note provided by Maxim. For a detailed explanation of the code, please reference the application note [USB Enumeration Code (and More) for the MAX3420E](https://www.maximintegrated.com/en/design/technical-documents/app-notes/3/3690.html).

There are a few differences between the MuroBoxPeripheralLoop and the Maxim application note: 
- The Configuration Descriptor is tailored for MIDI devices instead of HID devices.
- Process Out Token Packet instead of IN Token Packet
- Take out Suspend/Resume Feature

# Environment Setup

## Getting Started -- Arduino on ESP8266

- Step 1: Install the [Arduino Desktop IDE](https://www.arduino.cc/en/Guide
)

- Step 2: Download and unzip or clone the repository “Muro_box-Addon”. 
Note: the folder name has to be the same as the .ino file.
- Step 3: Config IDE 
    > - Start Arduino IDE and open the preference window.
    > - Copy and paste Board Manager URL to your preferences
    > Arduino >> preference -> Additional boards manager URLs: http://arduino.esp8266.com/stable/package_esp8266com_index.json
    > - Arduino >> Tools >> board >> boards manager -> install esp8266
    > - Arduino >> Tools >> board: select Generic ESP8266 Module 
    
    More Info: [how-to-install-esp8266-board-arduino-ide](https://randomnerdtutorials.com/how-to-install-esp8266-board-arduino-ide/)

- Step 4: Config setting options under the “Tool” menu
     > Board: "Generic ESP8266 Module"  
     Builtin Led: "O"  
     Upload Speed: "115200"  
     CPU Frequency: "160 MHz"  
     Crystal Frequency: "26 MHz"  
     Flash Size: "2MB (FS:512KB OTA:~768KB)"  
     Flash Mode: "QI0 (fast)"  
     Flash Frequency: "80MHz"  
     Reset Method: "dtr (aka nodemcu)"  
     Debug port: "Disabled"  
     Debug Level: "None"  
     IwIP Variant: "v2 Lower Memory"  
     VTables: "Flash"  
     Exceptions: "Legacy (new can return nullptr)"  
      Erase Flash: "Only Sketch"  
    Espressif FW: "nonos-sdk 2.2.1+119 (191122)"  
    SSL Support: "Basic SSL ciphers (lower ROM use)"  
    
 - Step 5: Install Arduino Libraries from Arduino Library Manager in IDE 
	> Arduino >> Tools >> Manage Libraries
	Install the following libraries: 
	>>ArduinoJson (by Benoit Blanchon)   
	ezTime (by Rop Gonggrijp)

- Step 6: Install User-Installed Libraries  
	User-installed libraries should be installed in the sketchbook libraries folder.  
	Create symbolic links to properly include all libraries from this repository.  
    >  **source folder <- symbolic link** 
    > `ln -s source_path symbolic_link`(for macOS)
    >>**symbolic links location**:   "libraries" folder inside Arduino sketchbook
    >> (The path to the sketchbook folder can be found in Arduino >> preference -> Sketchbook location)  
    >> **source folders**:  
    >> Muro_box-Addon/libraries/ESPSoftwareSerial  
    >> Muro_box-Addon/libraries/generic_storage  
    >> Muro_box-Addon/libraries/MD_MIDIFile-main  
    >> Muro_box-Addon/libraries/MIDI_Library-5.0.2  
    >> Muro_box-Addon/libraries/USB_Host_Shield_Library_2.0  

	More Info: [Installing Libraries Manually](https://learn.adafruit.com/adafruit-all-about-arduino-libraries-install-use/how-to-install-a-library)
- The final directory tree should look like this: 
```
Muro_box-Addon
 └── libraries
      ├── ESPSoftwareSerial
      ├── MD_MIDIFile-main
      ├── MIDI_Library-5.0.2
      ├── USB_Host_Shield_Library_2.0
      └── generic_storage
```
```
Arduino
 └── libraries
      ├── ArduinoJson
      ├── ezTime
      ├── ESPSoftwareSerial -> ../Muro_box-Addon/libraries/ESPSoftwareSerial
      ├── MD_MIDIFile-main -> ../Muro_box-Addon/libraries/MD_MIDIFile-main
      ├── MIDI_Library-5.0.2 -> ../Muro_box-Addon/libraries/MIDI_Library-5.0.2
      ├── USB_Host_Shield_Library_2.0 -> ../Muro_box-Addon/libraries/USB_Host_Shield_Library_2.0
      └── generic_storage -> ../Muro_box-Addon/libraries/generic_storage
```

## Hardware Setup
![pcb_board](https://user-images.githubusercontent.com/57285584/117103163-67a85f80-adac-11eb-9821-e9235ae1e407.jpeg)

Solder a 2 pin 2.54 mm connector plug female on J3 and two 3 pin 2.0 mm connector plug female on MIDI and DEBUG port, demonstrated down below. 
![pcb_board_connector](https://user-images.githubusercontent.com/57285584/117103254-8f97c300-adac-11eb-87b6-a52146dcb904.jpeg)

- MIDI: mainly used as the default serial port for uploading Arduino sketches, and MIDI communication between the subboard and the motherboard. 
- DEBUG: mainly used for software serial print (baud rate: 115200)
- J3: mainly used for entering flash download mode by wire GPIO0 to ground with a jumper

A USB to TTL Converter Module with 3 pin connector plug male is also needed. Recommended models: TTL-232R-3V3

## Compile and Upload Sketch
There are two ways to compile and upload a sketch:
-   UART Update in Arduino IDE
	1.  Choose the correct serial port under the Tool menu
	2.  Connect J3 with a jumper
	3.  Connect serial port to the MIDI connector plug
	4.  Put ESP8266 into download mode by power reset or pushing the reset button on the motherboard
	5.  Click the Upload icon on the IDE.
	6. If an error persistently occurred while uploading the sketch, temporarily remove the RXD pin connection and try again <img src="https://user-images.githubusercontent.com/57285584/117230914-b14a8600-ae50-11eb-830f-8249cc385bc2.jpg" width="300" height="350">


    
-   OTA Update Using Web Browser
	1.  In Arduino IDE, Arduino >> Sketch >> Export Compiled Binary  
	2.  Establish Wi-Fi connection on PC to ESP8266 named MuroBox-Addon-Webupdate
	3.  Visit http://MuroBox-Addon-Webupdate.local/update in any browser <img width="576" alt="Screen Shot 2021-05-06 at 10 55 49 AM" src="https://user-images.githubusercontent.com/57285584/117235696-c7107900-ae59-11eb-818a-4dc087997967.png">
	4.  Upload the binary file to update firmware (usually found in project repository, named Muro_box-Addon.ino.generic.bin)
	5.  The ESP8266 will reboot with the updated firmware.
	 
	 More information about [OTA Updates](https://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html)
## Useful Links
- [MAX3421E PROGRAMMING GUIDE](https://www.maximintegrated.com/en/design/technical-documents/app-notes/3/3785.html)
- [MAX3420E PROGRAMMING GUIDE](https://www.maximintegrated.com/en/design/technical-documents/app-notes/3/3598.html)
- [ESP-WROOM-02D](https://www.espressif.com/sites/default/files/documentation/esp-wroom-02u_esp-wroom-02d_datasheet_en.pdf)
- [USB in a NutShell](https://www.beyondlogic.org/usbnutshell/usb1.shtml)
- [Standard MIDI Files](https://arduinoplusplus.wordpress.com/2018/05/11/playing-midi-files-on-arduino-part-1-standard-midi-files/)
## Contributors

- Chen-Hsiang Feng <chen-hsiang.feng@tevofy.com>
- Yi-Fang Hsiung <hsiung@purdue.edu> 

## License
Copyright (C) 2021 Tevofy Technology LTD. All rights reserved.
Licensed under the [General Public License v2.0](LICENSE)
