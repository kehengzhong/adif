/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include "service.h"
#include "frame.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <windef.h>
#include <lmcons.h>
#include <lmshare.h>
#include <tlhelp32.h>
#endif

#ifdef UNIX
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <pwd.h>
#include <unistd.h>
#ifdef _LINUX_
#include <sys/io.h>
#include <sys/sysinfo.h>
#endif
#endif

int get_cpu_num ()
{
    int    num = 0;

#if defined(_WIN32) || defined(_WIN64)
    SYSTEM_INFO sysinfo;

    GetSystemInfo( &sysinfo );
    num = sysinfo.dwNumberOfProcessors;

#elif defined UNIX
    num = sysconf(_SC_NPROCESSORS_ONLN);
#ifdef _LINUX_
    if (num <= 0) {
        num = get_nprocs();
    }
#endif
#endif

    return num;
}


void ChangeByteOrder (uint8 * szString, int uscStrSize) 
{ 
    int i; 
    uint8 temp; 

    for (i = 0; i < uscStrSize; i+=2) { 
        temp = szString[i]; 
        szString[i] = szString[i+1]; 
        szString[i+1] = temp; 
    } 
} 

 
void sys_cpuid (uint32 i, uint32 * buf);
 
#ifdef UNIX

#if ( __i386__ )
 
void sys_cpuid (uint32 i, uint32 * buf)
{
    /*
     * we could not use %ebx as output parameter if gcc builds PIC,
     * and we could not save %ebx on stack, because %esp is used,
     * when the -fomit-frame-pointer optimization is specified.
     */
 
    __asm__ (
 
    "    mov    %%ebx, %%esi;  "
 
    "    cpuid;                "
    "    mov    %%eax, (%1);   "
    "    mov    %%ebx, 4(%1);  "
    "    mov    %%edx, 8(%1);  "
    "    mov    %%ecx, 12(%1); "
 
    "    mov    %%esi, %%ebx;  "
 
    : : "a" (i), "D" (buf) : "ecx", "edx", "esi", "memory" );
}
 
 
#else /* __amd64__ */

inline void sys_cpuid (uint32 i, uint32 * buf)
{
    uint32  eax, ebx, ecx, edx;
 
    __asm__ (

    "cpuid"
 
    : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (i) );
 
    buf[0] = eax;
    buf[1] = ebx;
    buf[2] = edx;
    buf[3] = ecx;
}
 
#endif
 
#ifdef _LINUX_

static int WaitIde()
{
    int al;

    while ((al=inb(0x1F7))>=0x80) ;
    return al;
}

static int ReadIDE(frame_p frame, uint8 swap)
{
    int      al;
    int      i;
    uint16   pw = 0;

    ioperm(0x1f0, 16, 1);

    WaitIde();
    outb(0xA0,0x1F6);
    al = WaitIde();
    if ((al&0x50)!=0x50) return -1;

    outb(0xA0,0x1F6);
    outb(0xEC,0x1F7);
    al = WaitIde();
    if ((al&0x58)!=0x58) return -2;

    for (i = 0; i < 256; i++) {
        pw = inw(0x1F0);
        //if (swap) pw = swap2(pw);
        frame_put_nlast(frame, &pw, 2);
    }

    return 0;
}

int read_harddisk_info (HDiskInfo * pinfo)
{
    frame_p    frame = NULL;
    int        ret = 0;
    PIDSECTOR  phdinfo = NULL;

    if (!pinfo) return -1;
    memset(pinfo, 0, sizeof(HDiskInfo));

    frame = frame_new(256);

    ret = ReadIDE(frame, 0);
    if (ret < 0) {
        frame_delete(&frame);
        return -100;
    }

    phdinfo = (PIDSECTOR)frameP(frame);
    memcpy(pinfo->module, phdinfo->sModelNumber,40); 
    memcpy(pinfo->firmware, phdinfo->sFirmwareRev,8); 
    memcpy(pinfo->serialno, phdinfo->sSerialNumber,20); 
    pinfo->capacity = phdinfo->ulTotalAddressableSectors/2/1024; 

    if (!isBigEndian()) {
        ChangeByteOrder(pinfo->module, 40);
        ChangeByteOrder(pinfo->firmware, 8);
        ChangeByteOrder(pinfo->serialno, 20);
    }
    
    frame_delete(&frame);

    return 0;
}
#endif


