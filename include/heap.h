/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#ifndef _HEAP_H_
#define _HEAP_H_

#ifdef  __cplusplus
extern "C" {
#endif

typedef int heapcmp_t (void * a, void * b);

typedef struct HeapST_ {
    int          num_alloc;
    int          num;
    heapcmp_t  * cmp;
    void      ** data;
} heap_t, *heap_p;

heap_t * heap_dup (heap_t * hp);
heap_t * heap_new (heapcmp_t * cmp, int len);

void     heap_pop_free  (heap_t * hp, void * vfunc);
void     heap_pop_kfree (heap_t * hp);
void     heap_free      (heap_t * hp);

void     heap_zero    (heap_t * hp);
int      heap_num     (heap_t *);
void   * heap_value   (heap_t *, int);
int      heap_enlarge (heap_t * hp, int scale);

void     heap_up   (heap_t * hp, int child);
void     heap_down (heap_t * hp, int index, int num);

int      heap_push (heap_t * hp, void * data);
void   * heap_pop  (heap_t * hp);

void     heap_sort (heap_t * hp);

int      heap_heapify (heap_t * hp);
int      heap_from    (heap_t * hp, void ** ar, int num);

void   * heap_search  (heap_t * hp, void * pattern, heapcmp_t * cmp);

#ifdef  __cplusplus
}
#endif

#endif

