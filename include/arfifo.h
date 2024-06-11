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

#ifndef _ARFIFO_H_
#define _ARFIFO_H_ 
    
#include "mthread.h"

#ifdef __cplusplus
extern "C" {
#endif 

typedef struct ar_fifo_s {
    CRITICAL_SECTION   afCS;

    uint8     alloctype : 4;//0-default kalloc/kfree 1-os-specific malloc/free 2-kmempool alloc/free 3-kmemblk alloc/free
    uint8     fixmem    : 4;
    void    * mpool;

    int       size;
    int       start;
    int       num;

    void   ** data;

} arfifo_t, *arfifo_p;


arfifo_t * ar_fifo_alloc (int size, int alloctype, void * mpool);
#define ar_fifo_new(size) ar_fifo_alloc(size, 0, NULL)
#define ar_fifo_osalloc(size) ar_fifo_alloc(size, 1, NULL)

arfifo_t * ar_fifo_from_fixmem (void * pmem, int memlen, int fifosize, int * pmemsize);

void   ar_fifo_free  (arfifo_t * af);
void   ar_fifo_free_all (arfifo_t * af, void * freefunc);

void   ar_fifo_zero  (arfifo_t * af);

int    ar_fifo_num   (arfifo_t * af);
void * ar_fifo_value (arfifo_t * af, int i);

int    ar_fifo_push  (arfifo_t * af, void * value);
void * ar_fifo_out   (arfifo_t * af);

void * ar_fifo_head  (arfifo_t * af);
void * ar_fifo_tail  (arfifo_t * af);


/******************************************
FIFO example:

arfifo_t * af = NULL;
void * val = NULL;

af = ar_fifo_new(8);

ar_fifo_push(af, pack1);
ar_fifo_push(af, pack2);
ar_fifo_push(af, pack3);
ar_fifo_push(af, pack4);

val = ar_fifo_out(af);

ar_fifo_free(af);

*******************************************/

#ifdef __cplusplus
}
#endif 
 
#endif