int daemonize (char * lockfile, char * pinstalldir)
{
    int i,lfp;
    char str[80];

    if (getppid() == 1) 
    return -1; /* already a daemon */

    i = fork();
    if (i < 0) exit(1); /* fork error */
    if (i > 0) {
        wait(&lfp);
        exit(0); /* parent exits */
    }

    /* child (daemon) continues */
    setsid(); /* obtain a new process group */

    i = fork();
    if (i < 0) exit(1); /* fork error */
    if (i > 0) exit(0); /* parent exits */
    
    for (i = 2/*getdtablesize()*/; i >= 0; --i) 
        close(i); /* close all descriptors */

    i = open("/dev/null",O_RDWR); 
    dup(i); 
    dup(i); /* handle standart I/O */

    umask(0); /* set newly created file permissions */

    if (!pinstalldir || strlen(pinstalldir) < 1)
        chdir( "." );
    else
        chdir(pinstalldir);

    lfp = open(lockfile, O_RDWR|O_CREAT, 0640);
    if (lfp < 0) exit(1); /* can not open */

    if (lockf(lfp, F_TLOCK, 0) < 0) 
    exit(0); /* can not lock */

    /* first instance continues */
    sprintf(str, "%ld\n", (long)getpid());
    write(lfp, str, strlen(str)); /* record pid to lockfile */

    return lfp;
}


int exec_cmd (char *cmdstr, char * argv[], int waitchild)
{
    pid_t   pid = 0;
    int     status = 0;
    int     i = 0;
    int     maxfd = 0;
 
    if (cmdstr == NULL)
        return(1);   /* always a command processor with UNIX */
 
    if ((pid = fork()) < 0) {
        status = -1;    /* probably out of processes */
    } else if (pid == 0) { /* child */
        //long i, maxfd = sysconf(_SC_OPEN_MAX);
        //for (i=0; i<maxfd; ++i)
        //    close(i);


        if (waitchild == 0) {
            if ((pid = fork()) < 0) exit(-1);
            else if (pid > 0) exit(0);

            maxfd = getdtablesize();
            for (i=0; i < maxfd; ++i)
                close(i); /* close all descriptors */

            SLEEP(1000);
        }

        execv(cmdstr, argv);
        _exit(127);     /* execl error */
    } else {  /* parent */
        while (waitchild && waitpid(pid, &status, 0) < 0) {
            if (errno != EINTR) {
                status = -1; /* error other than EINTR from waitpid() */
                break;
            }
        }
        if (waitchild == 0) {
            wait(&status);
        }
    }
 
    return(status);
}

int runas(char * user)
{
    struct passwd * pw_ent = NULL;

    if (user != NULL) {
        pw_ent = getpwnam(user);
    }

    if (pw_ent != NULL) {
        if (setegid(pw_ent->pw_gid) != 0) {
            return -1;
        }

        if (seteuid(pw_ent->pw_uid) != 0) {
            return -1;
        }
    }

    return 0;
}

#endif  //end ifdef UNIX


#if defined(_WIN32) || defined(_WIN64)

#define DFP_GET_VERSION        0x00074080 
#define DFP_SEND_DRIVE_COMMAND 0x0007c084 
#define DFP_RECEIVE_DRIVE_DATA 0x0007c088 

#pragma pack(1) 
typedef struct _GETVERSIONOUTPARAMS { 
    BYTE  bVersion;       // Binary driver version. 
    BYTE  bRevision;      // Binary driver revision. 
    BYTE  bReserved;      // Not used. 
    BYTE  bIDEDeviceMap;  // Bit map of IDE devices. 
    DWORD fCapabilities;  // Bit mask of driver capabilities. 
    DWORD dwReserved[4];  // For future use. 
} GETVERSIONOUTPARAMS, *PGETVERSIONOUTPARAMS, *LPGETVERSIONOUTPARAMS; 

#if 0
typedef struct _IDEREGS { 
    BYTE bFeaturesReg;     // Used for specifying SMART "commands". 
    BYTE bSectorCountReg;  // IDE sector count register 
    BYTE bSectorNumberReg; // IDE sector number register 
    BYTE bCylLowReg;       // IDE low order cylinder value 
    BYTE bCylHighReg;      // IDE high order cylinder value 
    BYTE bDriveHeadReg;    // IDE drive/head register 
    BYTE bCommandReg;      // Actual IDE command. 
    BYTE bReserved;        // reserved for future use.  Must be zero. 
} IDEREGS, *PIDEREGS, *LPIDEREGS; 

