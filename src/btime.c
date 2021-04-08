/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include "btype.h"
#include "btime.h"

#ifdef _WIN32
int gettimeofday(struct timeval * tv, struct timezone * tz)
{
    union {
        int64    ns100;
        FILETIME ft;
    } now;
     
    GetSystemTimeAsFileTime(&now.ft);
    tv->tv_usec = (long) ((now.ns100 / 10LL) % 1000000LL);
    tv->tv_sec = (long) ((now.ns100 - 116444736000000000LL) / 10000000LL);
    return 0;
 }
#endif

int current_timezone ()
{
    time_t     curt;
    time_t     tick;
    struct tm  st;
 
    curt = time(0);
    st = *gmtime(&curt);
    tick = mktime(&st);
 
    return (curt - tick) / 3600;
}

long btime (btime_t * tp)
{
    struct timeval tval;

    gettimeofday(&tval, NULL);
    if (tp) {
        tp->s = tval.tv_sec;
        tp->ms = tval.tv_usec/1000;
    }
    return tval.tv_sec * 1000000 + tval.tv_usec;
}


void btime_add (btime_t * tp, btime_t tv)
{
    long  ms = 0;

    if (!tp) return;

    ms = tp->ms + tv.ms;
    tp->s += tv.s + ms / 1000;
    tp->ms = ms % 1000;
}

void btime_add_ms (btime_t * tp, long ms)
{        
    if (!tp) return;
 
    ms += tp->ms; 
    tp->s += ms / 1000;
    tp->ms = ms % 1000;
} 
 
void btime_now_add (btime_t * tp, long ms)
{
    btime(tp);
    btime_add_ms(tp, ms);
}


btime_t btime_diff (btime_t * tp0, btime_t * tp1)
{
    btime_t ret = {0};

    if (!tp0 || !tp1) return ret;

    ret.s = tp1->s - tp0->s;
    if (tp1->ms >= tp0->ms){
        ret.ms = tp1->ms - tp0->ms;
    } else {
        ret.ms = 1000 - (tp0->ms - tp1->ms);
        ret.s--;
    }

    return ret;
}

long btime_diff_ms (btime_t * tp0, btime_t * tp1)
{
    long ms = 0;
 
    if (!tp0 || !tp1) return 0;
 
    ms = (tp1->s - tp0->s) * 1000;
    ms += tp1->ms - tp0->ms;
    return ms;
}

