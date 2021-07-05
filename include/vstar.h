/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#ifndef _VARRIABLE_STRUCTURE_ARRAY_H_
#define _VARRIABLE_STRUCTURE_ARRAY_H_
     
#include "dynarr.h"

#ifdef  __cplusplus 
extern "C" {     
#endif

typedef int (VarUnitFree) (void *);

typedef struct _VStructArray {
    int     unitsize;
    int     sorted;
 
    int     num;
    int     num_alloc;
 
    uint8 * data;
 
    VarUnitFree * clean;
 
} vstar_t, *vstar_p;
 

void * vstar_new  (int unitsize, int allocnum, void * free);
int    vstar_free (vstar_t * var);

int    vstar_size   (vstar_t * var);
int    vstar_num    (vstar_t * var);

void * vstar_ptr    (vstar_t * var);
int    vstar_len    (vstar_t * var);

void * vstar_get    (vstar_t * var, int where);
void * vstar_last   (vstar_t * var);
int    vstar_set    (vstar_t * var, int where, void * value);
int    vstar_enlarge(vstar_t * var, int scale);

int    vstar_zero   (vstar_t * var);
int    vstar_insert (vstar_t * var, void * data, int where);
int    vstar_delete (vstar_t * var, int where);

int    vstar_push   (vstar_t * var, void * data);
int    vstar_pop    (vstar_t * var);
 
/**
 * element_cmp is comparing function for qsort. This is different from arr_t qsort function.
 * int element_cmp (void * a, void * b) {
 *    ObjInst * obj1 = (ObjInst *)a;
 *    ObjInst * obj2 = (ObjInst *)b;
 *    return obj_cmp(obj1, obj2);
 * }
 *
 * pattern_cmp is comparing function for search or find operation based on pattern.
 * int pattern_cmp (void * a, void * pattern) {
 *    ObjInst * obj = (ObjInst *)a;
 *    PatternObj * pat = (PatternObj *)pattern;
 *    return obj_cmp_pattern(obj, pat);
 * }
 */

void    vstar_sort_by       (vstar_t * var, int (*element_cmp)(void *, void *));
int     vstar_insert_by     (vstar_t * var, void * item, int (*pattern_cmp)(void *, void *));

void  * vstar_find_by       (vstar_t * var, void * pattern, int (*pattern_cmp)(void *, void *));
int     vstar_findloc_by    (vstar_t * ar, void * pattern, int (*cmp)(void *, void *), int * found);
arr_t * vstar_find_all_by   (vstar_t * var, void * pattern, int (*pattern_cmp)(void *, void *));
int     vstar_delete_by     (vstar_t * var, void * pattern, int (*pattern_cmp)(void *, void *));
int     vstar_delete_all_by (vstar_t * var, void * pattern, int (*pattern_cmp)(void *, void *));

void  * vstar_search        (vstar_t * var, void * pattern, int (*pattern_cmp)(void *, void *));
arr_t * vstar_search_all    (vstar_t * var, void * pattern, int (*pattern_cmp)(void *, void *));

#ifdef  __cplusplus
}
#endif
 
#endif