typedef struct _SENDCMDINPARAMS { 
    DWORD   cBufferSize;    // Buffer size in bytes 
    IDEREGS irDriveRegs;    // Structure with drive register values. 
    BYTE    bDriveNumber;   // Physical drive number to send 
                            // command to (0,1,2,3). 
    BYTE    bReserved[3];   // Reserved for future expansion. 
    DWORD   dwReserved[4];  // For future use. 
    //BYTE  bBuffer[1];     // Input buffer. 
} SENDCMDINPARAMS, *PSENDCMDINPARAMS, *LPSENDCMDINPARAMS; 

typedef struct _DRIVERSTATUS { 
    BYTE  bDriverError;  // Error code from driver, 
                         // or 0 if no error. 
    BYTE  bIDEStatus;    // Contents of IDE Error register. 
                         // Only valid when bDriverError 
                         // is SMART_IDE_ERROR. 
    BYTE  bReserved[2];  // Reserved for future expansion. 
    DWORD dwReserved[2]; // Reserved for future expansion. 
} DRIVERSTATUS, *PDRIVERSTATUS, *LPDRIVERSTATUS; 

typedef struct _SENDCMDOUTPARAMS { 
    DWORD        cBufferSize;  // Size of bBuffer in bytes 
    DRIVERSTATUS DriverStatus; // Driver status structure. 
    BYTE         bBuffer[512]; // Buffer of arbitrary length 
                               // in which to store the data read from the drive. 
} SENDCMDOUTPARAMS, *PSENDCMDOUTPARAMS, *LPSENDCMDOUTPARAMS; 
#endif

#pragma pack() 


void hdid9x (HDiskInfo * pinfo) 
{ 
    PIDSECTOR           phdinfo; 
    GETVERSIONOUTPARAMS vers; 
    SENDCMDINPARAMS     in; 
    SENDCMDOUTPARAMS    out; 
    HANDLE              h;  
    DWORD               i; 
    BYTE                j; 

    ZeroMemory(&vers, sizeof(vers)); 
    //We start in 95/98/Me 
    h = CreateFile("\\\\.\\Smartvsd", 0, 0, 0, CREATE_NEW, 0, 0); 
    if (h == INVALID_HANDLE_VALUE) { 
        //open smartvsd.vxd failed 
        return; 
    } 

    if (!DeviceIoControl(h, DFP_GET_VERSION, 0, 0, &vers, sizeof(vers), &i, 0)) { 
     //DeviceIoControl failed: DFP_GET_VERSION 
     CloseHandle(h); 
     return; 
    } 
    //If IDE identify command not supported, fails 
    if (!(vers.fCapabilities & 1)){ 
        //Error: IDE identify command not supported. 
        CloseHandle(h); 
        return; 
    } 
    //Identify the IDE drives 
    for (j=0; j<4; j++){ 

        ZeroMemory(&in,sizeof(in)); 
        ZeroMemory(&out,sizeof(out)); 
        if (j & 1) { 
            in.irDriveRegs.bDriveHeadReg=0xb0; 
        } else { 
            in.irDriveRegs.bDriveHeadReg=0xa0; 
        } 
        if (vers.fCapabilities&(16>>j)) { 
            //We don't detect a ATAPI device. 
            //Drive j+1 is a ATAPI device, we don't detect it 
            continue; 
        } else { 
            in.irDriveRegs.bCommandReg = 0xec; 
        } 
        in.bDriveNumber = j; 
        in.irDriveRegs.bSectorCountReg = 1; 
        in.irDriveRegs.bSectorNumberReg = 1; 
        in.cBufferSize = 512; 
        if (!DeviceIoControl(h, DFP_RECEIVE_DRIVE_DATA, &in, sizeof(in), &out, sizeof(out), &i, 0)) { 
            //DeviceIoControl failed: DFP_RECEIVE_DRIVE_DATA
            CloseHandle(h); 
            return; 
        } 
        phdinfo=(PIDSECTOR)out.bBuffer; 
        memcpy(pinfo->module, phdinfo->sModelNumber,40); 
        memcpy(pinfo->firmware, phdinfo->sFirmwareRev,8); 
        memcpy(pinfo->serialno, phdinfo->sSerialNumber,20); 
        pinfo->capacity = phdinfo->ulTotalAddressableSectors/2/1024; 
        if (!isBigEndian()) {
            ChangeByteOrder(pinfo->module, 40);
            ChangeByteOrder(pinfo->firmware, 8);
            ChangeByteOrder(pinfo->serialno, 20);
        }
    } 

    //Close handle before quit 
    CloseHandle(h); 
} 


