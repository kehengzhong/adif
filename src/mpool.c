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
#include "mthread.h"
#include "memory.h"
#include "dynarr.h"
#include "arfifo.h"
#include "hashtab.h"
#include "bitarr.h"
#include "frame.h"
#include "trace.h"

typedef int (MPUnitInit) (void *);
typedef int (MPUnitFree) (void *);
typedef int (MPUnitSize) (void *);

typedef int swap_func_t (void * para, void * busyunit, void * idleunit);

typedef struct mem_cache {
    time_t             stamp;
    long               size;
    int                remaining;

    arfifo_t         * fifo;
    arfifo_t         * refifo;

    bitarr_t         * bitar;

    uint8            * pmem;
    uint8              pbyte[1];
} MemCache;

typedef struct mem_pool_s {

    CRITICAL_SECTION   mpCS;

    /* the following 3 members may be modified by multiple threads */
    int                allocated; /*already allocated number*/
    int                consumed; /*the number which has been pulled out for usage*/
    int                remaining; /*the available number remaining in the pool*/

    int                unitsize;
    int                allocnum;
    //when recycle unit, usizefunc returning size is greater than freesize, then free the unit
    unsigned           freesize: 31;
    unsigned           osalloc : 1;

    time_t             checktime;

    MPUnitInit       * initfunc;
    MPUnitFree       * freefunc;
    MPUnitSize       * usizefunc;

    arr_t            * cache_list;
    arr_t            * sort_cache_list;

    int                fifosize;
    int                bitarsize;
} mpool_t;


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
    if (pca->pmem + pca->size < unit) return -1; 
    return 0;
}


MemCache * mem_cache_alloc (mpool_t * mp)
{
    MemCache * pca = NULL;
    long       size = 0;
    int        i;

    if (!mp) return NULL;

    size = mp->unitsize * mp->allocnum;

    if (mp->osalloc)
        pca = kosmalloc(sizeof(*pca) + size + mp->fifosize * 2 + mp->bitarsize);
    else
        pca = kalloc(sizeof(*pca) + size + mp->fifosize * 2 + mp->bitarsize);
    if (!pca) return NULL;

    pca->stamp = time(0);
    pca->size = size;
    pca->remaining = mp->allocnum;
    pca->pmem = pca->pbyte + mp->fifosize * 2 + mp->bitarsize;

    pca->fifo = ar_fifo_from_fixmem(pca->pbyte, mp->fifosize, mp->allocnum + 1, NULL);
    pca->refifo = ar_fifo_from_fixmem(pca->pbyte + mp->fifosize, mp->fifosize, mp->allocnum + 1, NULL);
    pca->bitar = bitarr_from_fixmem(pca->pbyte + 2 * mp->fifosize, mp->bitarsize, mp->allocnum, NULL);

    for (i = 0; i < mp->allocnum; i++) {
        ar_fifo_push(pca->fifo, pca->pmem + i * mp->unitsize);
        mp->allocated++;
        mp->remaining++;
    }
    bitarr_fill(pca->bitar);

    return pca;
}

void mem_cache_free (mpool_t * mp, MemCache * pca)
{
    void * unit = NULL;
    int    i, num;

    if (!pca) return;

    if (!bitarr_filled(pca->bitar)) {
        for (i = 0; i < mp->allocnum; i++) {
            if (bitarr_get(pca->bitar, i) == 0) {
                tolog(0, "Panic: unfreed %p at %d, remaining=%d fifonum=%d refifonum=%d, when mpool free"
                         " cache %p allocnum=%d unitsize=%d\n",
                      pca->pmem + i * mp->unitsize, i, pca->remaining, ar_fifo_num(pca->fifo),
                      ar_fifo_num(pca->refifo), pca->pmem, mp->allocnum, mp->unitsize);
            }
        }
    }

    if (pca->bitar) {
        bitarr_free(pca->bitar);
        pca->bitar = NULL;
    }

    if (mp && mp->freefunc) {
        num = ar_fifo_num(pca->refifo);
        for (i = 0; i < num; i++) {
            unit = ar_fifo_value(pca->refifo, i);
            (*mp->freefunc)(unit);
        }
    }

    ar_fifo_free(pca->fifo);
    ar_fifo_free(pca->refifo);

    if (mp->osalloc)
        kosfree(pca);
    else
        kfree(pca);
}

