/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#ifndef _WORDLIB_H_
#define _WORDLIB_H_
 
#ifdef __cplusplus
extern "C" {
#endif      


#define WL_ASCII     1
#define WL_GBK       2
#define WL_UNICODE   3
#define WL_UTF8      4


typedef struct wordlib_ {
    ulong       tabsize;
    int         count;
    char        word_file[128];
    char        word_dir[128];
    uint8       charset;
    hashtab_t * subtab;
    void      * itempool;
} WordLib;


void * word_lib_create (char * wdir, ulong tabsize, uint8 charset);
int    word_lib_clean  (void * vwlib);

int    word_lib_loadfile (void * vwlib, char * file);

int    word_lib_getword (void * vwlib, void * pbyte, int bytelen, int * word);

int    word_lib_add (void * vlib, void * pbyte, int bytelen, void * varpara, void * varfree);
int    word_lib_get (void * vlib, void * pbyte, int bytelen, void ** pvar);
int    word_lib_del (void * vlib, void * pbyte, int bytelen);
 
/* segment the byte string based on word library, original byte string: pbyte length: len
 * if found the words in the lib, pres indicates the string pointer of the words,
 *  reslen return the lengthof the words; return value is the position
 * if not found, return value < 0 */
int word_lib_fwmaxmatch (void * vwlib, void * pbyte, int len, void ** pres, int * reslen, void ** pvar);
 
#ifdef __cplusplus
}
#endif
 
#endif

