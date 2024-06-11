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

#ifndef _HASH_TAB_H_
#define _HASH_TAB_H_

#include "dynarr.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef int (HashTabCmp) (void * a, void * b);
typedef void (HashTabFree) (void * a);
typedef ulong (HashFunc) (void * key);

#pragma pack(push ,1)

typedef struct hash_node {
    int   count;
    void * dptr;
} hashnode_t;


typedef struct HashTab_ {

    unsigned  linear    : 30;
    unsigned  alloctype : 2; //0-default kalloc/kfree 1-os-specific malloc/free 2-kmempool alloc/free 3-kmemblk alloc/free

    int       num_requested;
    int       len;
    int       num;

    arr_t       * nodelist;
    hashnode_t  * ptab;

    HashFunc    * hashFunc;
    HashTabCmp  * cmp;

    void        * mpool;
#ifdef _DEBUG
    int         collide_tab[50];
#endif

} hashtab_t;

#pragma pack(pop)


ulong generic_hash (void * key, int keylen, ulong seed);
ulong string_hash (void * key, int keylen, ulong seed);

uint32 murmur_hash2    (void * key, int len, uint32 seed);
uint64 murmur_hash2_64 (void * key, int len, uint64 seed);

ulong ckstr_generic_hash (void * vkey);
ulong ckstr_string_hash (void * vkey);

hashtab_t * ht_only_alloc (int num, HashTabCmp * cmp, int alloctype, void * mpool);
hashtab_t * ht_alloc      (int num, HashTabCmp * cmp, int alloctype, void * mpool);

#define ht_only_osalloc(num, cmp)  ht_only_alloc((num),(cmp), 1, NULL)
#define ht_osalloc(num, cmp)       ht_alloc((num),(cmp), 1, NULL)

#define ht_only_mpalloc(num, cmp, mpool)  ht_only_alloc((num),(cmp), 2, mpool)
#define ht_mpalloc(num, cmp, mpool)       ht_alloc((num),(cmp), 2, mpool)

/* Create a hash table instance. The implementation method as following: first, find a 
 * prime number close to a given number as the total hash table space, and allocate hash
 * node space. Then, set the comparison function of keywords in the hash table, and set
 * the string hash function as the default hash function. */
hashtab_t * ht_new (int num, HashTabCmp * cmp);

/* there is no linear list existing in hashtab_t via this API */
hashtab_t * ht_only_new (int num, HashTabCmp * cmp);

void ht_set_generic_hash (hashtab_t * ht);

/* set the hash function as the user-defined function. */
void ht_set_hash_func (hashtab_t * ht, HashFunc * hashfunc);


/* release the space of the hash table instance. if the value numbers of 
 * the same hash value is greater than 1, then the stack instance will be
 * released. release the hash node list and release the hash table inst. */
void ht_free (hashtab_t * ht);


/* free all the space including the instance that hash table points to using
 * the given free function. */
void ht_free_all (hashtab_t * ht, void * vfunc);

/* free all the member using given free-function, and empty the hashtab */
void ht_free_member (hashtab_t * ht, void * vfunc);

/* clear all the nodes that set before. after invoking the api, the hashtab
 * will be as if the initial state just allocated using ht_new() */
void ht_zero (hashtab_t * ht);


/* return the actual number of hash value. */
int ht_num (hashtab_t * ht);


/* get the hash value that the key corresponds to. if the value corresponding
 * to the key is never set, NULL is returned. */
void * ht_get (hashtab_t * ht, void * key);


int ht_sort (hashtab_t * ht, HashTabCmp * cmp);


/* get the hash value according to the index location. the index value must
 * be from 0 to the total number minor 1. */
void * ht_value (hashtab_t * ht, int index);


/* set a hash value for a key. */
int ht_set (hashtab_t * ht, void * key, void * value);


/* traverse all the hash value and pass the value to the caller-provided
 * routine. */
void ht_traverse (hashtab_t * ht, void * usrInfo, void (*check)(void *, void *));


/* delete the value corresponding to the key. */
void * ht_delete (hashtab_t * ht, void * key);


/* delete all the hash-nodes that have the same pattern as the comp function defined.
 * traverse all the nodes of the hash table, invoke the comp function with the para of
 * the node and usrInfo. if result is 0, then delete this node, and free the node pointer
 * via function freeFunc. */
void ht_delete_pattern (hashtab_t * ht, void * usrInfo, HashTabCmp * cmp, HashTabFree * freef);


void print_hashtab (void * vht, FILE * fp);

#ifdef __cplusplus
}
#endif

#endif

