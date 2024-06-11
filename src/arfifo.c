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
#include "kemalloc.h"
#include "mthread.h"
#include "arfifo.h"

#define MIN_AF_NODES  8
typedef int FreeFunc (void * a);


void ar_fifo_free (arfifo_t * af)
{
    if (!af) return;

    DeleteCriticalSection(&af->afCS);

    if (af->fixmem) return;

    if (af->data) k_mem_free(af->data, af->alloctype, af->mpool);
    k_mem_free(af, af->alloctype, af->mpool);
}

void ar_fifo_free_all (arfifo_t * af, void * freefunc)
{
    FreeFunc   * func = (FreeFunc *)freefunc;
    int          i = 0;
    int          index = 0;
 
    if (!af) return;
 
    if (freefunc) {
        EnterCriticalSection(&af->afCS);
        for (i = 0; i < af->num; i++) {
            index = (af->start + i) % af->size;
            (*func)(af->data[index]);
        }

        if (!af->fixmem && af->data)
            k_mem_free(af->data, af->alloctype, af->mpool);

        LeaveCriticalSection(&af->afCS);
    }
 
    DeleteCriticalSection(&af->afCS);

    if (!af->fixmem)
        k_mem_free(af, af->alloctype, af->mpool);
}
 
arfifo_t * ar_fifo_alloc (int size, int alloctype, void * mpool)
{
    arfifo_t * af = NULL;

    if (size < MIN_AF_NODES) size = MIN_AF_NODES;

    af = k_mem_zalloc(sizeof(*af), alloctype, mpool);
    if (!af) return NULL;

    af->alloctype = alloctype;
    af->mpool = mpool;
    af->fixmem = 0;

    InitializeCriticalSection(&af->afCS);
    af->num = 0;
    af->size = size;
    af->start = 0;

    af->data = (void **)kzalloc(sizeof(void *) * af->size);
    if (!af->data) {
        ar_fifo_free(af);
        return NULL;
    }

    return af;
}

arfifo_t * ar_fifo_from_fixmem (void * pmem, int memlen, int fifosize, int * pmemsize)
{
    arfifo_t * af = NULL;

    if (pmemsize) *pmemsize = sizeof(*af) + fifosize * sizeof(void *);

    if (!pmem || fifosize < 2 || memlen < (int)sizeof(*af) + fifosize * (int)sizeof(void *))
        return NULL;

    af = (arfifo_t *)pmem;

    af->alloctype = 0;
    af->mpool = NULL;
    af->fixmem = 1; //fix membuffer initialized for fifo

    InitializeCriticalSection(&af->afCS);
    af->num = 0;
    af->start = 0;

    af->size = fifosize;
    af->data = (void **)((uint8 *)pmem + sizeof(*af));

    memset(af->data, 0, sizeof(void *) * af->size);

    return af;
}

void ar_fifo_zero (arfifo_t * af)
{
    int        i = 0;

    if (!af) return;

    af->num = 0;
    af->start = 0;
 
    EnterCriticalSection(&af->afCS);
    for (i = 0; i < af->size; i++) {
        af->data[i] = NULL;
    }
    LeaveCriticalSection(&af->afCS);
}

 
int ar_fifo_num (arfifo_t * af)
{
    if (!af) return 0;
 
    return af->num;
}

void * ar_fifo_value (arfifo_t * af, int i)
{
    int        index = 0;
 
    if (!af) return NULL;
 
    if (i < 0 || i >= af->num) return NULL;

    index = (i + af->start) % af->size;
    return af->data[index];
}

 
int ar_fifo_push (arfifo_t * af, void * value)
{
    int        i, index = 0, wrapnum = 0;
    void    ** tmp = NULL;
    int        oldsize = 0;

    if (!af) return -1;

    EnterCriticalSection(&af->afCS);

    if (af->size < af->num + 1) {
        if (af->fixmem) {
            LeaveCriticalSection(&af->afCS);
            return -100;
        }
        oldsize = af->size;

        tmp = (void **)k_mem_realloc((void *)af->data,
                                     sizeof(void *) * af->size * 2,
                                     af->alloctype, af->mpool);
        if (tmp == NULL) {
            LeaveCriticalSection(&af->afCS);
            return -2;
        }
        af->data = tmp;
        af->size *= 2;

        if (af->start + af->num > oldsize) { //wrap around
            wrapnum = (af->start + af->num) % oldsize;
            if (wrapnum <= af->num/2) {
                for (i=0; i<wrapnum; i++) {
                    index = (oldsize + i) % af->size;
                    af->data[index] = af->data[i];
                }
            } else {
                for (i = 0; i < oldsize - af->start; i++) {
                    af->data[af->size-1-i] = af->data[oldsize-1-i];
                }
                af->start = af->size - oldsize + af->start;
            }
        }
    }

    index = (af->start + af->num) % af->size;
    af->data[index] = value;
    af->num++;

    LeaveCriticalSection(&af->afCS);

    return 0;
}

void * ar_fifo_out (arfifo_t * af)
{
    void     * value = NULL;

    if (!af || !af->data) return NULL;
    if (af->num < 1) return NULL;

    EnterCriticalSection(&af->afCS);
    value = af->data[af->start];
    af->start = (af->start + 1) % af->size;
    af->num--;
    LeaveCriticalSection(&af->afCS);

    return value;
}

void * ar_fifo_head (arfifo_t * af)
{
    if (!af || !af->data) return NULL;
    if (af->num < 1) return NULL;

    return ar_fifo_value(af, 0);
}

void * ar_fifo_tail (arfifo_t * af)
{
    if (!af || !af->data) return NULL;
    if (af->num < 1) return NULL;

    return ar_fifo_value(af, af->num - 1);
}

