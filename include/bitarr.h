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

#ifndef _BITARRAR_H_
#define _BITARRAR_H_ 
    
#ifdef __cplusplus
extern "C" {
#endif 

typedef struct bit_arr_s {

    int          bitnum;

    unsigned     unitnum : 30;
    unsigned     osalloc : 2;

    uint64     * bitarr;
} bitarr_t;
 

bitarr_t * bitarr_alloc  (int bitnum);

/* Memory is allocated by calling malloc function of the system instead of memory pool. */
bitarr_t * bitarr_osalloc (int bitnum);

bitarr_t * bitarr_from_fixmem (void * pmem, int memlen, int bitnum, int * psize);

bitarr_t * bitarr_resize (bitarr_t * bar, int bitnum);
void       bitarr_free   (bitarr_t * bar);

int bitarr_set   (bitarr_t * bar, int ind);
int bitarr_unset (bitarr_t * bar, int ind);
int bitarr_get   (bitarr_t * bar, int ind);

int bitarr_left  (bitarr_t * bar, int bits);
int bitarr_right (bitarr_t * bar, int bits);

int bitarr_and (bitarr_t * dst, bitarr_t * src);
int bitarr_or  (bitarr_t * dst, bitarr_t * src);
int bitarr_xor (bitarr_t * dst, bitarr_t * src);

int bitarr_cleared (bitarr_t * bar);
int bitarr_filled (bitarr_t * bar);

int bitarr_zero   (bitarr_t * bar);
int bitarr_fill   (bitarr_t * bar);

void bitarr_print (bitarr_t * bar, FILE * fp);

int  bit_mask_get (uint32 * bitmap, uint8 ch);
void bit_mask_set (uint32 * bitmap, uint8 ch);
void bit_mask_unset (uint32 * bitmap, uint8 ch);

#ifdef __cplusplus
}
#endif 
 
#endif

