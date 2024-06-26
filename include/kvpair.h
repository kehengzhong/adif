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

#ifndef _KVPAIR_H_
#define _KVPAIR_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct kvpair_value {
    uint8    * value;
    int        valuelen;
} KVPairValue;
 
 
typedef struct kvpair_item {
    unsigned   namelen   : 30;
    unsigned   alloctype : 2; //0-default kalloc/kfree 1-os-specific malloc/free 2-kmempool alloc/free 3-kmemblk alloc/free

    void     * mpool;

    uint8    * name;
 
    int        valnum;
    void     * valobj;  //num=1->KVPairValue, num>1->arr_t containing KVPairValue array
} KVPairItem;
 
 
typedef struct kvpair_obj {
    unsigned           htsize : 30;
    unsigned           alloctype : 2; //0-default kalloc/kfree 1-os-specific malloc/free 2-kmempool alloc/free 3-kmemblk alloc/free

    void             * mpool;

    CRITICAL_SECTION   objCS;
    hashtab_t        * objtab;
 
    uint8              sepa[16];
    uint8              kvsep[8];
} KVPairObj;

void * kvpair_alloc (int htsize, char * sepa, char * kvsep, int alloctype, void * mpool);
void * kvpair_init  (int htsize, char * sepa, char * kvsep);
int    kvpair_clean (void * vobj);
int    kvpair_zero  (void * vobj);

int    kvpair_valuenum (void * vobj, void * key, int keylen);

int    kvpair_num (void * vobj);

int    kvpair_seq_get (void * vobj, int seq, void ** pkey, int * keylen, int index, void ** pval, int * vallen);
int    kvpair_getP (void * vobj, void * key, int keylen, int ind, void ** pval, int * vallen);
int    kvpair_get  (void * vobj, void * key, int keylen, int ind, void * val, int * vallen);

int    kvpair_del (void * vobj, void * key, int keylen, int index);

int    kvpair_get_int8   (void * vobj, void * key, int keylen, int ind, int8 * val);
int    kvpair_get_uint8  (void * vobj, void * key, int keylen, int ind, uint8 * val);
int    kvpair_get_int16  (void * vobj, void * key, int keylen, int ind, int16 * val);
int    kvpair_get_uint16 (void * vobj, void * key, int keylen, int ind, uint16 * val);
int    kvpair_get_int    (void * vobj, void * key, int keylen, int ind, int * val);
int    kvpair_get_uint32 (void * vobj, void * key, int keylen, int ind, uint32 * val);
int    kvpair_get_long   (void * vobj, void * key, int keylen, int ind, long * val);
int    kvpair_get_ulong  (void * vobj, void * key, int keylen, int ind, ulong * val);
int    kvpair_get_int64  (void * vobj, void * key, int keylen, int ind, int64 * val);
int    kvpair_get_uint64 (void * vobj, void * key, int keylen, int ind, uint64 * val);
int    kvpair_get_double (void * vobj, void * key, int keylen, int index, double * val);

int    kvpair_add     (void * vobj, void * key, int keylen, void * val, int vallen);

int    kvpair_add_by_fca (void * vobj, void * fca, long keypos, int keylen, long valpos, int vallen);

int    kvpair_add_int8   (void * vobj, void * key, int keylen, int8 val);
int    kvpair_add_uint8  (void * vobj, void * key, int keylen, uint8 val);
int    kvpair_add_int16  (void * vobj, void * key, int keylen, int16 val);
int    kvpair_add_uint16 (void * vobj, void * key, int keylen, uint16 val);
int    kvpair_add_int    (void * vobj, void * key, int keylen, int val);
int    kvpair_add_uint32 (void * vobj, void * key, int keylen, uint32 val);
int    kvpair_add_long   (void * vobj, void * key, int keylen, long val);
int    kvpair_add_ulong  (void * vobj, void * key, int keylen, ulong val);
int    kvpair_add_int64  (void * vobj, void * key, int keylen, int64 val);
int    kvpair_add_uint64 (void * vobj, void * key, int keylen, uint64 val);
int    kvpair_add_double (void * vobj, void * key, int keylen, double val);

int    kvpair_decode (void * vobj, void * p, int len);
int    kvpair_encode (void * vobj, void * p, int len);

long   kvpair_fca_decode (void * vobj, void * fca, long startpos, long length);

#ifdef __cplusplus
}
#endif

#endif


