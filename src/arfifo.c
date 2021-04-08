/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include "btype.h"
#include "memory.h"
#include "mthread.h"
#include "arfifo.h"

#define MIN_AF_NODES  8
typedef int FreeFunc (void * a);

typedef struct ar_fifo {
    CRITICAL_SECTION   afCS;
     
    int                size;
    int                start;
    int                num; 
     
    void            ** data; 
} arfifo_t;

void * ar_fifo_new (int size)
{
    arfifo_t * af = NULL;

    if (size < MIN_AF_NODES) size = MIN_AF_NODES;

    af = kzalloc(sizeof(*af));
    if (!af) return NULL;

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

void ar_fifo_free (void * vaf)
{
    arfifo_t * af = (arfifo_t *)vaf;

    if (!af) return;

    DeleteCriticalSection(&af->afCS);
    if (af->data) kfree(af->data);

    kfree(af);
}

void ar_fifo_free_all (void * vaf, void * freefunc)
{
    arfifo_t   * af = (arfifo_t *)vaf;
    FreeFunc   * func = (FreeFunc *)freefunc;
    int          i = 0;
    int          index = 0;
 
    if (!af) return;
 
    if (freefunc) {
        EnterCriticalSection(&af->afCS);
        for (i=0; i<af->num; i++) {
            index = (af->start + i) % af->size;
            (*func)(af->data[index]);
        }

        if (af->data) kfree(af->data);
        LeaveCriticalSection(&af->afCS);
    }
 
    DeleteCriticalSection(&af->afCS);
    kfree(af);
}
 
void ar_fifo_zero (void * vaf)
{
    arfifo_t * af = (arfifo_t *)vaf;
    int        i = 0;

    if (!af) return;

    af->num = 0;
    af->start = 0;
 
    EnterCriticalSection(&af->afCS);
    for (i=0; i<af->size; i++) {
        af->data[i] = NULL;
    }
    LeaveCriticalSection(&af->afCS);
}

 
int ar_fifo_num (void * vaf)
{
    arfifo_t * af = (arfifo_t *)vaf;
 
    if (!af) return 0;
 
    return af->num;
}

void * ar_fifo_value (void * vaf, int i)
{
    arfifo_t * af = (arfifo_t *)vaf;
    int        index = 0;
 
    if (!af) return NULL;
 
    if (i < 0 || i >= af->num) return NULL;

    index = (i + af->start) % af->size;
    return af->data[index];
}

int ar_fifo_push (void * vaf, void * value)
{
    arfifo_t * af = (arfifo_t *)vaf;
    int        i, index = 0, wrapnum = 0;
    void    ** tmp = NULL;
    int        oldsize = 0;

    if (!af) return -1;

    EnterCriticalSection(&af->afCS);

    if (af->size < af->num + 1) {
        oldsize = af->size;
        tmp = (void **)krealloc((void *)af->data, sizeof(void *) * af->size * 2);
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
                for (i=0; i<oldsize-af->start; i++) {
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

void * ar_fifo_out (void * vaf)
{
    arfifo_t * af = (arfifo_t *)vaf;
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

void * ar_fifo_front (void * vaf)
{
    arfifo_t * af = (arfifo_t *)vaf;
 
    if (!af || !af->data) return NULL;
    if (af->num < 1) return NULL;

    return ar_fifo_value(af, 0);
}

void * ar_fifo_back (void * vaf)
{
    arfifo_t * af = (arfifo_t *)vaf;
 
    if (!af || !af->data) return NULL;
    if (af->num < 1) return NULL;

    return ar_fifo_value(af, af->num - 1);
}