void hdidnt(HDiskInfo * pinfo)
{ 
    char                hd[80]; 
    PIDSECTOR           phdinfo; 
    GETVERSIONOUTPARAMS vers; 
    SENDCMDINPARAMS     in; 
    SENDCMDOUTPARAMS    out; 
    HANDLE              h;  
    DWORD               i; 
    BYTE                j; 

    ZeroMemory(&vers,sizeof(vers)); 
    //We start in NT/Win2000 
    for (j=0; j<4; j++) { 
        sprintf(hd, "\\\\.\\PhysicalDrive%d",j); 
        h = CreateFile(hd, GENERIC_READ|GENERIC_WRITE, 
                   FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING,0,0); 
        if (h == INVALID_HANDLE_VALUE) { 
            continue; 
        } 
        if (!DeviceIoControl(h, DFP_GET_VERSION, 0, 0, &vers, sizeof(vers), &i, 0)) { 
            CloseHandle(h); 
            continue; 
        } 
        //If IDE identify command not supported, fails 
        if (!(vers.fCapabilities & 1)) { 
            //Error: IDE identify command not supported 
               CloseHandle(h); 
            return; 
        } 
        //Identify the IDE drives 
        ZeroMemory(&in,sizeof(in)); 
        ZeroMemory(&out,sizeof(out)); 
        if (j&1) { 
            in.irDriveRegs.bDriveHeadReg = 0xb0; 
        } else { 
            in.irDriveRegs.bDriveHeadReg = 0xa0; 
        } 
        if (vers.fCapabilities&(16>>j)) { 
            //We don't detect a ATAPI device. 
            //Drive j+1 is a ATAPI device, we don't detect it 
            continue; 
        } else { 
            in.irDriveRegs.bCommandReg = 0xec; 
        } 
        in.bDriveNumber = j; 
        in.irDriveRegs.bSectorCountReg = 1; 
        in.irDriveRegs.bSectorNumberReg = 1; 
        in.cBufferSize = 512; 
        if (!DeviceIoControl(h, DFP_RECEIVE_DRIVE_DATA, &in, sizeof(in), &out, sizeof(out), &i, 0)) { 
            //DeviceIoControl failed: DFP_RECEIVE_DRIVE_DATA 
            CloseHandle(h); 
            return; 
        } 

        phdinfo = (PIDSECTOR)out.bBuffer; 
        memcpy(pinfo->module, phdinfo->sModelNumber,40); 
        memcpy(pinfo->firmware, phdinfo->sFirmwareRev,8); 
        memcpy(pinfo->serialno, phdinfo->sSerialNumber,20); 
        pinfo->capacity = phdinfo->ulTotalAddressableSectors/2/1024; 
        if (!isBigEndian()) {
            ChangeByteOrder(pinfo->module, 40);
            ChangeByteOrder(pinfo->firmware, 8);
            ChangeByteOrder(pinfo->serialno, 20);
        }

        CloseHandle(h); 
    } 
    return; 
} 


int read_harddisk_info (HDiskInfo * pinfo)
{ 
    OSVERSIONINFO VersionInfo; 

    if (!pinfo) return -1;
    memset(pinfo, 0, sizeof(HDiskInfo));

    ZeroMemory(&VersionInfo, sizeof(VersionInfo)); 
    VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo); 
    GetVersionEx(&VersionInfo); 

    switch (VersionInfo.dwPlatformId) { 
    case VER_PLATFORM_WIN32s: 
        //Win32s is not supported by this programm 
        return -100; 
    case VER_PLATFORM_WIN32_WINDOWS: 
        hdid9x(pinfo); 
        break; 
    case VER_PLATFORM_WIN32_NT: 
        hdidnt(pinfo); 
        break; 
    } 
    return 0;
} 

#endif
