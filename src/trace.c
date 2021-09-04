/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
 
#include "btype.h"
#include "memory.h"
#include "fileop.h"
#include "frame.h"
 
#ifdef UNIX
#include "mthread.h"
#endif

typedef struct trace_log_ {
    char               logfile[128];
    FILE             * logfp;
    int                logline;
    CRITICAL_SECTION   logCS;
} trlog_t;

void * g_trace_log = NULL;


void * trlog_init (char * logfile, int calcline)
{
    trlog_t * hlog = NULL;

    hlog = kzalloc(sizeof(*hlog));
    if (!hlog) return NULL;

    if (!g_trace_log) {
        g_trace_log = hlog;
    }

    if (!logfile) logfile = "log.txt";

    if (calcline)
        hlog->logline = file_lines(logfile);
    else
        hlog->logline = 0;

    InitializeCriticalSection(&hlog->logCS);

    strncpy(hlog->logfile, logfile, sizeof(hlog->logfile)-1);
    hlog->logfp = fopen(hlog->logfile, "a+");

    return hlog;
}

void trlog_clean (void * vlog)
{
    trlog_t * hlog = (trlog_t *)vlog;

    if (!hlog) hlog = g_trace_log;

    if (!hlog) return;

    if (g_trace_log == hlog)
        g_trace_log = NULL;

    DeleteCriticalSection(&hlog->logCS);

    if (hlog->logfp) {
        fflush(hlog->logfp);
        fclose(hlog->logfp);
    }

    kfree(hlog);
}

int trlog_line (void * vlog)
{
    trlog_t   * hlog = (trlog_t *)vlog;

    if (!hlog) return 0;

    return hlog->logline;
}

void trlog_rollover (void * vlog, int line)
{
    trlog_t   * hlog = (trlog_t *)vlog;
    int         ret;

    EnterCriticalSection(&hlog->logCS);

    if (hlog->logfp) fclose(hlog->logfp);
 
    ret = file_rollover(hlog->logfile, line);
    if (ret >= 0) hlog->logline -= line;
    if (hlog->logline < 0) hlog->logline = 0;
 
    hlog->logfp = fopen(hlog->logfile, "a+");

    LeaveCriticalSection(&hlog->logCS);
}


void trlogfile (void * vlog, int rectime, char * file, int line, char * fmt, ...)
{
    trlog_t   * hlog = (trlog_t *)vlog;
    va_list     args;
    time_t      curt = 0;
    struct tm   st;
 
    if (!hlog) return;
 
    EnterCriticalSection(&hlog->logCS);
 
    if (rectime) {
        time(&curt);
        st = *localtime(&curt);
 
        fprintf(hlog->logfp, "%04d-%02d-%02d %02d:%02d:%02d ",
                st.tm_year+1900, st.tm_mon+1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
        if (file) fprintf(hlog->logfp, "%s:%d ", file, line);
    }
 
    va_start(args, fmt);
    vfprintf(hlog->logfp, fmt, args);
    va_end(args);
 
    hlog->logline++;
    fflush(hlog->logfp);

    LeaveCriticalSection(&hlog->logCS);
}

 
void printOctet (FILE * fp, void * data, int start, int count, int margin)
{
#define CHARS_ON_LINE 16
#define MARGIN_MAX    20
 
    uint8 hexbyte[CHARS_ON_LINE * 5];
    uint8 marginChar [MARGIN_MAX + 1];
    int   lines, i, j, hexInd, ascInd, iter;
    int   ch;
#if defined(_WIN32) || defined(_WIN64)
    char  trline[256];
#endif
 
    if (start < 0) start = 0;
 
    lines = count / CHARS_ON_LINE;
    if (count % CHARS_ON_LINE) lines++;
 
    memset (marginChar, ' ', MARGIN_MAX + 1);
    if (margin < 0) margin = 0;
    if (margin > MARGIN_MAX) margin = MARGIN_MAX;
    marginChar[margin] = '\0';
 
    for (i = 0; i < lines; i++) {
        hexInd = 0;
        ascInd = 4 + CHARS_ON_LINE * 3;
        memset(hexbyte, ' ', CHARS_ON_LINE * 5);
 
        for (j = 0; j < CHARS_ON_LINE; j++) {
            if ( (iter = j + i * CHARS_ON_LINE) >= count)
                break;
            ch = ((uint8 *)data)[iter+start];
 
            hexbyte[hexInd++] = toHex((uint8)((ch>>4)&15), 1);
            hexbyte[hexInd++] = toHex((uint8)(ch&15), 1);
            hexInd++;
 
            hexbyte[ascInd++] = (ch>=(uint8)32 && ch<=(uint8)126) ? ch : '.';
        }
        hexbyte[CHARS_ON_LINE * 4 + 4] = '\n';
        hexbyte[CHARS_ON_LINE * 4 + 5] = '\0';
 
        fprintf(fp, "%s0x%04X   %s", marginChar, i, hexbyte);
#if defined(_WIN32) || defined(_WIN64)
        sprintf(trline, "%s0x%04X   %s", marginChar, i, hexbyte);
        OutputDebugString(trline);
#endif
    }
    fflush(fp);
}
 

