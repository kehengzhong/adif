/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#ifndef _PATTERN_MATCH_H_
#define _PATTERN_MATCH_H_
    
#include "bitarr.h"

#ifdef __cplusplus
extern "C" {
#endif 

/* Shift-and algorithm for exact single pattern matching */

typedef struct pattern_bitvector_s {
 
    /* 0 - binary char, veclen = 256
       1 - ascii char, veclen = 127 
       2 - ascii char and case incensitive, veclen = 127 */
    uint8        vectype;
 
    bitarr_t   * bitvec[256];
    int          veclen;
 
    uint8      * pattern;
    int          patlen;
 
} pat_bitvec_t;


pat_bitvec_t * pat_bitvec_alloc (void * vpat, int len, int vectype);
void           pat_bitvec_free  (pat_bitvec_t * patvec);

void * shift_and_find (void * pbyte, int len, void * pattern, int patlen, void * pvec);
void * shift_and_find_string (void * pbyte, int len, void * pattern, int patlen, void * pvec);


/* KMP algorithm for exact single pattern matching */

typedef struct pattern_kmpvector_s {

    /* 0 - binary char, veclen = 256
       1 - ascii char, veclen = 127 
       2 - ascii char and case incensitive, veclen = 127 */
    unsigned     vectype:3;
    unsigned     reverse:1;

    unsigned     patlen:28;
    uint8      * pattern;
 
    int        * failvec;

} pat_kmpvec_t;

pat_kmpvec_t * pat_kmpvec_alloc (void * vpat, int len, int vectype, int reverse);
void           pat_kmpvec_free  (pat_kmpvec_t * patvec);

void * kmp_find_bytes  (void * pbyte, int len, void * pattern, int patlen, void * pvec);
void * kmp_rfind_bytes (void * pbyte, int len, void * pattern, int patlen, void * pvec);

void * kmp_find_string  (void * pbyte, int len, void * pattern, int patlen, void * pvec);
void * kmp_rfind_string (void * pbyte, int len, void * pattern, int patlen, void * pvec);


/* Boyer-More algorithm for exact single pattern matching */
 
typedef struct pattern_bmvector_s {
 
    /* 0 - binary char, veclen = 256
       1 - ascii char, veclen = 127
       2 - ascii char and case incensitive, veclen = 127 */
    unsigned     vectype:3;
    unsigned     reverse:1;
 
    unsigned     patlen:28;
    uint8      * pattern;
 
    int          badch[256];
    int        * goodsuff;
 
} pat_bmvec_t;
 
pat_bmvec_t * pat_bmvec_alloc (void * vpat, int len, int vectype);
void          pat_bmvec_free  (pat_bmvec_t * patvec);
 
void * bm_find_bytes   (void * pbyte, int len, void * pattern, int patlen, void * pvec);
void * bm_find_string  (void * pbyte, int len, void * pattern, int patlen, void * pvec);

int64  bm_find_file    (char * fname, int64 offset, void * pattern, int patlen,
                        int nocase, void * pvec, void * poslist);

int64  bm_find_filebuf (void * fbf, int64 offset, void * pattern, int patlen,
                        int nocase, void * pvec, void * poslist);


/* Sunday algorithm for exact single pattern matching */
 
typedef struct pattern_sunvector_s {

    /* 0 - binary char, veclen = 256
       1 - ascii char, veclen = 127
       2 - ascii char and case incensitive, veclen = 127 */
    unsigned     vectype:3;
    unsigned     reverse:1;
 
    unsigned     patlen:28;
    uint8      * pattern;

    int         chloc[256];

} pat_sunvec_t;

pat_sunvec_t * pat_sunvec_alloc (void * vpat, int len, int vectype);
void           pat_sunvec_free  (pat_sunvec_t * patvec);
 
void * sun_find_bytes   (void * pbyte, int len, void * pattern, int patlen, void * pvec);
void * sun_find_string  (void * pbyte, int len, void * pattern, int patlen, void * pvec);

void * sun_find_mbytes (void ** ppbyte, int * plen, int num, void * pattern, int patlen, void * pvec, int * pind);
void * sun_find_mstring (void ** ppbyte, int * plen, int num, void * pattern, int patlen, void * pvec, int * pind);

#ifdef __cplusplus
}
#endif 
 
#endif

