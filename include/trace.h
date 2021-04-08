/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#ifndef _TRACE_H_
#define _TRACE_H_

#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void * g_trace_log;

void * trlog_init (char * logfile, int calcline);
void   trlog_clean (void * vlog);

int trlog_line (void * vlog);
void trlog_rollover (void * vlog, int line);

void trlogfile (void * vlog, int rectime, char * file, int line, char * fmt, ...);
#define trlog(hlog, rectime, fmt, ...) trlogfile(hlog, rectime, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define tolog(rectime, fmt, ...) trlogfile(g_trace_log, rectime, NULL, __LINE__, fmt, ##__VA_ARGS__)

void printOctet(FILE * fp, void * data, int start, int count, int margin);

#ifdef __cplusplus
}
#endif

#endif