int mem_cache_index (mpool_t * mp, MemCache * pca, void * unit)
{
    long  difval = 0;
    long  index = 0;

    if (!mp || !pca || !unit) return -1;

    difval = (uint8 *)unit - pca->pmem;
    if (difval < 0) return -2;
    if (difval % mp->unitsize != 0) return -3;

    index = difval / mp->unitsize;
    if (index >= mp->allocnum) return -4;

    return index;
}


int mem_cache_get_bit (mpool_t * mp, MemCache * pca, void * unit)
{
    int index = 0;

    if (!mp || !pca || !unit) return -1;

    index = mem_cache_index(mp, pca, unit);
    if (index < 0) return index;

    return bitarr_get(pca->bitar, index);
}

int mem_cache_set_bit (mpool_t * mp, MemCache * pca, void * unit, int val)
{
    int index = 0;

    if (!mp || !pca || !unit) return -1;

    index = mem_cache_index(mp, pca, unit);
    if (index < 0) return index;

    if (val != 0)
        return bitarr_set(pca->bitar, index);
    else
        return bitarr_unset(pca->bitar, index);
}

void * mem_cache_fetch (mpool_t * mp, MemCache * pca)
{
    void    * unit = NULL;
    int       index = 0;

    if (!mp) return NULL;

    if (!pca || pca->remaining <= 0) return NULL;

    if (ar_fifo_num(pca->fifo) > 0) {
        unit = ar_fifo_out(pca->fifo); 
        if (unit) memset(unit, 0, mp->unitsize);
    }

    if (!unit)
        unit = ar_fifo_out(pca->refifo);

    if (unit) {
        index = mem_cache_index(mp, pca, unit);
        if (index < 0) return NULL;

        bitarr_unset(pca->bitar, index);

        if (--pca->remaining < 0) pca->remaining = 0;
        pca->stamp = time(0);

        mp->consumed += 1;
        if (--mp->remaining < 0) mp->remaining = 0;
    }

    return unit;
}

int mem_cache_recycle (mpool_t * mp, MemCache * pca, void * unit)
{
    int    index = 0;

    if (!mp) return -1;
    if (!pca) return -2;
    if (!unit) return -3;

    index = mem_cache_index(mp, pca, unit);
    if (index < 0) return -100;

    if (bitarr_get(pca->bitar, index) == 1)
        return -101;

    if (mp->freefunc && mp->usizefunc && mp->freesize > 0) {
        if ((*mp->usizefunc)(unit) >= (int)mp->freesize)
            (*mp->freefunc)(unit);
    }

    ar_fifo_push(pca->refifo, unit); 
    bitarr_set(pca->bitar, index);
    pca->remaining += 1;
    pca->stamp = time(0);

    mp->remaining += 1;
    mp->consumed -= 1;

    return 0;
}


mpool_t * mpool_alloc ()
{
    mpool_t * mp = NULL;

    mp = kzalloc(sizeof(*mp));
    if (!mp) return NULL;

    InitializeCriticalSection(&mp->mpCS);

    mp->allocated = 0;
    mp->consumed = 0;
    mp->remaining = 0;

    mp->unitsize = 0;
    mp->allocnum = 4;
    mp->freesize = 0;
    mp->osalloc = 0;

    mp->checktime = 0;

    mp->initfunc = NULL;
    mp->freefunc = NULL;
    mp->usizefunc = NULL;

    mp->cache_list = arr_new(8);
    mp->sort_cache_list = arr_new(8);

    mp->fifosize = 0;
    mp->bitarsize = 0;

    return mp;
}

mpool_t * mpool_osalloc ()
{
    mpool_t * mp = NULL;

    mp = koszmalloc(sizeof(*mp));
    if (!mp) return NULL;

    InitializeCriticalSection(&mp->mpCS);

    mp->allocated = 0;
    mp->consumed = 0;
    mp->remaining = 0;

    mp->unitsize = 0;
    mp->allocnum = 4;
    mp->freesize = 0;
    mp->osalloc = 1;

    mp->initfunc = NULL;
    mp->freefunc = NULL;
    mp->usizefunc = NULL;

    mp->cache_list = arr_osalloc(8);
    mp->sort_cache_list = arr_osalloc(8);

    mp->fifosize = 0;
    mp->bitarsize = 0;

    return mp;
}

