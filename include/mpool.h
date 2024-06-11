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

#ifndef _MPOOL_H_
#define _MPOOL_H_ 
    
#ifdef __cplusplus
extern "C" {
#endif 

struct mem_pool_s;
typedef struct mem_pool_s mpool_t;


mpool_t * mpool_alloc ();
int       mpool_free  (mpool_t * mp);

/* Memory is allocated by calling malloc function of the system instead of memory pool. */
mpool_t * mpool_osalloc ();

void * mpool_fetch   (mpool_t * mp);
int    mpool_recycle (mpool_t * mp, void * unit);

int    mpool_set_allocnum (mpool_t * mp, int num);
int    mpool_set_unitsize (mpool_t * mp, int size);
int    mpool_set_freesize (mpool_t * mp, int size);

int    mpool_set_initfunc (mpool_t * mp, void * func);
int    mpool_set_freefunc (mpool_t * mp, void * func);
int    mpool_set_usizefunc (mpool_t * mp, void * func);

int    mpool_allocnum (mpool_t * mp);
int    mpool_unitsize (mpool_t * mp);
int    mpool_freesize (mpool_t * mp);

int    mpool_fetched_num (mpool_t * mp);
int    mpool_allocated (mpool_t * mp);
int    mpool_consumed (mpool_t * mp);
int    mpool_remaining (mpool_t * mp);

int    mpool_status (mpool_t * mp, int * allocated, int * remaining, int * consumed, int * cachenum);
int    mpool_para (mpool_t * mp, int * allocnum, int * unitsize, int * blksize);

long   mpool_size (mpool_t * mp);

void   mpool_print (mpool_t * mp, char * title, int margin, void * frm, FILE * fp);

/******************************************
MPool example:

void * mp = NULL;
void * val = NULL;

mp = mpool_alloc();
mpool_set_unitsize(mp, sizeof(PackLoc));
mpool_set_allocnum(mp, 100);

val = mpool_fetch(mp);
...
mpool_recycle(mp, val);

mpool_free(mp);

*******************************************/

#ifdef __cplusplus
}
#endif 
 
#endif

