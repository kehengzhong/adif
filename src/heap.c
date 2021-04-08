/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "heap.h"

#define MIN_NODES    4

typedef void heapfree (void * );


heap_t * heap_dup (heap_t * hp)
{
    heap_t * ret = NULL;

    if ((ret = heap_new(hp->cmp, hp->num_alloc)) == NULL)
         return NULL;

    memcpy((void *)ret->data, (void *)hp->data, sizeof(void *) * hp->num);
    ret->num_alloc = hp->num_alloc;
    ret->num = hp->num;
    ret->cmp = hp->cmp;

    return ret;
}

heap_t * heap_new (heapcmp_t * cmp, int len)
{
    heap_t * ret = NULL;

    if (len < MIN_NODES) len = MIN_NODES;

    if ((ret = (heap_t *) kzalloc(sizeof(heap_t))) == NULL)
        return NULL;

    ret->data = (void **) kzalloc(sizeof(void *) * len);
    if (ret->data == NULL) {
        kfree(ret);
        return NULL;
    }

    ret->num_alloc = len;
    ret->num = 0;
    ret->cmp = cmp;

    return ret;
}

void heap_pop_free (heap_t * hp, void * vfunc)
{
    heapfree * func = (heapfree *)vfunc;
    int        i;

    if (!hp) return;

    for (i = 0; i < hp->num; i++) {
        if (hp->data[i] != NULL)
            (*func)(hp->data[i]);
    }

    heap_free(hp);
}

void heap_pop_kfree (heap_t * hp)
{
    int i;
    
    if (!hp) return;

    for (i = 0; i < hp->num; i++)
        if (hp->data[i] != NULL)
            kfree(hp->data[i]);

    heap_free(hp);
}   


void heap_free (heap_t * hp)
{
    if (!hp) return;

    if (hp->data != NULL)
        kfree(hp->data);

    kfree(hp);
}

void heap_zero(heap_t * hp)
{
    if (!hp) return;
    if (hp->num <= 0) return;

    memset((void *)hp->data, 0, sizeof(hp->data) * hp->num);
    hp->num=0;
}

int heap_num (heap_t * hp)
{
    if (!hp) return 0;

    return hp->num;
}

void * heap_value (heap_t * hp, int i)
{
    if (!hp) return NULL;

    if (hp->num <= 0 || i < 0 || i >= hp->num)
        return NULL;

    return hp->data[i];
}

int heap_enlarge (heap_t * hp, int scale)
{
    void **s;
    int i;

    if (!hp || scale <= 0) return 0;

    if (hp->num_alloc < hp->num + scale) {
        s = (void **)krealloc((void *)hp->data,
                        (unsigned int)sizeof(void *) * (hp->num + scale));
        if (s == NULL)
            return 0;

        hp->data = s;
        hp->num_alloc = hp->num + scale;
    }

    for (i = hp->num; i< hp->num + scale; i++) {
        hp->data[i] = NULL;
    }

    hp->num += scale;

    return hp->num;
}


void heap_up (heap_t * hp, int child)
{
    int    parent = -1;
    void * tmp = NULL;

    if (!hp || child >= hp->num) return;

    while (child > 0) {

        parent = (child - 1) / 2;

        //if (hp->data[ parent ] > hp->data[ child ]) {

        if ((*hp->cmp)(hp->data[ parent ], hp->data[ child ]) > 0) {

            /* if the parent is greater than child, the current child
               becomes parent by swaping them. */

            tmp = hp->data[ parent ];
            hp->data[ parent ] = hp->data[ child ];
            hp->data[ child ] = tmp;

            child = parent;

        } else {
            break;
        }
    }
}

void heap_down (heap_t * hp, int index, int num)
{
    int    parent = index;
    int    child = 2 * parent + 1;
    void * tmp = NULL;

    if (!hp || index >= hp->num) return;

    while (child < num) {

        /* find the smaller child. 
           if left child is greater than right child,
           choose the right child */

        if (child + 1 < num && 
            (*hp->cmp)(hp->data[ child ], hp->data[ child + 1 ]) > 0)
            child++;

        if ((*hp->cmp)(hp->data[ parent ], hp->data[ child ]) > 0) {

            /* if the parent is greater than child, the smaller becomes 
               parent by swaping them. go on shifting the child down */

            tmp = hp->data[ parent ];
            hp->data[ parent ] = hp->data[ child ];
            hp->data[ child ] = tmp;

            parent = child;
            child = 2 * parent + 1;

        } else {
            break;
        }
    }
}

int heap_push (heap_t * hp, void * data)
{
    void ** s;

    if (!hp) return 0;

    if (hp->num_alloc <= hp->num) {
        s = (void **)krealloc((void *)hp->data,
                              sizeof(void *) * hp->num_alloc * 2);
        if (s == NULL) return 0;

        hp->data = s;
        hp->num_alloc *= 2;
    }

    hp->data[hp->num++] = data;
    heap_up(hp, hp->num);

    return hp->num;
}

void * heap_pop (heap_t * hp)
{
    void  * obj = NULL;

    if (!hp || hp->num <= 0) return NULL;

    obj = hp->data[0];
    hp->data[0] = hp->data[ hp->num - 1 ];

    hp->num--;

    heap_down(hp, 0, hp->num);

    return obj;
}


void heap_sort (heap_t * hp)
{
    int    num = 0;
    void * tmp = NULL;

    if (!hp) return;

    num = hp->num;

    while (num > 1) {
        tmp = hp->data[0];
        hp->data[0] = hp->data[num - 1];
        hp->data[num - 1] = tmp;

        num--;

        heap_down(hp, 0, num);
    }
}


int heap_heapify (heap_t * hp)
{
    int  i;

    if (!hp) return -1;

    for (i = (hp->num - 2) / 2; i >= 0; i--) {
        heap_down(hp, i, hp->num);
    }

    return 0;
}

int heap_from (heap_t * hp, void ** ar, int num)
{
    int  i;

    if (!hp || !ar) return -1;

    if (num > hp->num_alloc - hp->num)
        heap_enlarge(hp, num - (hp->num_alloc - hp->num));

    for (i = 0; i < num; i++) {
        hp->data[i] = ar[i];
    }

    hp->num = i;

    return heap_heapify(hp);
}

void * heap_search (heap_t * hp, void * pattern, heapcmp_t * cmp)
{
    void * item = NULL;
    int i;

    if (!hp || !pattern)
        return NULL;

    for (i = 0; i < hp->num; i++) {
        item = hp->data[i];
        if (cmp) {
            if ((*cmp)(item, pattern) == 0) break;
        } else {
            if (item == pattern) break;
        }
    }

    if (i == hp->num)
        item = NULL;

    return item;
}