int mpool_free (mpool_t * mp)
{
    void     * pca = NULL;

    if (!mp) return -1;

    EnterCriticalSection(&mp->mpCS);

    while (arr_num(mp->cache_list) > 0) {
        pca = arr_pop(mp->cache_list);
        if (!pca) continue;

        mem_cache_free(mp, pca);
    }
    arr_free(mp->cache_list);
    arr_free(mp->sort_cache_list);

    LeaveCriticalSection(&mp->mpCS);

    DeleteCriticalSection(&mp->mpCS);

    if (mp->osalloc)
        kosfree(mp);
    else
        kfree(mp);

    return 0;
}

int mpool_check (mpool_t * mp)
{
    MemCache * pca = NULL; 
    int        i = 0, num;
    time_t     curt = 0;
 
    if (!mp) return -1;

    curt = time(0);
    if (curt - mp->checktime < 15) {
        return 0;
    }

    EnterCriticalSection(&mp->mpCS);

    num = arr_num(mp->cache_list);
    for (i = num - 1; mp->remaining > mp->allocnum && i >= 0; i--) {
        pca = arr_value(mp->cache_list, i);
        if (!pca) { arr_delete(mp->cache_list, i); i--; num--; continue; }

        if (curt - pca->stamp < 300) continue;

        if (pca->remaining == mp->allocnum || 
            ar_fifo_num(pca->fifo) + ar_fifo_num(pca->refifo) == mp->allocnum ||
            bitarr_filled(pca->bitar))
        {
            arr_delete(mp->cache_list, i);
            arr_delete_ptr(mp->sort_cache_list, pca);
            i--; num--;
 
            mem_cache_free(mp, pca);

            mp->allocated -= mp->allocnum;
            mp->remaining -= mp->allocnum;
        }
    }

    mp->checktime = curt;

    LeaveCriticalSection(&mp->mpCS);

    return 0;
}

int mpool_get_busyunit (mpool_t * mp, void * vswapfunc, void * para)
{
    swap_func_t * swapfunc = (swap_func_t *)vswapfunc;
    MemCache * pca = NULL;
    MemCache * ipca = NULL;
    void     * busyunit = NULL;
    void     * idleunit = NULL;
    int        i = 0, num, j, k;
    int        index = 0;
    time_t     curt = 0;

    if (!mp) return -1;
    if (!swapfunc) return -2;

    curt = time(0);
    if (curt - mp->checktime < 15) {
        return 0;
    }

    EnterCriticalSection(&mp->mpCS);

    num = arr_num(mp->cache_list);
    for (i = num - 1; mp->remaining > mp->allocnum && i >= 0; i--) {
        pca = arr_value(mp->cache_list, i);
        if (!pca) { arr_delete(mp->cache_list, i); i--; num--; continue; }

        if (curt - pca->stamp < 600) continue;
        if (bitarr_filled(pca->bitar)) continue;
        if (pca->remaining * 100 / mp->allocnum < 96) continue;

        for (j = 0; j < mp->allocnum; j++) {
            if (bitarr_get(pca->bitar, j) == 0) {
                busyunit = pca->pmem + i * mp->unitsize;

                /* take out one idle unit from memory cache before current MemCache */
                for (k = 0; k < i; k++) {
                    ipca = arr_value(mp->cache_list, k);
                    if (!ipca) continue;

                    if (ar_fifo_num(ipca->fifo) > 0) {
                        idleunit = ar_fifo_out(ipca->fifo);
                        if (idleunit) memset(idleunit, 0, mp->unitsize);
                    }
                    if (!idleunit) idleunit = ar_fifo_out(ipca->refifo);
                    if (idleunit) {
                        index = mem_cache_index(mp, ipca, idleunit);
                        bitarr_unset(ipca->bitar, index);

                        if (--ipca->remaining < 0) ipca->remaining = 0;
                    }

                    if (!idleunit) {
                        LeaveCriticalSection(&mp->mpCS);
                        return -100;
                    }
                    if (mp->initfunc) (*mp->initfunc)(idleunit);
                    break;
                }
                if (k >= i || !idleunit) {
                    LeaveCriticalSection(&mp->mpCS);
                    return -101;
                }

                if ((*swapfunc)(para, busyunit, idleunit) >= 0) {
                    ar_fifo_push(pca->refifo, busyunit);
                    bitarr_set(pca->bitar, j);
                    pca->remaining += 1;
                } else {
                    ar_fifo_push(ipca->refifo, busyunit);
                    bitarr_set(ipca->bitar, index);
                    ipca->remaining += 1;
                }
            }
        }
    }

    LeaveCriticalSection(&mp->mpCS);

    return 0;
}

