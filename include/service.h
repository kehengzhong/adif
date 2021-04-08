/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#ifndef _SERVICE_H_
#define _SERVICE_H_

#include "btype.h"

#pragma pack(1)

typedef struct _IDSector { 
    uint16 wGenConfig; 
    uint16 wNumCyls; 
    uint16 wReserved; 
    uint16 wNumHeads; 
    uint16 wBytesPerTrack; 
    uint16 wBytesPerSector; 
    uint16 wSectorsPerTrack; 
    uint16 wVendorUnique[3]; 
    char   sSerialNumber[20]; 
    uint16 wBufferType; 
    uint16 wBufferSize; 
    uint16 wECCSize; 
    char   sFirmwareRev[8]; 
    char   sModelNumber[40]; 
    uint16 wMoreVendorUnique; 
    uint16 wDoubleWordIO; 
    uint16 wCapabilities; 
    uint16 wReserved1; 
    uint16 wPIOTiming; 
    uint16 wDMATiming; 
    uint16 wBS; 
    uint16 wNumCurrentCyls; 
    uint16 wNumCurrentHeads; 
    uint16 wNumCurrentSectorsPerTrack; 
    uint32 ulCurrentSectorCapacity; 
    uint16 wMultSectorStuff; 
    uint32 ulTotalAddressableSectors; 
    uint16 wSingleWordDMA; 
    uint16 wMultiWordDMA; 
    uint8  bReserved[128]; 
} IDSECTOR, *PIDSECTOR; 


typedef struct HDiskInfo_ {
    uint8    module[128];
    uint8    firmware[128];
    uint8    serialno[32];
    uint32   capacity;
} HDiskInfo;

#pragma pack()

#ifdef __cplusplus
extern "C" {
#endif

int get_cpu_num ();

void  sys_cpuid (uint32 i, uint32 * buf);

int   read_harddisk_info (HDiskInfo * pinfo);

#ifdef UNIX

int daemonize (char * lockfile, char * pinstalldir);
int exec_cmd  (char * cmdstr, char * argv[], int waitchild);

#endif

#ifdef __cplusplus
}
#endif

#endif

