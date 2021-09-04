/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#ifndef __BTYPE_H__
#define __BTYPE_H__


#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#pragma comment(lib,"Ws2_32.lib")
#include <windows.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#ifdef UNIX
#include <unistd.h>
#include <sys/time.h>
#include <strings.h>
#endif

typedef unsigned long long  uint64;
typedef long long           sint64;
typedef long long           int64;
typedef unsigned long       ulong;
typedef unsigned int        uint32;
typedef int                 sint32;
typedef int                 int32;
typedef unsigned short int  uint16;
typedef short int           sint16;
typedef short int           int16;
typedef unsigned char       uint8;
typedef char                sint8;
typedef char                int8;

#define isBigEndian() ((*(uint16 *) ("KE") >> 8) == 'K')

#define rol(x,n) ( ((x) << (n)) | ((x) >> (32-(n))) )
#define rol2(x,n)( ((x) << (n)) | ((x) >> (64-(n))) )
#define swap2(b) ( ((uint16)(b)<<8)|((uint16)(b)>>8) )
#define swap4(b) ( (rol((uint32)(b), 24) & 0xff00ff00) | (rol((uint32)(b), 8) & 0x00ff00ff) )
#define swap8(b) ( (rol2((uint64)(b),8 )&0x000000FF000000FFULL) | \
                   (rol2((uint64)(b),24)&0x0000FF000000FF00ULL) | \
                   (rol2((uint64)(b),40)&0x00FF000000FF0000ULL) | \
                   (rol2((uint64)(b),56)&0xFF000000FF000000ULL) )

#ifndef min
#define min(x,y)  ((x) <= (y)?(x):(y))
#endif
#ifndef max
#define max(x,y)  ((x) >= (y)?(x):(y))
#endif
#ifndef htonll
#define htonll(b) ( isBigEndian()?(b):swap8(b) )
#endif
#ifndef ntohll
#define ntohll(b) ( isBigEndian()?(b):swap8(b) )
#endif
#ifndef adf_tolower
#define adf_tolower(c) (int)((c >= 'A' && c <= 'Z') ? (c | 0x20) : c)
#endif
#ifndef adf_toupper
#define adf_toupper(c) (int)((c >= 'a' && c <= 'z') ? (c & ~0x20) : c)
#endif
#ifndef tv_diff_us
#define tv_diff_us(t1,t2) (((t2)->tv_sec-(t1)->tv_sec)*1000000+(t2)->tv_usec-(t1)->tv_usec)
#endif

#ifndef ULONG_MAX
#define  ULONG_MAX   (-1)UL 
#endif
#ifndef  TRUE
#define  TRUE               1
#endif
#ifndef  FALSE
#define  FALSE              0
#endif


#ifdef UNIX

#ifndef SLEEP 
#define SLEEP(x) usleep((x)*1000)
#endif

#ifndef SOCKET
#define SOCKET int
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET   -1
#endif

#ifndef WSAGetLastError
#define WSAGetLastError() errno
#endif

#ifndef closesocket
#define closesocket close
#endif

#ifndef OutputDebugString
#define OutputDebugString printf
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#endif /* end if UNIX */


#if defined(_WIN32) || defined(_WIN64)

struct iovec {
    void   * iov_base;    /* Starting address */
    size_t   iov_len;     /* Number of bytes to transfer */
};

#ifndef SLEEP 
#define SLEEP Sleep
#endif

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif

#ifndef srandom
#define srandom srand
#endif

#ifndef random
#define random rand
#endif

#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif

#ifndef snprintf
#define snprintf _snprintf
#endif

#define MSG_NOSIGNAL 0

#if _MSC_VER < 1300
#define strtoll(p,e,b) ((*(e)=(char*)(p)+(((b)== 0)?strspn((p),"0123456789"):0)),_atoi64(p))
#else
#define strtoll(p, e, b) _strtoi64(p, e, b) 
#endif

#ifndef strtoull
#define strtoull strtoul
#endif

int gettimeofday(struct timeval *tv, struct timezone *tz);

#pragma warning(disable : 4996)
#pragma warning(disable : 4244)
#pragma warning(disable : 4133)
#pragma warning(disable : 4267)

#endif /* endif _WIN32 */


#ifdef __cplusplus
extern "C" {
#endif

char  * winstrcpy (char * dst, char * src);
int     toHex (int ch, int upercase);

int current_timezone ();

#ifdef __cplusplus
}
#endif

#endif

