/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#ifndef _BUFPOOL_H_
#define _BUFPOOL_H_

#ifdef __cplusplus
extern "C" {
#endif


typedef int  (PoolUnitInit) (void *);
typedef void (PoolUnitFree) (void *);
typedef int  (PoolUnitSize) (void *);

struct buffer_pool;
typedef struct buffer_pool bpool_t;

bpool_t * bpool_init  (bpool_t * pool);
int       bpool_clean (bpool_t * pool);

int       bpool_fetched_num (bpool_t * pool);

int       bpool_set_initfunc    (bpool_t * pool, void * init);
int       bpool_set_freefunc    (bpool_t * pool, void * free);
int       bpool_set_getsizefunc (bpool_t * pool, void * getsize);

int       bpool_set_unitsize  (bpool_t * pool, int size);
int       bpool_set_allocnum  (bpool_t * pool, int escl);
int       bpool_set_freesize  (bpool_t * pool, int size);

void    * bpool_fetch   (bpool_t * pool);
int       bpool_recycle (bpool_t * pool, void * unit);

int       bpool_get_state (bpool_t * pool, int * allocated, int * remaining,
                           int * exhausted, int * fifonum, int * refifonum);

#ifdef __cplusplus
}
#endif

#endif

