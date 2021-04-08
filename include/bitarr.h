/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#ifndef _BITARRAR_H_
#define _BITARRAR_H_ 
    
#ifdef __cplusplus
extern "C" {
#endif 

typedef struct bit_arr_s {

    unsigned     alloc:1;
    unsigned     bitnum:31;

    int          unitnum;
    uint64     * bitarr;
} bitarr_t;
 

bitarr_t * bitarr_alloc  (int bitnum);
void       bitarr_init   (bitarr_t * bar, int bitnum);
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

int bitarr_filled (bitarr_t * bar);
int bitarr_zero   (bitarr_t * bar);

void bitarr_print (bitarr_t * bar);

int  bit_mask_get (uint32 * bitmap, uint8 ch);
void bit_mask_set (uint32 * bitmap, uint8 ch);
void bit_mask_unset (uint32 * bitmap, uint8 ch);

#ifdef __cplusplus
}
#endif 
 
#endif

