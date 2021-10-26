/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "dynarr.h"

#define MIN_NODES    4

typedef void ArrFree (void * );

#define FP_ICC (int (*)(const void *, const void *))


arr_t * arr_dup (arr_t * ar)
{
    arr_t * ret = NULL;

    if ((ret = arr_new(ar->num_alloc)) == NULL)
         return NULL;

    memcpy((void *)ret->data, (void *)ar->data, sizeof(void *) * ar->num);
    ret->num_alloc = ar->num_alloc;
    ret->num = ar->num;

    return ret;
}

arr_t * arr_new (int len)
{
    arr_t * ret = NULL;

    if (len < MIN_NODES) len = MIN_NODES;

    if ((ret = (arr_t *) kzalloc(sizeof(arr_t))) == NULL)
        return NULL;

    ret->data = (void **) kzalloc(sizeof(void *) * len);
    if (ret->data == NULL) {
        kfree(ret);
        return NULL;
    }

    ret->num_alloc = len;
    ret->num = 0;
    return ret;
}


int arr_insert (arr_t * ar, void * data, int loc)
{
    void ** s;

    if (!ar) return 0;

    if (ar->num_alloc <= ar->num + 1) {
        s = (void **)krealloc((void *)ar->data,
                 (unsigned int)sizeof(void *) * ar->num_alloc * 2);
        if (s == NULL) return 0;
        ar->data = s;
        ar->num_alloc *= 2;
    }

    if ((loc >= ar->num) || (loc < 0))
        ar->data[ar->num] = data;

    else {
        memmove(&ar->data[loc+1], &ar->data[loc], (ar->num - loc) * sizeof(void *));
        ar->data[loc] = data;
    }

    ar->num++;

    return ar->num;
}


void * arr_delete_ptr (arr_t * ar, void * p)
{
    int i;	

    if (!ar || !p) return NULL;
	
    for (i = 0; i < ar->num; i++)
        if (ar->data[i] == p)
            return arr_delete(ar,i);

    return NULL;
}


void * arr_delete (arr_t * ar, int loc)
{
    void * ret = NULL;

    if (!ar || ar->num == 0 || loc < 0 || loc >= ar->num)
         return NULL;

    ret = ar->data[loc];
    if (loc != ar->num-1) {
        memmove(&ar->data[loc], &ar->data[loc+1], (ar->num-loc-1)*sizeof(void *));
    }

    ar->num--;
    return ret;
}

int arr_push (arr_t * ar, void * data)
{
    return arr_insert(ar, data, ar->num);
}

void * arr_pop (arr_t * ar)
{
    if (!ar) return NULL;

    if (ar->num <= 0) return NULL;

    return arr_delete(ar, ar->num-1);
}

void arr_zero(arr_t * ar)
{
    if (!ar) return;
    if (ar->num <= 0) return;

    memset((void *)ar->data, 0, sizeof(ar->data) * ar->num);
    ar->num=0;
}

void arr_pop_free (arr_t * ar, void * vfunc)
{
    ArrFree * func = (ArrFree *)vfunc;
    int       i;

    if (!ar) return;

    for (i = 0; i < ar->num; i++) {
        if (ar->data[i] != NULL)
            (*func)(ar->data[i]);
    }

    arr_free(ar);
}

void arr_pop_kfree (arr_t * ar)
{
    int i;
    
    if (!ar) return;

    for (i = 0; i < ar->num; i++)
        if (ar->data[i] != NULL)
            kfree(ar->data[i]);

    arr_free(ar);
}   

void arr_free (arr_t * ar)
{
    if (!ar) return;

    if (ar->data != NULL) kfree(ar->data);

    kfree(ar);
}


int arr_num (arr_t * ar)
{
    if (!ar) return 0;

    return ar->num;
}

void * arr_last (arr_t * ar)
{
    if (!ar) return NULL;
 
    if (ar->num <= 0)
        return NULL;
 
    return ar->data[ar->num - 1];
}

void * arr_value (arr_t * ar, int i)
{
    if (!ar) return NULL;

    if (ar->num <= 0 || i < 0 || i >= ar->num)
        return NULL;

    return ar->data[i];
}

void * arr_get (arr_t * ar, int i)
{
    if (!ar) return NULL;

    if (ar->num <= 0 || i < 0 || i >= ar->num)
        return NULL;

    return ar->data[i];
}

void * arr_set (arr_t * ar, int i, void * value)
{
    if (!ar) return NULL;

    if (ar->num <= 0 || i < 0 || i >= ar->num)
        return NULL;

    return ar->data[i] = value;
}


