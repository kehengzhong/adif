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

#ifndef _FRAG_PACK_H_
#define _FRAG_PACK_H_

#include "mthread.h"
#include "vstar.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct frag_item {
    int64    offset;
    int64    length;
} FragItem;
 
typedef struct FragPack_ {

    uint8   complete  : 6;
    uint8   alloctype : 2;//0-default kalloc/kfree 1-os-specific malloc/free 2-kmempool alloc/free 3-kmemblk alloc/free

    int64   length;
    int64   rcvlen;
 
    void  * mpool;

    CRITICAL_SECTION   packCS;
    vstar_t          * pack_var;

} FragPack;

void * frag_pack_alloc (int alloctype, void * mpool);
void   frag_pack_free  (void * vfrag);

int    frag_pack_zero (void * vfrag);

void   frag_pack_set_length (void * vfrag, int64 length);
int    frag_pack_complete   (void * vfrag);
int64  frag_pack_length     (void * vfrag);
int64  frag_pack_rcvlen     (void * vfrag, int * fragnum);
int64  frag_pack_curlen     (void * vfrag);

int    frag_pack_read (void * vfrag, void * hfile, int64 pos);
int    frag_pack_write (void * vfrag, void * hfile, int64 pos);

int    frag_pack_add (void * vfrag, int64 pos, int64 len);
int    frag_pack_del (void * vfrag, int64 pos, int64 len);
int    frag_pack_get (void * vfrag, int64 pos, int64 * actpos, int64 * actlen);

int    frag_pack_gap (void * vfrag, int64 pos, int64 * gappos, int64 * gaplen);

/* check if the given range pos:length is contained.
   return value: 0 - completely not contained
                 1 - right-side partial contained,   !    |--!-------|
                 2 - left-side partial contained,    |------!---|    !
                 3 - completely contained            |-----!----!----|
 */
int    frag_pack_contain (void * vfrag, int64 pos, int64 length, int64 * datapos,
                          int64 * datalen, int64 * gappos, int64 * gaplen);

void   frag_pack_print (void * vfrag, FILE * fp);

#ifdef __cplusplus
}
#endif

#endif

