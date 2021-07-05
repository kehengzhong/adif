/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#include "btype.h"
#include "memory.h"
#include "arfifo.h"
#include "hashtab.h"

#ifdef UNIX
#include "mthread.h"
#endif


typedef int (PoolUnitInit) (void *);
typedef int (PoolUnitFree) (void *);
typedef int (PoolUnitSize) (void *);


typedef struct buffer_pool {

    uint8    need_free; /*flag indicated the instance allocated by init*/

    /* fixed buffer unit size usually gotton from defined structure */
    int      unitsize;

    /* allocated buffer unit amounts one time */
    int      allocnum;

    /* the members in buffer unit may be allocated more memory
       during application utilization after being fetched out.
       when recycling it back into pool, check if memory of inner-member-allocated
       exceeds this threshold. release the units that get exceeded */
    int      unit_freesize;

    PoolUnitInit * unitinit;
    PoolUnitFree * unitfree;
    PoolUnitSize * getunitsize;

    /* the following 3 members may be modified by multiple threads */
    int      allocated; /*already allocated number*/
    int      exhausted; /*the number which has been pulled out for usage*/
    int      remaining; /*the available number remaining in the pool*/

    time_t   idletick;

    /* the memory units organized via the following linked list */
    CRITICAL_SECTION   ulCS;
    void             * fifo;
    void             * refifo;

    hashtab_t        * rmdup_tab;
    int                hashlen;

} bpool_t;

int bpool_hash_cmp(void * a, void * b)
{
    ulong  ua = (ulong)a;
    ulong  ub = (ulong)b;

    if (ua > ub) return 1;
    if (ua < ub) return -1;
    return 0;
}

ulong bpool_hash (void * key)
{
    ulong ret = (ulong)key;
    return ret;
}

bpool_t * bpool_init (bpool_t * pool)
{
    bpool_t * pmem = NULL;

    if (pool) {
        pmem = pool;
        memset(pmem, 0, sizeof(*pmem));
        pmem->need_free = 0;
    } else {
        pmem = (bpool_t *)kzalloc(sizeof(*pmem));
        pmem->need_free = 1;
    }

    pmem->allocnum = 1;

    pmem->idletick = 0;

    InitializeCriticalSection(&pmem->ulCS);
    pmem->fifo = ar_fifo_new(128);
    pmem->refifo = ar_fifo_new(128);

    pmem->hashlen = 4096;
    pmem->rmdup_tab = ht_only_new(pmem->hashlen, bpool_hash_cmp);
    ht_set_hash_func(pmem->rmdup_tab, bpool_hash);

    return pmem;
}


int bpool_clean (bpool_t * pool)
{
    int     i = 0, num = 0;
    void  * punit = NULL;

    if (!pool) return -1;

    EnterCriticalSection(&pool->ulCS);

    num = ar_fifo_num(pool->refifo);
    for (i=0; i<num; i++) {
        punit = ar_fifo_value(pool->refifo, i);
        if (pool->unitfree)
            (*pool->unitfree)(punit);
        else
            kfree(punit);
    }
    ar_fifo_free(pool->refifo);
    pool->refifo = NULL;

    num = ar_fifo_num(pool->fifo);
    for (i=0; i<num; i++) {
        punit = ar_fifo_value(pool->fifo, i);
        kfree(punit);
    }
    ar_fifo_free(pool->fifo);
    pool->fifo = NULL;

    ht_free(pool->rmdup_tab);
    pool->rmdup_tab = NULL;

    LeaveCriticalSection(&pool->ulCS);

    DeleteCriticalSection(&pool->ulCS);

    if (pool->need_free) kfree(pool);
    return 0;
}

int bpool_fetched_num (bpool_t * pool)
{
    if (!pool) return 0;

    return pool->exhausted;
}

int bpool_set_initfunc (bpool_t * pool, void * init)
{
    if (!pool) return -1;
    pool->unitinit = (PoolUnitFree *)init;
    return 0;
}

int bpool_set_freefunc (bpool_t * pool, void * free)
{
    if (!pool) return -1;
    pool->unitfree = (PoolUnitFree *)free;
    return 0;
}

int bpool_set_getsizefunc (bpool_t * pool, void * vgetsize)
{
    PoolUnitSize  * getsize = (PoolUnitSize *)vgetsize;

    if (!pool) return -1;
    pool->getunitsize = getsize;
    return 0;
}

