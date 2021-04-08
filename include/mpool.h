/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#ifndef _MPOOL_H_
#define _MPOOL_H_ 
    
#ifdef __cplusplus
extern "C" {
#endif 

struct mem_pool_s;
typedef struct mem_pool_s  mpool_t;

mpool_t * mpool_alloc ();
int       mpool_free  (mpool_t * mp);

void * mpool_fetch   (mpool_t * mp);
int    mpool_recycle (mpool_t * mp, void * unit);

int    mpool_set_allocnum (mpool_t * mp, int num);
int    mpool_set_unitsize (mpool_t * mp, int size);
int    mpool_set_freesize (mpool_t * mp, int size);

int    mpool_set_initfunc (mpool_t * mp, void * func);
int    mpool_set_freefunc (mpool_t * mp, void * func);
int    mpool_set_usizefunc (mpool_t * mp, void * func);

int    mpool_allocnum (mpool_t * mp);
int    mpool_unitsize (mpool_t * mp);
int    mpool_freesize (mpool_t * mp);

/******************************************
FIFO example:

void * mp = NULL;
void * val = NULL;

mp = mpool_alloc();
mpool_set_unitsize(mp, sizeof(PackLoc));
mpool_set_allocnum(mp, 100);

val = mpool_fetch(mp);
...
mpool_recycle(mp, val);

mpool_free(mp);

*******************************************/

#ifdef __cplusplus
}
#endif 
 
#endif

