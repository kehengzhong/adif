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

#ifndef _DYNARR_H_
#define _DYNARR_H_

#ifdef  __cplusplus
extern "C" {
#endif

typedef int ArrCmp(void * a, void * b);

typedef struct DynArr_ {
    unsigned  num_alloc : 30;
    unsigned  alloctype : 2; //0-default kalloc/kfree 1-os-specific malloc/free 2-kmempool alloc/free 3-kmemblk alloc/free
    int       num;
    void    * mpool;
    void   ** data;
} arr_t;

arr_t * arr_alloc (int len, int alloctype, void * mpool);

/* Duplicate the whole dynamic array. In the copied dynamic array, the content
 * objects saved by each array member are exactly the same. Therefore, care
 * should be taken when releasing array instances. */
arr_t * arr_dup (arr_t * ar);

#define arr_new(len)             arr_alloc(len, 0, NULL)
#define arr_osalloc(len)         arr_alloc(len, 1, NULL)
#define arr_mpalloc(len, mpool)  arr_alloc(len, 2, mpool)

int     arr_insert    (arr_t * ar, void * data, int where);

void  * arr_delete_ptr(arr_t * ar, void * p);
void  * arr_delete    (arr_t * ar, int loc);

int     arr_push      (arr_t * ar, void * data);
void  * arr_pop       (arr_t * ar);
void    arr_zero      (arr_t * ar);

void    arr_free      (arr_t * ar);
void    arr_pop_free  (arr_t * ar, void * vfunc);
void    arr_pop_kfree (arr_t * ar);

int     arr_num  (arr_t *);
void  * arr_value(arr_t *, int);

void  * arr_last (arr_t * ar);
void  * arr_get  (arr_t *, int);
void  * arr_set  (arr_t *, int, void *);


/* enlarge the data array, the new nodes are assigned NULL */
int arr_enlarge (arr_t * ar, int scale);

/* call qsort to sort all the members with given comparing function
    int sort_by_cmp (void * a, void * b) {
        NodeST * nodea = *(NodeST **)a;
        NodeST * nodeb = *(NodeST **)b;

        return nodea->key - nodeb->key;
    }
 */
void arr_sort_by (arr_t * ar, ArrCmp * cmp);


/* the array should be sorted beforehand, or the member count is not greater
 *  than 1. seek a position that suits for the new member, and insert it
    int insert_by_cmp (void * a, void * b) {
        NodeST * nodea = (NodeST *)a;
        NodeST * nodeb = (NodeST *)b;

        return nodea->key - nodeb->key;
    } */
int arr_insert_by (arr_t * ar, void * item, ArrCmp * cmp);


/*  int findloc_by_cmp (void * a, void * b) {
        NodeST * node = (NodeST *)a;
        PatternST * pat = (PatternST *)b;

        return node->key - pat;
    }
 */
int arr_findloc_by (arr_t * ar, void * pattern, ArrCmp * cmp, int * found);

/* Use the comparison function provided by the caller to find a member that
 * matches the specified pattern. The array must be sorted by arr_sort_by
 * before calling this function. The sample code of the comparison function
 * is as follows:
    int find_by_cmp (void * a, void * b) {
        NodeST * node = (NodeST *)a;
        PatternST * pat = (PatternST *)b;

        return node->key - pat;
    } */
void * arr_find_by (arr_t * ar, void * pattern, ArrCmp * cmp);


/* find all members that matches the specified pattern through the comparing 
 * function provided by user.  the array must be sorted through arr_sort_by 
 * before invoking this function
    int find_all_cmp (void * a, void * b) {
        NodeST * node = (NodeST *)a;
        PatternST * pat = (PatternST *)b;

        return node->key - pat;
    }
 */
arr_t * arr_find_all_by (arr_t * ar, void * pattern, ArrCmp * cmp);

/* delete one member that matches the specified pattern through the comparing
 * function provided by user. the array must be sorted through arr_sort_by
 * before invoking this function
    int delete_by_cmp (void * a, void * b) {
        NodeST * node = (NodeST *)a;
        PatternST * pat = (PatternST *)b;

        return node->key - pat;
    }
 */
void * arr_delete_by (arr_t * ar, void * pattern, ArrCmp * cmp);


/* delete all members that matches the specified pattern through the comparing
 * function provided by user.  the array must be sorted through arr_sort_by
 * before invoking this function 
    int delete_all_by_cmp (void * a, void * b) {
        NodeST * node = (NodeST *)a;
        PatternST * pat = (PatternST *)b;

        return node->key - pat;
    }
 */
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

