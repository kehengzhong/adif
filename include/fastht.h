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

#ifndef _FAST_HTAB_H_
#define _FAST_HTAB_H_


#ifdef __cplusplus
extern "C" {
#endif

#define HASH_OFFSET 0
#define HASH_A  1
#define HASH_B 2

#pragma pack(push ,1)

typedef struct fash_hash_node {
    uint8     exist;
    uint32    hashA;
    uint32    hashB;

    void    * value;
    int       valuelen;
} FastHashNode;

#pragma pack(pop)


typedef struct fash_hash_tab {

    ulong          reqsize;
    ulong          size;

    int            num;
    FastHashNode * ptab;

} FastHashTab;


uint32 fast_hash_func (void * key, int keylen, int hashtype);

void * fast_ht_new (ulong size);
void   fast_ht_free (void * vht);

void   fast_ht_free_all (void * vht, void * vfunc);
void   fast_ht_free_member (void * vht, void * vfunc);
void   fast_ht_zero (void * vht);

int    fast_ht_num (void * vht);

void * fast_ht_get (void * vht, void * key, int keylen, void ** pval, int * vallen);
int    fast_ht_set (void * vht, void * key, int keylen, void * value, int valuelen);
void * fast_ht_del (void * vht, void * key, int keylen, void ** pval, int * vallen);


#ifdef __cplusplus
}
#endif


#endif


