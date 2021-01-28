/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#ifndef _DYNARR_H_
#define _DYNARR_H_

#ifdef  __cplusplus
extern "C" {
#endif

typedef int ArrCmp(void * a, void * b);

typedef struct DynArr_ {
    int      num_alloc;
    int      num;
    void  ** data;
} arr_t;

int     arr_num  (arr_t *);
void  * arr_value(arr_t *, int);

void  * arr_last (arr_t * ar);
void  * arr_set  (arr_t *, int, void *);
void  * arr_get  (arr_t *, int);

arr_t * arr_new  (int len);

void    arr_free      (arr_t * ar);
void    arr_pop_free  (arr_t * ar, void * vfunc);
void    arr_pop_kfree (arr_t * ar);

int     arr_insert    (arr_t * ar, void * data, int where);
int     arr_push      (arr_t * ar, void * data);
void  * arr_pop       (arr_t * ar);
void    arr_zero      (arr_t * ar);

void  * arr_delete    (arr_t * ar, int loc);
void  * arr_delete_ptr(arr_t * ar, void * p);


/* enlarge the data array, the new nodes are assigned NULL */
int arr_enlarge (arr_t * ar, int scale);

/* duplicate the whole array member. the actual content pointed to by the data
 * field are same. therefore, when releasing the actual content, caution should
 * be taken. */
arr_t * arr_dup(arr_t * ar);

/* call qsort to sort all the members with given comparing function */
void arr_sort_by (arr_t * ar, ArrCmp * cmp);


/* the array should be sorted beforehand, or the member count is not greater
 *  than 1. seek a position that suits for the new member, and insert it */
int arr_insert_by (arr_t * ar, void * item, ArrCmp * cmp);


int arr_findloc_by (arr_t * ar, void * pattern, ArrCmp * cmp, int * found);

/* find one member that matches the specified pattern through the comparing
 * function provided by user. the array must be sorted through arr_sort_by 
 * before invoking this function */
void * arr_find_by (arr_t * ar, void * pattern, ArrCmp * cmp);


/* find all members that matches the specified pattern through the comparing 
 * function provided by user.  the array must be sorted through arr_sort_by 
 * before invoking this function */
arr_t * arr_find_all_by (arr_t * ar, void * pattern, ArrCmp * cmp);

/* delete one member that matches the specified pattern through the comparing
 * function provided by user. the array must be sorted through arr_sort_by
 * before invoking this function */
void * arr_delete_by (arr_t * ar, void * pattern, ArrCmp * cmp);


/* delete all members that matches the specified pattern through the comparing
 * function provided by user.  the array must be sorted through arr_sort_by
 * before invoking this function */
arr_t * arr_delete_all_by (arr_t * ar, void * pattern, ArrCmp * cmp);

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
void * arr_consistent_hash_node (arr_t * ar, void * pattern, ArrCmp * cmp);

/* linear search one by one. it's a slow search method. */
void * arr_search (arr_t * ar, void * pattern, ArrCmp * cmp);

/* linear search one by one. all matched members will be gathered into 
 * a array as output. */
arr_t * arr_search_all (arr_t * ar, void * pattern, ArrCmp * cmp);


#ifdef  __cplusplus
}
#endif

#endif

