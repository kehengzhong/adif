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

#ifndef _MEMALLOC_H_
#define _MEMALLOC_H_ 

#include "btype.h"
#include "dynarr.h"
#include "mpool.h"
#include "frame.h"
#include "rbtree.h"
#include "mthread.h"
    
#ifdef __cplusplus
extern "C" {
#endif 

/* This module implements a mechanism of allocating memory units from pre-allocated
   large memory blocks. The allocated memory unit and the requested size are all 8
   bytes aligned. Memory units allocated from large memory blocks can be of any size.
   If a large memory block is used out, a new large memory block is automatically allocated. 
   Memory pool mechanism is used to manage the allocation and release of large memory
   blocks. At the peak of business operation, more memory is consumed, and more memory
   blocks will be automatically allocated to meet the storage needs. At the low point
   of business operation, a large number of memory units allocated earlier will be
   released, resulting in the automatic release of memory blocks without outstanding
   allocated memory units. Each large memory block uses a red-black tree to manage
   allocated memory units and a dynamic array to manage idle memory units. Adjacent
   idle memory units will be merged together for later allocation. */


typedef struct kem_block_ {
    time_t             stamp;
    long               size;
    long               actsize;
    uint8              allocflag : 4; //indicate if KemBlk instance is allocated
    uint8              alloctype : 4; //0-kalloc/kfree 1-kosalloc/kosfree 2-kemalloc 3-kemblk alloc/free

    mpool_t          * kemunit_pool;

    void             * kmpool;

    CRITICAL_SECTION   memCS;
    long               allocsize;
    long               restsize;
    int                allocnum;

    CRITICAL_SECTION   alloctreeCS;
    CRITICAL_SECTION   idletreeCS;

    rbtree_t         * alloc_tree;
    rbtree_t         * size_idle_tree;
    rbtree_t         * pos_idle_tree;
    rbtree_t           tree_mem[3];

    uint8            * pbgn;
    uint8              pmem[1];

} KemBlk;

int    kemblk_init  (void * mp, void * vblk, long size, int mpunit);
void * kemblk_alloc (void * mp, long size);

#define kemblk_free(blk) kemblk_free_dbg(blk, __FILE__, __LINE__)
void   kemblk_free_dbg (void * vblk, char * file, int line);

int kemblk_brief (void * vblk, char * buf, int size);

#define kemblk_alloc_unit(blk, size) kemblk_alloc_unit_dbg(blk, size, __FILE__, __LINE__)
void * kemblk_alloc_unit_dbg (void * blk, int size, char * file, int line);

#define kemblk_free_unit(blk, p) kemblk_free_unit_dbg(blk, p, __FILE__, __LINE__)
int kemblk_free_unit_dbg (void * blk, void * p, char * file, int line);

#define kemblk_realloc_unit(blk, p, size) kemblk_realloc_unit_dbg(blk, p, size, __FILE__, __LINE__)
void * kemblk_realloc_unit_dbg (void * vblk, void * p, int resize, char * file, int line);


typedef struct kem_alloc_pool_s {
    uint8              alloctype;

    CRITICAL_SECTION   mpCS;
    arr_t            * blk_list;
    arr_t            * sort_blk_list;

    long               blksize;
    int                allocnum; //total number of allocated memory units
    time_t             checktime;

    mpool_t          * kemunit_pool;

} KemPool, kempool_t, *kempool_p;


KemPool * kempool_alloc (long size, int unitnum);
int       kempool_free (KemPool * mp);

long kempool_size (KemPool * mp);

void kempool_print (KemPool * mp, frame_t * frm, FILE * fp, int alloclist,
                    int idlelist, int showsize, char * title, int margin);

#define kem_alloc(mp, size) kem_alloc_dbg(mp, size, __FILE__, __LINE__)
void * kem_alloc_dbg (void * vmp, int size, char * file, int line);

#define kem_zalloc(mp, size) kem_zalloc_dbg(mp, size, __FILE__, __LINE__)
void * kem_zalloc_dbg(void * vmp, int size, char * file, int line);

#define kem_free(mp, p) kem_free_dbg(mp, p, __FILE__, __LINE__)
int kem_free_dbg (void * vmp, void * p, char * file, int line);

#define kem_realloc(mp, p, resize) kem_realloc_dbg(mp, p, resize, __FILE__, __LINE__)
void * kem_realloc_dbg (void * vmp, void * p, int resize, char * file ,int line);

int    kem_size    (void * vmp, void * p);
void * kem_by_index (void * vmp, int index);


void * k_mem_alloc_dbg   (int size, int alloctype, void * mpool, char * file, int line);
void * k_mem_zalloc_dbg  (int size, int alloctype, void * mpool, char * file, int line);
void * k_mem_realloc_dbg (void * pm, int size, int alloctype, void * mpool, char * file, int line);
void   k_mem_free_dbg    (void * pm, int alloctype, void * mpool, char * file, int line);

#define k_mem_alloc(size, atype, mpool)       k_mem_alloc_dbg(size, atype, mpool, __FILE__, __LINE__)
#define k_mem_zalloc(size, atype, mpool)      k_mem_zalloc_dbg(size, atype, mpool, __FILE__, __LINE__)
#define k_mem_realloc(p, size, atype, mpool)  k_mem_realloc_dbg(p, size, atype, mpool, __FILE__, __LINE__)
#define k_mem_free(p, atype, mpool)           k_mem_free_dbg(p, atype, mpool, __FILE__, __LINE__)

#define k_mem_str_dup(p, size, atype, mpool)  k_mem_str_dup_dbg(p, size, atype, mpool, __FILE__, __LINE__)
void * k_mem_str_dup_dbg (void * p, int bytelen, int alloctype, void * mpool, char * file, int line);


/******************************************
KemAlloc example:

KemPool * mp = NULL;
void * val = NULL;

mp = kempool_alloc(1024*1024);

val = kem_alloc(mp, 1024);
...
kem_free(mp, val);

kempool_free(mp);

*******************************************/

#ifdef __cplusplus
}
#endif 
 
#endif

