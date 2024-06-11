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
 * #                  .' \\|     |# '.                 #
 * #                 / \\|||  :  |||# \                #
 * #                / _||||| -:- |||||- \              #
 * #               |   | \\\  -  #/ |   |              #
 * #               | \_|  ''\---/''  |_/ |             #
 * #               \  .-\__  '-'  ___/-. /             #
 * #             ___'. .'  /--.--\  `. .'___           #
 * #          ."" '<  `.___\_<|>_/___.' >' "".         #
 * #         | | :  `- \`.;`\ _ /`;.`/ - ` : | |       #
 * #         \  \ `_.   \_ __\ /__ _/   .-` /  /       #
 * #     =====`-.____`.___ \_____/___.-`___.-'=====    #
 * #                       `=---='                     #
 * #     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   #
 * #               佛力加持      佛光普照              #
 * #  Buddha's power blessing, Buddha's light shining  #
 * #####################################################
 */

#ifndef _BUFPOOL_H_
#define _BUFPOOL_H_

#ifdef __cplusplus
extern "C" {
#endif


typedef int  (PoolUnitInit) (void *);
typedef void (PoolUnitFree) (void *);
typedef int  (PoolUnitSize) (void *);

struct buffer_pool;
typedef struct buffer_pool bpool_t;

bpool_t * bpool_init  (bpool_t * pool);
int       bpool_clean (bpool_t * pool);

void    * bpool_fetch   (bpool_t * pool);
int       bpool_recycle (bpool_t * pool, void * unit);

int       bpool_set_initfunc    (bpool_t * pool, void * init);
int       bpool_set_freefunc    (bpool_t * pool, void * free);
int       bpool_set_getsizefunc (bpool_t * pool, void * getsize);

int       bpool_set_unitsize  (bpool_t * pool, int size);
int       bpool_set_allocnum  (bpool_t * pool, int escl);
int       bpool_set_freesize  (bpool_t * pool, int size);

int       bpool_allocated (bpool_t * mp);
int       bpool_consumed (bpool_t * mp);
int       bpool_remaining (bpool_t * mp);

int       bpool_status (bpool_t * pool, int * allocated, int * remaining,
                        int * consumed, int * fifonum, int * refifonum);

#ifdef __cplusplus
}
#endif

#endif

