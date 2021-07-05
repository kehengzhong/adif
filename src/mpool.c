/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include "btype.h"
#include "mthread.h"
#include "memory.h"
#include "dynarr.h"
#include "arfifo.h"
#include "hashtab.h"

typedef int (MPUnitInit) (void *);
typedef int (MPUnitFree) (void *);
typedef int (MPUnitSize) (void *);


static int mem_cache_cmp_mem_cache (void * a, void * b)
{
    if (!a) return -1; 
    if (!b) return 1; 
 
    if (a > b) return 1; 
    if (a < b) return -1; 
    return 0;
}

/*
typedef struct mem_cache {
    long              size;
    uint8             pmem[1];
} MemCache;

static int mem_cache_cmp_mem_cache (void * a, void * b)
{
    MemCache * pca = (MemCache *)a;
    MemCache * pcb = (MemCache *)b;

    if (!pca) return -1;
    if (!pcb) return 1;

    if (pca->pmem > pcb->pmem) return 1;
    if (pca->pmem < pcb->pmem) return -1;
    return 0;
}

static int mem_cache_cmp_unit (void * a, void * b)
{    
    MemCache * pca = (MemCache *)a;
    uint8    * unit = (uint8 *)b;
 
    if (!pca) return -1; 
    if (!unit) return 1; 
 
    if (pca->pmem > unit) return 1; 
    if (pca->pmem + pca->size < unit && pca) return -1; 
    return 0;
}*/

typedef struct mem_pool_s {
    CRITICAL_SECTION   mpCS;
    void             * fifo;
    void             * refifo;

    int                unitsize;
    int                allocnum;
    arr_t            * cache_list;
    //when recycle unit, usizefunc returning size is greater than freesize, then free the unit
    int                freesize;

    MPUnitInit       * initfunc;
    MPUnitFree       * freefunc;
    MPUnitSize       * usizefunc;

    hashtab_t        * rmdup_tab;
} mpool_t;


int mpool_hash_cmp(void * a, void * b)
{
    ulong  ua = (ulong)a;
    ulong  ub = (ulong)b;
 
    if (ua > ub) return 1;
    if (ua < ub) return -1;
    return 0;
}
 
ulong mpool_hash (void * key)
{
    ulong ret = (ulong)key;
    return ret;
}


mpool_t * mpool_alloc ()
{
    mpool_t * mp = NULL;

    mp = kzalloc(sizeof(*mp));
    if (!mp) return NULL;

    InitializeCriticalSection(&mp->mpCS);

    mp->fifo = ar_fifo_new(16);
    mp->refifo = ar_fifo_new(16);

    mp->unitsize = 0;
    mp->allocnum = 4;
    mp->cache_list = arr_new(8);
    mp->freesize = 0;

    mp->initfunc = NULL;
    mp->freefunc = NULL;
    mp->usizefunc = NULL;

    mp->rmdup_tab = ht_only_new(4096, mpool_hash_cmp);
    ht_set_hash_func(mp->rmdup_tab, mpool_hash);

    return mp;
}

int mpool_free (mpool_t * mp)
{
    void     * pca = NULL;
    int        i = 0, num = 0;
    void     * unit = NULL;

    if (!mp) return -1;

    EnterCriticalSection(&mp->mpCS);

    if (mp->freefunc) {
        num = ar_fifo_num(mp->refifo);
        for (i = 0; i < num; i++) {
            unit = ar_fifo_value(mp->refifo, i);
            (*mp->freefunc)(unit);
        }
    }

    while (arr_num(mp->cache_list) > 0) {
        pca = arr_pop(mp->cache_list);
        if (!pca) continue;

        kfree(pca);
    }
    arr_free(mp->cache_list);

    ar_fifo_free(mp->fifo);
    ar_fifo_free(mp->refifo);

    ht_free(mp->rmdup_tab);
    mp->rmdup_tab = NULL;

    LeaveCriticalSection(&mp->mpCS);

    DeleteCriticalSection(&mp->mpCS);
    kfree(mp);

    return 0;
}

 
void * mpool_fetch (mpool_t * mp)
{
    void     * pca = NULL; 
    void     * unit = NULL;
    int        i = 0;
    long       size = 0;
 
    if (!mp) return NULL;
    if (mp->unitsize < 1) return NULL;
 
    EnterCriticalSection(&mp->mpCS);

    if (ar_fifo_num(mp->fifo) <= 0 && ar_fifo_num(mp->refifo) <= 0) {

        /* allocate allocnum * unitsize space as mem_cache */

        size = mp->unitsize * mp->allocnum;
        pca = kzalloc(size);
        if (!pca) {
            LeaveCriticalSection(&mp->mpCS);
            return NULL;
        }

        arr_insert_by(mp->cache_list, pca, mem_cache_cmp_mem_cache);

        for (i = 0; i < mp->allocnum; i++) {
            ar_fifo_push(mp->fifo, (uint8 *)pca + i * mp->unitsize);
        }
    }

    if (ar_fifo_num(mp->fifo) > 0)
        unit = ar_fifo_out(mp->fifo); 

    if (!unit)
        unit = ar_fifo_out(mp->refifo);

    if (unit) 
        ht_set(mp->rmdup_tab, unit, unit);

    LeaveCriticalSection(&mp->mpCS);

    if (unit && mp->initfunc) (*mp->initfunc)(unit);

    return unit;
}

int mpool_recycle (mpool_t * mp, void * unit)
{
    if (!mp) return -1;
    if (!unit) return -2;

    EnterCriticalSection(&mp->mpCS);

    if (ht_delete(mp->rmdup_tab, unit) != unit) {
        LeaveCriticalSection(&mp->mpCS);
        return -100;
    }

#if 0
    /* hashtab rmdup_tab assured the memory pointer is allocated by mpool */

    if (arr_find_by(mp->cache_list, unit, mem_cache_cmp_unit) == NULL) {
        LeaveCriticalSection(&mp->mpCS);
        return -100;
    }
#endif

    if (mp->freefunc && mp->usizefunc && mp->freesize > 0) {
        if ((*mp->usizefunc)(unit) >= mp->freesize)
            (*mp->freefunc)(unit);
    }

    ar_fifo_push(mp->refifo, unit); 

    LeaveCriticalSection(&mp->mpCS);

    return 0;
}

 
int mpool_set_allocnum (mpool_t * mp, int num)
{
    if (!mp) return -1;

    mp->allocnum = num;
    return 0;
}

int mpool_set_unitsize (mpool_t * mp, int size)
{
    if (!mp) return -1;
 
    mp->unitsize = size;
    return 0;
}

int mpool_set_freesize (mpool_t * mp, int size)
{
    if (!mp) return -1;
 
    mp->freesize = size;
    return 0;
}

int mpool_set_initfunc (mpool_t * mp, void * func)
{
    if (!mp) return -1;
 
    mp->initfunc = func;
    return 0;
}

int mpool_set_freefunc (mpool_t * mp, void * func)
{
    if (!mp) return -1;
 
    mp->freefunc = func;
    return 0;
}

int mpool_set_usizefunc (mpool_t * mp, void * func)
{
    if (!mp) return -1;
 
    mp->usizefunc = func;
    return 0;
}

int mpool_allocnum (mpool_t * mp)
{
    if (!mp) return 0;
 
    return mp->allocnum;
}
 
int mpool_unitsize (mpool_t * mp)
{
    if (!mp) return 0;
 
    return mp->unitsize;
}
 
int mpool_freesize (mpool_t * mp)
{
    if (!mp) return 0;
 
    return mp->freesize;
}
 