int arr_enlarge (arr_t * ar, int scale)
{
    void **s;
    int i;

    if (!ar || scale <= 0) return 0;

    if (ar->num_alloc <= ar->num + scale) {
        s = (void **)krealloc((void *)ar->data,
                        sizeof(void *) * (ar->num + scale + MIN_NODES));
        if (s == NULL) return 0;

        ar->data = s;
        ar->num_alloc = ar->num + scale + MIN_NODES;
    }

    for (i = ar->num; i< ar->num + scale; i++) {
        ar->data[i] = NULL;
    }

    ar->num += scale;

    return ar->num;
}


void arr_sort_by (arr_t * ar, ArrCmp * cmp)
{
    if (!ar) return;

    qsort(ar->data, ar->num, sizeof(void *), FP_ICC cmp);
}

int arr_insert_by (arr_t * ar, void * item, ArrCmp * cmp)
{
    int lo, mid, hi;
    int cur=0, end=0;
    int result;

    if (!ar || !item || !cmp)
        return -1;

    if (ar->num == 0)
        return arr_push(ar, item);

    lo = 0;
    hi = ar->num-1;

    while (lo <= hi) {
        mid = (lo + hi) / 2;
        if (!(result = (*cmp)(ar->data[mid], item))) {
            for (cur = mid+1; cur <= hi; cur++) {
                if ((*cmp)(ar->data[cur], item) != 0) {
                    end = cur-1;
                    break;
                }
            }

            if (cur > hi) end = hi;

            return arr_insert(ar, item, end + 1);

        } else if (result < 0) {
            lo = mid + 1;

        } else {
            hi = mid -1;
        }
    }

    return arr_insert(ar, item, lo);
}

int arr_findloc_by (arr_t * ar, void * pattern, ArrCmp * cmp, int * found)
{
    int lo, hi, mid = 0;
    int result;

    if (found) *found = 0;

    if (!ar || !pattern || !cmp)
        return 0;

    lo = 0;
    hi = ar->num - 1;

    while (lo <= hi) {
        mid = (lo + hi) / 2;

        if (!(result = (*cmp)(ar->data[mid], pattern))) {
            if (found) *found = 1;
            return mid;

        } else if (result < 0) {
            lo = mid + 1;
            if (lo > hi) return lo;

        } else {
            hi = mid -1;
            if (hi < lo) return mid;
        }
    }

    return lo;
}


void * arr_find_by (arr_t * ar, void * pattern, ArrCmp * cmp)
{
    int lo, hi, mid;
    int result;

    if (!ar || !pattern)
        return NULL;

    lo = 0;
    hi = ar->num - 1;

    while (lo <= hi) {
        mid = (lo + hi) / 2;

        if (!(result = (*cmp)(ar->data[mid], pattern))) {
            return ar->data[mid];

        } else if (result < 0) {
            lo = mid + 1;

        } else {
            hi = mid -1;
        }
    }

    return NULL;
}


arr_t * arr_find_all_by (arr_t * ar, void * pattern, ArrCmp * cmp)
{
    int lo, hi, mid=0, cur=0, bgn=0, end=0;
    int result;
    arr_t * ret = NULL;

    if (!ar || !pattern)
        return NULL;

    lo = 0;
    hi = ar->num - 1;

    while (lo <= hi) {
        mid = (lo + hi) / 2;

        if (!(result = (*cmp)(ar->data[mid], pattern))) {
            ret = arr_new(4);
            if (!ret) return NULL;

            for (cur = mid+1; cur <= hi; cur++) {
                if ((*cmp)(ar->data[cur], pattern) != 0) {
                    end = cur-1;
                    break;
                }
            }

            if (cur > hi) end = hi;

            for (cur = mid-1; cur >= lo; cur--) {
                if ((*cmp)(ar->data[cur], pattern) != 0) {
                    bgn = cur+1;
                    break;
                }
            }

            if (cur < lo) bgn = lo;

            for (cur=bgn; cur<=end; cur++)
                arr_push(ret, ar->data[cur]);

            return ret;

        } else if (result < 0) {
            lo = mid + 1;

        } else {
            hi = mid -1;
        }
    }

    return NULL;
}

void * arr_delete_by (arr_t * ar, void * pattern, ArrCmp * cmp)
{
    int lo=0, hi=0, mid=0;
    int result=0; 
    
    if (!ar || !pattern)
        return NULL;

    lo = 0;
    hi = ar->num - 1;
    
    while (lo <= hi) {
        mid = (lo + hi) / 2;

        if (!(result = (*cmp)(ar->data[mid], pattern))) {
            return arr_delete(ar, mid);

        } else if (result < 0) {
            lo = mid + 1; 

        } else { 
            hi = mid -1;
        }   
    }   
    
    return NULL;
}   


