/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#ifndef _WMSEARCH_H_
#define _WMSEARCH_H_

#ifdef __cplusplus
extern "C" {
#endif
 
typedef int PMatchSucc (void * para, int srctype, void * psrc, long srclen, long pos,
                        int * skipnum, uint8 * pat, int patlen,
                        uint8 * pdst, int dstlen, int * repnum);

void * wm_init  (int isascii, int ignorecase);
int    wm_free  (void * vbody);
int    wm_reset (void * vbody);

int    wm_pattern_add     (void * vbody, void * pat, int patlen, void * matchfunc, void * para);
int    wm_pattern_precalc (void * vbody);

long   wm_bytes_search     (void * vbody, void * pbyte, long bytelen, void ** foundobj, int foundexit);
long   wm_filecache_search (void * vbody, void * fca, long pos, void ** foundobj, int foundexit);
long   wm_file_search      (void * vbody, char * file);

long   wm_match_replace (void * vbody, void * fca, long pos, uint8 * pdst, int dstlen, int * cpbytes);
long   wm_bytes_replace (void * vbody, uint8 * pbyte, long bytelen, uint8 * pdst, int dstlen, int * cpbytes);

#ifdef __cplusplus
}
#endif

#endif 