void * mpool_fetch (mpool_t * mp)
{
    MemCache * pca = NULL; 
    void     * unit = NULL;
    int        i = 0, num;
 
    if (!mp) return NULL;
    if (mp->unitsize < 1) return NULL;
 
    EnterCriticalSection(&mp->mpCS);

    num = arr_num(mp->cache_list);
    for (i = 0; i < num; i++) {
        pca = arr_value(mp->cache_list, i);
        if (!pca) { arr_delete(mp->cache_list, i); i--; num--; continue; }

        if (ar_fifo_num(pca->fifo) + ar_fifo_num(pca->refifo) > 0) {
            unit = mem_cache_fetch(mp, pca);
            if (unit) {
                LeaveCriticalSection(&mp->mpCS);

                if (mp->initfunc) (*mp->initfunc)(unit);
                return unit;
            }

            tolog(1, "Panic: mpool_fetch fifo>0 but unit==NULL, No.%d/%d rest=%d fifo=%d refifo=%d "
                     "time(0)-stamp=%ld unitsize=%d allocnum=%d alloc=%d used=%d rest=%d\n",
                  i, num, pca->remaining, ar_fifo_num(pca->fifo), ar_fifo_num(pca->refifo),
                  time(0)-pca->stamp, mp->unitsize, mp->allocnum, mp->allocated,
                  mp->consumed, mp->remaining);
        }
    }

    pca = mem_cache_alloc(mp);
    if (!pca) {
        LeaveCriticalSection(&mp->mpCS);
        tolog(1, "Panic: mpool_fetch failed. MPool: unitsize=%d "
                 "allocnum=%d alloc=%d used=%d rest=%d osalloc=%d cachenum=%d\n",
              mp->unitsize, mp->allocnum, mp->allocated, mp->consumed,
              mp->remaining, mp->osalloc, arr_num(mp->cache_list));
        return NULL;
    }

    arr_push(mp->cache_list, pca);
    arr_insert_by(mp->sort_cache_list, pca, mem_cache_cmp_mem_cache);

    unit = mem_cache_fetch(mp, pca);

    LeaveCriticalSection(&mp->mpCS);

    if (unit && mp->initfunc) (*mp->initfunc)(unit);

    return unit;
}

int mpool_recycle (mpool_t * mp, void * unit)
{
    MemCache * pca = NULL;
    int        index = -1;

    if (!mp) return -1;
    if (!unit) return -2;

    EnterCriticalSection(&mp->mpCS);

    pca = arr_find_by(mp->sort_cache_list, unit, mem_cache_cmp_unit);
    if (!pca || (index = mem_cache_index(mp, pca, unit)) < 0 || bitarr_get(pca->bitar, index) == 1) {
        LeaveCriticalSection(&mp->mpCS);
        tolog(1, "Panic: mpool_recycle failed, unit=%p not found in MPool: unitsize=%d "
                 "allocnum=%d alloc=%d used=%d rest=%d osalloc=%d cachenum=%d\n",
              unit, mp->unitsize, mp->allocnum, mp->allocated, mp->consumed,
              mp->remaining, mp->osalloc, arr_num(mp->cache_list));
        return -100;
    }

    ar_fifo_push(pca->refifo, unit);
    bitarr_set(pca->bitar, index);
    pca->remaining += 1;
    pca->stamp = time(0);

    mp->remaining += 1;
    mp->consumed -= 1;

    LeaveCriticalSection(&mp->mpCS);

    if (mp->freefunc && mp->usizefunc && mp->freesize > 0) {
        if ((*mp->usizefunc)(unit) >= (int)mp->freesize)
            (*mp->freefunc)(unit);
    }

    mpool_check(mp);

    return 0;
}

 
int mpool_set_allocnum (mpool_t * mp, int num)
{
    if (!mp) return -1;

    mp->allocnum = num;

    ar_fifo_from_fixmem(NULL, 0, mp->allocnum + 1, &mp->fifosize);
    bitarr_from_fixmem(NULL, 0, mp->allocnum, &mp->bitarsize);

    return 0;
}

