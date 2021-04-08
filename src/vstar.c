/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include "btype.h"
#include "memory.h"
#include "dynarr.h"
#include "vstar.h"

#define VAR_MIN_NODES    2
#define CmpFP (int (*)(const void *,const void *))

void * vstar_new (int unitsize, int allocnum, void * free)
{
    vstar_t * ar = NULL;
    int       ressize = 0;

    if (unitsize <= 0) return NULL;

    ressize = allocnum;
    if (ressize < VAR_MIN_NODES) ressize = VAR_MIN_NODES;

    ar = kzalloc(sizeof(*ar));
    if (!ar) return NULL;

    ar->unitsize = unitsize;
    ar->sorted = 0;
    ar->num = 0;
    ar->clean = (VarUnitFree *)free;

    ar->data = (void *)kzalloc(unitsize * ressize);
    if (ar->data == NULL) {
        vstar_free(ar);
        return NULL;
    }
    ar->num_alloc = ressize;

    return ar;
}

int vstar_free (vstar_t * ar)
{
    int       i = 0;

    if (!ar) return -1;

    if (ar->clean) {
        for (i = 0; i < ar->num; i++)
            (*ar->clean)(ar->data + i * ar->unitsize);
    }

    if (ar->data) {
        kfree(ar->data);
        ar->data = NULL;
    }

    kfree(ar);
    return 0;
}
 
int vstar_size (vstar_t * ar)
{
    if (!ar) return 0;

    return ar->num_alloc;
}

int vstar_num (vstar_t * ar)
{
    if (!ar) return 0;

    return ar->num;
}
 
void * vstar_ptr (vstar_t * ar)
{
    if (!ar) return NULL;

    return (void *)ar->data;
}

int vstar_len (vstar_t * ar)
{
    if (!ar) return 0;

    return ar->num * ar->unitsize;
}


void * vstar_get (vstar_t * ar, int where)
{
    if (!ar || ar->num <= 0) return NULL;

    if (where < 0 || where >= ar->num) return NULL;

    return ar->data + where * ar->unitsize;
}

void * vstar_last (vstar_t * ar)
{
    if (!ar || ar->num <= 0) return NULL;
 
    return ar->data + (ar->num - 1) * ar->unitsize;
}

int vstar_set (vstar_t * ar, int where, void * value)
{
    if (!ar || ar->num <= 0) return -1;

    if (where < 0 || where >= ar->num) return -2;

    memcpy(ar->data + where * ar->unitsize, value, ar->unitsize);

    return 0;
}

int vstar_enlarge (vstar_t * ar, int scale)
{
    void    * s = NULL;
    int       rest = 0;

    if (!ar || scale <= 0) return -1;

    rest = ar->num_alloc - ar->num;

    if (rest < scale) {
        s = (void *)krealloc((void *)ar->data, 
                              ar->unitsize * (ar->num_alloc + scale));
        if (s == NULL) return 0;
        ar->data = s;
        ar->num_alloc = ar->num_alloc + scale;
    }

    memset(ar->data + ar->num * ar->unitsize, 0, ar->unitsize * scale);
    ar->num += scale;
    ar->sorted = 0;

    return ar->num;
}
 
int vstar_zero (vstar_t * ar)
{
    if (!ar) return -1;

    if (ar->num <= 0) return 0;

    memset((void *)ar->data, 0, ar->unitsize * ar->num_alloc);
    ar->num = 0;

    return 0;
}

int vstar_insert (vstar_t * ar, void * data, int loc)
{
    void    * s = NULL;
    int       newnum = 0;

    if (!ar) return -1;

    if (ar->num_alloc <= ar->num + 1) {
        newnum = ar->num_alloc;

        s = (void *)krealloc((void *)ar->data,
                              ar->unitsize * (ar->num_alloc + newnum));
        if (s == NULL) return -1;

        ar->data = s;
        ar->num_alloc += newnum;
    }

    if (loc >= ar->num || loc < 0)
        memcpy(ar->data + ar->num * ar->unitsize, data, ar->unitsize);
    else {
        if (ar->num - loc > 0) {
            memmove(ar->data + (loc + 1) * ar->unitsize,
                    ar->data + loc * ar->unitsize,
                    (ar->num - loc) * ar->unitsize);
        }
 
        memcpy(ar->data + loc * ar->unitsize, data, ar->unitsize);
    }
    ar->num++;
    ar->sorted=0;

    return loc;
}