int bpool_set_unitsize (bpool_t * pool, int size)
{
    if (!pool) return -1;
    pool->unitsize = size;
    return size;
}


int bpool_set_allocnum (bpool_t * pool, int escl)
{
    if (!pool) return -1;

    if (escl <= 0) escl = 1;
    pool->allocnum = escl;
    return escl;
}

int bpool_set_freesize (bpool_t * pool, int size)
{
    if (!pool) return -1;
    pool->unit_freesize = size;
    return size;
}


int bpool_get_state (bpool_t * pool, int * allocated, int * remaining,
                     int * exhausted, int * fifonum, int * refifonum)
{
    if (!pool) return -1;

    if (allocated) *allocated = pool->allocated;
    if (remaining) *remaining = pool->remaining;
    if (exhausted) *exhausted = pool->exhausted;
    if (fifonum) *fifonum = ar_fifo_num(pool->fifo);
    if (refifonum) *refifonum =  ar_fifo_num(pool->refifo);

    return 0;
}


void * bpool_fetch (bpool_t * pool)
{
    void * punit = NULL;
    int    i = 0;

    if (!pool) return NULL;

    EnterCriticalSection(&pool->ulCS);

    /* pool->fifo stores objects pre-allocated but not handed out.
       pool->refifo stores the recycled objects. */
    if (ar_fifo_num(pool->fifo) <= 0 && ar_fifo_num(pool->refifo) <= 0) {
        for (i = 0; i < pool->allocnum; i++) {
            punit = kzalloc(pool->unitsize);
            if (!punit)  continue;

            ar_fifo_push(pool->fifo, punit);
            pool->allocated++;
            pool->remaining++;
        }
    }

    punit = NULL;
    if (ar_fifo_num(pool->fifo) > 0)
        punit = ar_fifo_out(pool->fifo);
    if (!punit) {
        punit = ar_fifo_out(pool->refifo);
    }

    if (punit) {
        pool->exhausted += 1;
        if (--pool->remaining < 0) pool->remaining = 0;

        /* record it in hashtab before handing out.  when recycling,
           the pbuf should be verified based on records in hashtab.
           only those fetched from pool can be recycled, just once!
           the repeated recycling to one pbuf is very dangerous and
           must be prohibited! */
        ht_set(pool->rmdup_tab, punit, punit);
    }

    LeaveCriticalSection(&pool->ulCS);

    if (punit && pool->unitinit)
        (*pool->unitinit)(punit);

    return punit;
}

int bpool_recycle (bpool_t * pool, void * punit)
{
    int   threshold = 0;

    if (!pool || !punit) return -1;

    EnterCriticalSection(&pool->ulCS);

    if (ht_delete(pool->rmdup_tab, punit) != punit) {
        LeaveCriticalSection(&pool->ulCS);
        return -10;
    }

    if (pool->unitfree && pool->unit_freesize > 0 && pool->getunitsize != NULL) {
        if ((*pool->getunitsize)(punit) >= pool->unit_freesize) {
            (*pool->unitfree)(punit);
            pool->allocated--;
            pool->exhausted--;
            LeaveCriticalSection(&pool->ulCS);
            return 0;
        }
    }

    ar_fifo_push(pool->refifo, punit);
    pool->remaining += 1;
    pool->exhausted -= 1;

    /* when the peak time of daily visiting passed, the loads
       of CPU/Memory will go down. the resouces allocated in highest
       load should be released partly and kept in normal level. */

    if (pool->allocnum >= 4) threshold = pool->allocnum >> 1;
    else threshold = pool->allocnum << 1;

    if (pool->allocated > pool->allocnum && pool->exhausted <= threshold) {
        if (pool->idletick == 0) {
            pool->idletick = time(0);

        } else if (time(0) - pool->idletick > 300) {
            while (pool->allocated > pool->allocnum && pool->remaining > threshold) {
                punit = ar_fifo_out(pool->refifo);
                if (punit) {
                    if (pool->unitfree) (*pool->unitfree)(punit);
                    else kfree(punit);
     
                    pool->remaining--;
                    pool->allocated--;
                } else {
                    punit = ar_fifo_out(pool->fifo);
                    if (punit) {
                        kfree(punit);
                        pool->remaining--;
                        pool->allocated--;
                    }
                }
            }
        }
    } else if (pool->idletick > 0) {
        pool->idletick = 0;
    }

    LeaveCriticalSection(&pool->ulCS);
    return 0;
}