int mpool_set_unitsize (mpool_t * mp, int size)
{
    if (!mp) return -1;
 
    mp->unitsize = align_size(size, ADF_ALIGNMENT);
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
 
int mpool_fetched_num (mpool_t * mp)
{
    if (!mp) return 0;

    return mp->consumed;
}

int mpool_allocated (mpool_t * mp)
{
    if (!mp) return 0;

    return mp->allocated;
}

int mpool_consumed (mpool_t * mp)
{
    if (!mp) return 0;

    return mp->consumed;
}

int mpool_remaining (mpool_t * mp)
{
    if (!mp) return 0;

    return mp->remaining;
}


int mpool_status (mpool_t * mp, int * allocated, int * remaining, int * consumed, int * cachenum)
{
    if (!mp) return -1;

    if (allocated) *allocated = mp->allocated;
    if (remaining) *remaining = mp->remaining;
    if (consumed) *consumed = mp->consumed;
    if (cachenum) *cachenum = arr_num(mp->cache_list);

    return 0;
}

int mpool_para (mpool_t * mp, int * allocnum, int * unitsize, int * blksize)
{
    if (!mp) return -1;

    if (allocnum) *allocnum = mp->allocnum;
    if (unitsize) *unitsize = mp->unitsize;
    if (blksize) *blksize = sizeof(MemCache) + mp->allocnum * mp->unitsize
                            + 2*mp->fifosize + mp->bitarsize;

    return 0;
}

long mpool_size (mpool_t * mp)
{
    long size = 0;
    int  num = 0;

    if (!mp) return 0;

    size = sizeof(MemCache) + mp->allocnum * mp->unitsize + 2*mp->fifosize + mp->bitarsize;
    num = arr_num(mp->cache_list);

    return size * num + sizeof(*mp); 
}

void mpool_print (mpool_t * mp, char * title, int margin, void * vfrm, FILE * fp)
{
    frame_t  * frm = (frame_t *)vfrm;
    MemCache * pca = NULL;
    char       mar[32] = {0};
    int        i, num;
    long       size = 0;
    time_t     curt = 0;

    if (!mp) return;

    num = sizeof(mar)/sizeof(char);

    if (margin < 0) margin = 0;
    if (margin >= num) margin = num - 1;

    for (i = 0; i < margin; i++) mar[i] = ' ';
    mar[margin] = '\0';

    curt = time(0);

    size = sizeof(MemCache) + mp->allocnum * mp->unitsize + 2*mp->fifosize + mp->bitarsize;
    num = arr_num(mp->cache_list);

    if (frm)
        frame_appendf(frm, "%s%s: alloc=%d used=%d rest=%d blknum=%d allocnum=%d unitsize=%d"
                           " fifosize=%d bitarsize=%d blksize=%ld\n",
                      mar, title, mp->allocated, mp->consumed, mp->remaining, num,
                      mp->allocnum, mp->unitsize, mp->fifosize, mp->bitarsize, size);

    if (fp)
        fprintf(fp, "%s%s: alloc=%d used=%d rest=%d blknum=%d allocnum=%d unitsize=%d"
                    " fifosize=%d bitarsize=%d blksize=%ld\n",
                mar, title, mp->allocated, mp->consumed, mp->remaining, num,
                mp->allocnum, mp->unitsize, mp->fifosize, mp->bitarsize, size);

    for (i = 0; i < num; i++) {
        pca = arr_value(mp->cache_list, i);
        if (!pca) continue;

        if (frm)
            frame_appendf(frm, "%s    Blk %d/%d: poolfull=%d rest=%d curt-stamp=%ld refifonum=%d fifonum=%d "
                               "poolempty=%d pmem=%p size=%ld\n", 
                          mar, i, num, bitarr_filled(pca->bitar), pca->remaining, curt-pca->stamp,
                          ar_fifo_num(pca->refifo), ar_fifo_num(pca->fifo),
                          bitarr_cleared(pca->bitar), pca->pmem, pca->size);

        if (fp)
            fprintf(fp, "%s    Blk %d/%d: poolfull=%d rest=%d curt-stamp=%ld refifonum=%d fifonum=%d "
                        "poolempty=%d pmem=%p size=%ld\n", 
                    mar, i, num, bitarr_filled(pca->bitar), pca->remaining, curt-pca->stamp,
                    ar_fifo_num(pca->refifo), ar_fifo_num(pca->fifo),
                    bitarr_cleared(pca->bitar), pca->pmem, pca->size);
    }
}