int vstar_delete (vstar_t * ar, int loc)
{
    if (!ar) return -1;

    if (loc < 0) loc = ar->num - 1;

    if (ar == NULL || ar->num <= 0 || loc < 0 || loc >= ar->num)
         return -2;
 
    if (ar->clean) (*ar->clean)(ar->data+loc*ar->unitsize);
   
    if (loc != ar->num-1) {
        memmove(ar->data + loc * ar->unitsize,
                ar->data + (loc + 1) * ar->unitsize,
                (ar->num - 1 - loc) * ar->unitsize);
    }
    ar->num--;

    return loc;
}

int vstar_push (vstar_t * ar, void * data)
{
    return vstar_insert(ar, data, -1);
}

int vstar_pop (vstar_t * ar)
{
    return vstar_delete(ar, -1);
}


void vstar_sort_by (vstar_t * ar, int (*cmp)(void *, void *))
{
    if (!ar) return;
    if (ar->num < 2) return;

    qsort(ar->data, ar->num, ar->unitsize, CmpFP cmp);
}

int vstar_insert_by (vstar_t * ar, void * item, int (*cmp)(void *, void *))
{
    int       lo, mid, hi;
    int       cur = 0, end = 0;
    int       result;
 
    if (!ar || !item || !cmp) return -1;
 
    if (ar->num == 0) return vstar_push(ar, item);
 
    lo = 0;
    hi = ar->num-1;
 
    while (lo <= hi) {

        mid = (lo + hi) / 2;

        if (!(result = (*cmp)(vstar_get(ar, mid), item))) {

            for (cur = mid+1; cur <= hi; cur++) {
                if ((*cmp)(vstar_get(ar, cur), item) != 0) {
                    end = cur-1;
                    break;
                }
            }

            if (cur > hi) end = hi;
            return vstar_insert(ar, item, end + 1);

        } else if (result < 0) {
            lo = mid + 1;

        } else {
            hi = mid -1;
        }
    }
 
    return vstar_insert(ar, item, lo);
}
 
int vstar_findloc_by (vstar_t * ar, void * pattern, int (*cmp)(void *, void *), int * found)
{
    int       lo, hi, mid;
    int       result;
 
    if (found) *found = 0;

    if (!ar || !pattern || !cmp) return 0;
 
    lo = 0;
    hi = ar->num - 1;
 
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        if (!(result = (*cmp)(vstar_get(ar, mid), pattern))) {
            if (found) *found = 1;
            return mid;
        }
        else if (result < 0) {
            lo = mid + 1;
            if (lo > hi) return lo;
        } else {
            hi = mid -1;
            if (hi < lo) return mid;
        }
    }
 
    return lo;
}

void * vstar_find_by (vstar_t * ar, void * pattern, int (*cmp)(void *, void *))
{
    int       lo, hi, mid;
    int       result;
 
    if (!ar || !pattern || !cmp) return NULL; 
 
    lo = 0;
    hi = ar->num - 1;
 
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        if (!(result = (*cmp)(vstar_get(ar, mid), pattern))) {
            return vstar_get(ar, mid);
        }
        else if (result < 0) {
            lo = mid + 1;
        } else {
            hi = mid -1;
        }
    }
 
    return NULL;
}

