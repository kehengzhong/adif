/*
 * Copyright (c) 2003-2024 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 *
 * #####################################################
 * #                       _oo0oo_                     #
 * #                      o8888888o                    #
 * #                      88" . "88                    #
 * #                      (| -_- |)                    #
 * #                      0\  =  /0                    #
 * #                    ___/`---'\___                  #
 * #                  .' \\|     |// '.                #
 * #                 / \\|||  :  |||// \               #
 * #                / _||||| -:- |||||- \              #
 * #               |   | \\\  -  /// |   |             #
 * #               | \_|  ''\---/''  |_/ |             #
 * #               \  .-\__  '-'  ___/-. /             #
 * #             ___'. .'  /--.--\  `. .'___           #
 * #          ."" '<  `.___\_<|>_/___.'  >' "" .       #
 * #         | | :  `- \`.;`\ _ /`;.`/ -`  : | |       #
 * #         \  \ `_.   \_ __\ /__ _/   .-` /  /       #
 * #     =====`-.____`.___ \_____/___.-`___.-'=====    #
 * #                       `=---='                     #
 * #     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   #
 * #               佛力加持      佛光普照              #
 * #  Buddha's power blessing, Buddha's light shining  #
 * #####################################################
 */

#include "btype.h"
#include "memory.h"
#include "arfifo.h"
#include "rbtree.h"

#ifdef UNIX
#include "mthread.h"
#endif


typedef int (PoolUnitInit) (void *);
typedef int (PoolUnitFree) (void *);
typedef int (PoolUnitSize) (void *);


typedef struct buffer_pool {

    /* flag indicating whether to free the current structure variable when it is released. */
    uint8    need_free;

    /* the size of the memory unit, which is taken from a data structure as the memory pool. */
    int      unitsize;

    /* the number of fixed-size memory units in a single memory block */
    int      allocnum;

    /* After the memory unit bearing a data structure is allocated, its member variables
       may occupy a lot of memory resources. When recycling, this memory unit that consumes
       a lot of memory resources needs special handling.
       This variable is the threshold of memory resource consumption. */
    int      unit_freesize;

    PoolUnitInit * unitinit;
    PoolUnitFree * unitfree;
    PoolUnitSize * getunitsize;

    /* the following 3 members may be modified by multiple threads */
    int      allocated; /*already allocated number*/
    int      consumed;  /*the number which has been pulled out for usage*/
    int      remaining; /*the available number remaining in the pool*/

    time_t   idletick;

    /* the memory units organized via the following fifo queue */
    CRITICAL_SECTION   ulCS;
    arfifo_t         * fifo;
    arfifo_t         * refifo;

    rbtree_t         * rmduptree;

} bpool_t;


int bpool_unit_cmp_key (void * a, void * b)
{
    ulong  ua = (ulong)a;
    ulong  ub = (ulong)b;

    if (ua > ub) return 1;
    if (ua < ub) return -1;
    return 0;
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

    pmem->rmduptree = rbtree_new(bpool_unit_cmp_key, 1);

    return pmem;
}


int bpool_clean (bpool_t * pool)
{
    int     i = 0, num = 0;
    void  * punit = NULL;

    if (!pool) return -1;

    EnterCriticalSection(&pool->ulCS);

    num = ar_fifo_num(pool->refifo);
    for (i = 0; i < num; i++) {
        punit = ar_fifo_value(pool->refifo, i);
        if (pool->unitfree)
            (*pool->unitfree)(punit);
        else
            kfree(punit);
    }
    ar_fifo_free(pool->refifo);
    pool->refifo = NULL;

    num = ar_fifo_num(pool->fifo);
    for (i = 0; i < num; i++) {
        punit = ar_fifo_value(pool->fifo, i);
        kfree(punit);
    }
    ar_fifo_free(pool->fifo);
    pool->fifo = NULL;

    rbtree_free(pool->rmduptree);
    pool->rmduptree = NULL;

    LeaveCriticalSection(&pool->ulCS);

    DeleteCriticalSection(&pool->ulCS);

    if (pool->need_free) kfree(pool);
    return 0;
}

void * bpool_fetch (bpool_t * pool)
{
    void * punit = NULL;
    int    i = 0;

    if (!pool) return NULL;

    EnterCriticalSection(&pool->ulCS);

    /* pool->fifo stores objects pre-allocated but not handed out.
       pool->recytree stores the recycled objects. */
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
        pool->consumed += 1;
        if (--pool->remaining < 0) pool->remaining = 0;

        /* Before allocating a memory unit from the memory pool and handing it
           over, it needs to be saved in the rbtree. When it is recycled, the
           memory unit should be verified according to the records in rbtree.
           Only the memory units taken out of the memory pool can be recycled,
           and only once. It is very dangerous and not allowed to recycle the
           same memory unit into the memory pool repeatedly! */
        rbtree_insert(pool->rmduptree, punit, punit, NULL);
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

    if (rbtree_delete(pool->rmduptree, punit) != punit) {
        LeaveCriticalSection(&pool->ulCS);
        return -10;
    }

    if (pool->unitfree && pool->unit_freesize > 0 && pool->getunitsize != NULL) {
        if ((*pool->getunitsize)(punit) >= pool->unit_freesize) {
            (*pool->unitfree)(punit);
            pool->allocated--;
            pool->consumed--;
            LeaveCriticalSection(&pool->ulCS);
            return 0;
        }
    }

    ar_fifo_push(pool->refifo, punit);
    pool->remaining += 1;
    pool->consumed -= 1;

    /* When the peak of business access passes, the load of CPU/memory
       will decrease. The resources allocated to the highest load should
       be partially released and kept at a normal level. */

    threshold = pool->allocnum << 1;

    if (pool->allocated > pool->allocnum && pool->remaining >= threshold) {
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

int bpool_allocated (bpool_t * mp)
{
    if (!mp) return 0;

    return mp->allocated;
}

int bpool_consumed (bpool_t * mp)
{
    if (!mp) return 0;

    return mp->consumed;
}

int bpool_remaining (bpool_t * mp)
{
    if (!mp) return 0;

    return mp->remaining;
}

int bpool_status (bpool_t * pool, int * allocated, int * remaining,
                  int * consumed, int * fifonum, int * refifonum)
{
    if (!pool) return -1;

    if (allocated) *allocated = pool->allocated;
    if (remaining) *remaining = pool->remaining;
    if (consumed) *consumed = pool->consumed;
    if (fifonum) *fifonum = ar_fifo_num(pool->fifo);
    if (refifonum) *refifonum =  ar_fifo_num(pool->refifo);

    return 0;
}