arr_t * arr_delete_all_by (arr_t * ar, void * pattern, ArrCmp * cmp)
{
    int     lo, hi, mid=0, cur=0, bgn=0, end=0;
    int     result;
    arr_t * ret = NULL;

    if (!ar || !pattern)
        return NULL;

    lo = 0;
    hi = ar->num - 1;

    while (lo <= hi) {
        mid = (lo + hi) / 2;

        if (!(result = (*cmp)(arr_value(ar, mid), pattern))) {
            ret = arr_new(4);
            if (!ret) return NULL;

            for (cur = mid+1; cur <= hi; cur++) {
                if ((*cmp)(arr_value(ar, cur), pattern) != 0) {
                    end = cur-1;
                    break;
                }
            }

            if (cur > hi) end = hi;

            for (cur = mid-1; cur >= lo; cur--) {
                if ((*cmp)(arr_value(ar, cur), pattern) != 0) {
                    bgn = cur+1;
                    break;
                }
            }

            if (cur < lo) bgn = lo;

            for (cur = bgn; cur <= end; cur++) {
                arr_push(ret, arr_delete(ar, bgn));
            }

            return ret;

        } else if (result < 0) {
            lo = mid + 1;

        } else {
            hi = mid -1;
        }
    }

    return NULL;
}

/* array is ordered by hash value of each member when appending.
   The hash value in 32-bit digital space constructs the node loop.
   pattern is a hash value of the search key

   typedef struct NodeObjST {
       uint32    hash;
       void    * hw;
       ......
   } NodeObj;

   int insert_cmp(NodeObj * obj1, NodeObj * obj2) {
      return obj1->hash - obj2->hash;
   }
   int consisten_cmp(NodeObj * obj, uint32 * hash) {
      return obj->hash - *hash;
   }

   NodeObj * obj;
   arr_t  * alist;

   alist = arr_new(4);

   obj = node_obj_new(); node_obj_fill_info(obj);
   obj->hash = node_hash_func(obj->hw, ip, hwid, ...); arr_insert_by(alist, obj, insert_cmp);
   obj = node_obj_new(); node_obj_fill_info(obj);
   obj->hash = node_hash_func(obj->hw, ip, hwid, ...); arr_insert_by(alist, obj, insert_cmp);
   .......
   
   //finding one node object by consistent hash argorithm 
   hash = data_hash_func(data, fid, ...);
   obj = arr_consistent_hash_node(alist, &hash, consistent_cmp);

   store_data_into_node_obj(obj, data);
 */
   
void * arr_consistent_hash_node (arr_t * ar, void * pattern, ArrCmp * cmp)
{
    int lo, hi, mid = 0;
    int result;
 
    if (!ar || ar->num < 1 || !pattern || !cmp)
        return NULL;
 
    if (ar->num == 1)
        return ar->data[0];

    lo = 0;
    hi = ar->num - 1;
 
    while (lo <= hi) {
        mid = (lo + hi) / 2;

        if (!(result = (*cmp)(arr_value(ar, mid), pattern))) {
            return ar->data[mid];
        }

        else if (result < 0) { // Node[mid] < pattern
            lo = mid + 1;
            if (lo > hi) {
                if (lo >= ar->num)
                    return ar->data[0];
                else
                    return ar->data[lo];
            }
        }

        else { // Node[mid] > pattern
            hi = mid -1;
            if (hi < lo)
                return ar->data[mid];
        }
    }
 
    return ar->data[lo];
}


void * arr_search (arr_t * ar, void * pattern, ArrCmp * cmp)
{
    void * item = NULL;
    int i;

    if (!ar || !pattern)
        return NULL;

    for (i = 0; i < ar->num; i++) {
        item = ar->data[i];
        if (cmp) {
            if ((*cmp)(item, pattern) == 0) break;
        } else {
            if (item == pattern) break;
        }
    }

    if (i == ar->num)
        item = NULL;

    return item;
}


arr_t * arr_search_all (arr_t * ar, void *pattern, ArrCmp * cmp)
{
    arr_t * newar;
    void * item = NULL;
    int i;

    if (!ar || !pattern)
        return NULL;

    newar = arr_new(4);

    for (i = 0; i < ar->num; i++) {
        item = arr_value(ar, i);
        if (cmp) {
            if ((*cmp)(item, pattern) == 0)
                arr_push (newar, item);

        } else {
            if (item == pattern)
                arr_push(newar, item);
        }
    }

    if (arr_num (newar) == 0) {
        arr_free (newar);
        newar = NULL;
    }

    return newar;
}