arr_t * vstar_find_all_by (vstar_t * ar, void * pattern, int (*cmp)(void *, void *))
{
    int       lo, hi, mid=0, cur=0, bgn=0, end=0;
    int       result; 
    arr_t   * ret = NULL;
 
    if (!ar || !pattern || !cmp) return NULL;
     
    lo = 0;
    hi = ar->num - 1;
 
    while (lo <= hi) {
        mid = (lo + hi) / 2;

        if (!(result = (*cmp)(vstar_get(ar, mid), pattern))) {
            ret = arr_new (4);
            if (!ret) return NULL;

            for (cur = mid+1; cur <= hi; cur++) {
                if ((*cmp)(vstar_get(ar, cur), pattern) != 0) {
                    end = cur-1;
                    break;
                }
            }

            if (cur > hi) end = hi;

            for (cur = mid-1; cur >= lo; cur--) {
                if ((*cmp)(vstar_get(ar, cur), pattern) != 0) {
                    bgn = cur+1;
                    break;
                }
            }

            if (cur < lo) bgn = lo;
 
            for (cur=bgn; cur<=end; cur++)
                arr_push(ret, vstar_get(ar, cur));
 
            return ret;

        } else if (result < 0) {
            lo = mid + 1;

        } else {
            hi = mid -1;
        }
    }
 
    return NULL;
}

int vstar_delete_by (vstar_t * ar, void * pattern, int (*cmp)(void *, void *))
{
    int       lo = 0, hi = 0, mid = 0; 
    int       result = 0;

    if (!ar || !pattern || !cmp) return -1; 

    lo = 0;
    hi = ar->num - 1;
 
    while (lo <= hi) {
        mid = (lo + hi) / 2;

        if (!(result = (*cmp)(vstar_get(ar, mid), pattern))) {
            return vstar_delete(ar, mid);

        } else if (result < 0) {
            lo = mid + 1;

        } else {
            hi = mid -1;
        }
    }
 
    return -100;
}

int vstar_delete_all_by (vstar_t * ar, void * pattern, int (*cmp)(void *, void *))
{
    int       lo, hi, mid = 0, cur = 0, bgn = 0, end = 0;
    int       result; 

    if (!ar || !pattern || !cmp) return -1;
 
    lo = 0;
    hi = ar->num - 1;
 
    while (lo <= hi) {
        mid = (lo + hi) / 2;

        if (!(result = (*cmp)(vstar_get(ar, mid), pattern))) {

            for (cur = mid+1; cur <= hi; cur++) {
                if ((*cmp)(vstar_get(ar, cur), pattern) != 0) {
                    end = cur-1;
                    break;
                }
            }

            if (cur > hi) end = hi;

            for (cur = mid-1; cur >= lo; cur--) {
                if ((*cmp)(vstar_get(ar, cur), pattern) != 0) {
                    bgn = cur+1;
                    break;
                }
            }

            if (cur < lo) bgn = lo;
 
            for (cur=bgn; cur<=end; cur++) {
                vstar_delete(ar, bgn);
            }

            return 0;

        } else if (result < 0) {
            lo = mid + 1;

        } else {
            hi = mid -1;
        }
    }
 
    return -100;
}
 
void * vstar_search (vstar_t * ar, void * pattern, int (*cmp)(void *, void *))
{
    void    * item = NULL;
    int       i;  

    if (!ar || !pattern || cmp)
        return NULL; 
         
    for (i = 0; i < ar->num; i++) {
        item = vstar_get(ar, i);
        if ((*cmp)(item, pattern) == 0)
            break;
    }
 
    if (i == ar->num)
        item = NULL;
 
    return item;
}

arr_t * vstar_search_all (vstar_t * ar, void * pattern, int (*cmp)(void *, void *))
{
    arr_t   * newst = NULL;
    void    * item = NULL;
    int       i; 
 
    if (!ar || !pattern || !cmp)
        return NULL;
 
    newst = arr_new(4);
 
    for (i = 0; i < ar->num; i++) {
        item = vstar_get(ar, i);
        if ((*cmp)(item, pattern) == 0)
            arr_push(newst, item);
    }
 
    if (arr_num (newst) == 0) {
        arr_free (newst);
        newst = NULL;
    }
 
    return newst;
}

