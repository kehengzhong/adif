/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#ifndef _NATIVE_FILE_H_
#define _NATIVE_FILE_H_

#ifdef UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include "mthread.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Read or Write flag
 * NF_READ -- open the file for read, if file not exist, just return NULL
 * NF_READPLUS -- open the file for read, if file not exit, create one empty file
 * NF_WRITE -- open the file for write, if file exist, delete it or truncate to 0
 * NF_WRITEPLUS -- open the file for write, if file exist, reserve the file 
 * NF_EXEC -- create one file with executing privilege
 */
#define NF_READPLUS   0x00000002
#define NF_READ       0x00000001
#define NF_WRITEPLUS  0x00000020
#define NF_WRITE      0x00000010
#define NF_EXEC       0x10000000

#define NF_MASK_RW    0x000000FF
#define NF_MASK_R     0x0000000F
#define NF_MASK_W     0x000000F0

typedef struct native_file_ { 
 
    CRITICAL_SECTION   fileCS;
    char               name[128];
    int                flag;
    int                oflag;
    int                mode;
 
    time_t             ctime;
    time_t             mtime;
    long               inode;
    uint32             mimeid;

#ifdef UNIX
    int                fd;
#endif
#if defined(_WIN32) || defined(_WIN64)
    HANDLE             filehandle;
    int                fd;
#endif  
 
    int64              offset;
    int64              size; 
     
} NativeFile; 


void * native_file_open   (char * nfile, int flag);
void * native_file_create (char * nfile, int flag);
int    native_file_close  (void * hfile);

int    native_file_read   (void * hfile, void * buf, int size);
int    native_file_write  (void * hfile, void * buf, int size);
int    native_file_seek   (void * hfile, int64 offset);
int    native_file_copy   (void * vsrc, int64 offset, int64 length, void * vdst, int64 * actnum);
int    native_file_resize (void * hfile, int64 newsize);

char * native_file_name (void * vhfile);

#if defined(_WIN32) || defined(_WIN64)
HANDLE native_file_handle (void * vhfile);
#endif
int    native_file_fd (void * vhfile);

int64  native_file_size   (void * hfile);
int64  native_file_offset (void * hfile);

int    native_file_eof (void *vhfile);

int    native_file_attr (void * hfile, int64 * msize, time_t * mtime, long * inode, uint32 * mimeid);

int    native_file_stat (char * nfile, struct stat * st);
int    native_file_remove (char * nfile);

#ifdef __cplusplus
}
#endif

#endif

