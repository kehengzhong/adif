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

#ifndef _HEAP_H_
#define _HEAP_H_

#ifdef  __cplusplus
extern "C" {
#endif

typedef int heapcmp_t (void * a, void * b);

typedef struct HeapST_ {
    int          num_alloc;
    int          num;
    heapcmp_t  * cmp;
    void      ** data;
} heap_t, *heap_p;

heap_t * heap_dup (heap_t * hp);
heap_t * heap_new (heapcmp_t * cmp, int len);

void     heap_pop_free  (heap_t * hp, void * vfunc);
void     heap_pop_kfree (heap_t * hp);
void     heap_free      (heap_t * hp);

void     heap_zero    (heap_t * hp);
int      heap_num     (heap_t *);
void   * heap_value   (heap_t *, int);
int      heap_set     (heap_t * hp, int i, void * val);
int      heap_enlarge (heap_t * hp, int scale);

int      heap_push (heap_t * hp, void * data);
void   * heap_pop  (heap_t * hp);

void     heap_sort (heap_t * hp);

int      heap_heapify (heap_t * hp);
int      heap_from    (heap_t * hp, void ** ar, int num);

void   * heap_search  (heap_t * hp, void * pattern, heapcmp_t * cmp);

#ifdef  __cplusplus
}
#endif

#endif

