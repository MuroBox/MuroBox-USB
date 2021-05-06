/* Copyright (C) 2021 Tevofy Technology LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").
 */
#include "MuroBoxUsbFAT.hpp"
#include "MuroBoxUtility.hpp"

#include <masstorage.h>
#include <Storage.h>
#include <PCpartition/PCPartition.h>
#include <FAT/FAT.h>
#include <Usb.h>
#include <string.h>

    /* Create one Muro Box UsbFAT Instance */
    MuroBoxUsbFAT M_UsbFAT;

volatile uint8_t current_state = 1;
volatile uint8_t last_state = 0;
volatile bool fatready = false;
volatile bool partsready = false;
volatile bool notified = false;

volatile uint32_t usbon_time;
volatile bool reportlvl = false;
int cpart = 0;
PCPartition *PT;

static PFAT *Fats[_VOLUMES];
static part_t parts[_VOLUMES];
static storage_t sto[_VOLUMES];

/*make sure this is a power of two. */
#define mbxs 512
static uint8_t My_Buff_x[mbxs]; /* File read buffer */


void MuroBoxUsbFAT::MuroBoxUsbFATSetup() {
        bool serr = false;
        for(int i = 0; i < _VOLUMES; i++) {
                Fats[i] = NULL;
                sto[i].private_data = new pvt_t;
                ((pvt_t *)sto[i].private_data)->B = 255; // impossible
        }
        // Set this to higher values to enable more debug information
        // minimum 0x00, maximum 0xff
        UsbDEBUGlvl = 0x81;


    // Initialize generic storage. This must be done before USB starts.
    Init_Generic_Storage();
}

bool isfat(uint8_t t) {
        return (t == 0x01 || t == 0x04 || t == 0x06 || t == 0x0b || t == 0x0c || t == 0x0e || t == 0x1);
}

void MuroBoxUsbFAT::MuroBoxUsbFATLoop()
{
        //yield();
        FIL My_File_Object_x; /* File object */

        current_state = Usb.getUsbTaskState();
        if(current_state != last_state) {
                if(current_state == USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE) {
                        partsready = false;
                        for(int i = 0; i < cpart; i++) {
                                if(Fats[i] != NULL)
                                        delete Fats[i];
                                Fats[i] = NULL;
                        }
                        fatready = false;
                        notified = false;
                        cpart = 0;
                }
                last_state = current_state;
        }

// only do any of this if usb is on

        if(partsready && !fatready) {
                if(cpart > 0) fatready = true;
        }
        // This is horrible, and needs to be moved elsewhere!
        for(int B = 0; B < MAX_USB_MS_DRIVERS; B++) {
                if((!partsready) && (UHS_USB_BulkOnly[B]->GetAddress())) {
                        // Build a list.
                        int ML = UHS_USB_BulkOnly[B]->GetbMaxLUN();
                        //printf("MAXLUN = %i\r\n", ML);
                        ML++;
                        for(int i = 0; i < ML; i++) {
                                if(UHS_USB_BulkOnly[B]->LUNIsGood(i)) {
                                        partsready = true;
                                        ((pvt_t *)(sto[i].private_data))->lun = i;
                                        ((pvt_t *)(sto[i].private_data))->B = B;
                                        sto[i].Reads = *UHS_USB_BulkOnly_Read;
                                        sto[i].Writes = *UHS_USB_BulkOnly_Write;
                                        sto[i].Status = *UHS_USB_BulkOnly_Status;
                                        sto[i].Initialize = *UHS_USB_BulkOnly_Initialize;
                                        sto[i].Commit = *UHS_USB_BulkOnly_Commit;
                                        sto[i].TotalSectors = UHS_USB_BulkOnly[B]->GetCapacity(i);
                                        sto[i].SectorSize = UHS_USB_BulkOnly[B]->GetSectorSize(i);
                                        //printf_P(PSTR("LUN:\t\t%u\r\n"), i);
                                        //printer.myPrintf("LUN:\t\t\r\n");
                                        //printer.myPrintf(i);
                                        //printf_P(PSTR("Total Sectors:\t%08lx\t%lu\r\n"), sto[i].TotalSectors, sto[i].TotalSectors);
                                        //printer.myPrintf("Total Sectors: ");
                                        //printer.myPrintf(sto[i].TotalSectors);
                                        //printf_P(PSTR("Sector Size:\t%04x\t\t%u\r\n"), sto[i].SectorSize, sto[i].SectorSize);
                                        //printer.myPrintf("Sector Size: \r\n");
                                        //printer.myPrintf(sto[i].SectorSize);
                                        // get the partition data...
                                        PT = new PCPartition;
                                        if(!PT->Init(&sto[i])) {
                                                part_t *apart;
                                                for(int j = 0; j < 4; j++) {
                                                        apart = PT->GetPart(j);
                                                        if(apart != NULL && apart->type != 0x00) {
                                                                memcpy(&(parts[cpart]), apart, sizeof (part_t));
                                                                //printf_P(PSTR("Partition %u type %#02x\r\n"), j, parts[cpart].type);
                                                                // for now
                                                                if(isfat(parts[cpart].type)) {
                                                                        Fats[cpart] = new PFAT(&sto[i], cpart, parts[cpart].firstSector);
                                                                        //int r = Fats[cpart]->Good();
                                                                        if(Fats[cpart]->MountStatus()) {
                                                                                delete Fats[cpart];
                                                                                Fats[cpart] = NULL;
                                                                        } else cpart++;
                                                                }
                                                        }
                                                }
                                        } else {
                                                // try superblock
                                                Fats[cpart] = new PFAT(&sto[i], cpart, 0);
                                                //int r = Fats[cpart]->Good();
                                                if(Fats[cpart]->MountStatus()) {
                                                        //printf_P(PSTR("Superblock error %x\r\n"), r);
                                                        delete Fats[cpart];
                                                        Fats[cpart] = NULL;
                                                } else cpart++;
                                        }
                                        delete PT;
                                } else {
                                        sto[i].Writes = NULL;
                                        sto[i].Reads = NULL;
                                        sto[i].Initialize = NULL;
                                        sto[i].TotalSectors = 0UL;
                                        sto[i].SectorSize = 0;
                                }
                        }
                }
        }
        if(fatready) {
                if(Fats[0] != NULL) {
                        struct Pvt * p;
                        p = ((struct Pvt *)(Fats[0]->storage->private_data));
                        if(!UHS_USB_BulkOnly[p->B]->LUNIsGood(p->lun)) {
                                // media change
                                partsready = false;
                                for(int i = 0; i < cpart; i++) {
                                        if(Fats[i] != NULL)
                                                delete Fats[i];
                                        Fats[cpart] = NULL;
                                }
                                fatready = false;
                                notified = false;
                                cpart = 0;
                        }
                }
        }

        if(fatready) {
                UINT bw, br, i;
                if(!notified) {
                        notified = true;
                        FATFS *fs = NULL;
                        for(int zz = 0; zz < _VOLUMES; zz++) {
                                if(Fats[zz]->volmap == 0) fs = Fats[zz]->ffs;
                        }

                        goto out;
                        return;
                out:
                        DISK_IOCTL(fs->drv, CTRL_COMMIT, 0);
                        // printf_P(PSTR("\r\nTest CTRL_COMMIT, "));
                        USBTRACE("fat ready\r\n");
                        return;
                }
        }

        return;
}
