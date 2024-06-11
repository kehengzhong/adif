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

#include "btype.h"
#include "memory.h"
#include "bitarr.h"
 
#define UNITBYTE  sizeof(uint64)
#define UNITBIT   (sizeof(uint64) * 8)
#define DIVBIT    6
#define MODBITS   0x3F


bitarr_t * bitarr_alloc (int bitnum)
{
    bitarr_t * bar = NULL;

    bar = kzalloc(sizeof(*bar));
    if (!bar) return NULL;

    bar->osalloc = 0;
    bar->bitnum = bitnum;
    bar->unitnum = (bitnum + UNITBIT - 1) / UNITBIT;

    bar->bitarr = kzalloc(bar->unitnum * UNITBYTE);
 
    return bar;
}

bitarr_t * bitarr_osalloc (int bitnum)
{
    bitarr_t * bar = NULL;

    bar = koszmalloc(sizeof(*bar));
    if (!bar) return NULL;

    bar->osalloc = 1;
    bar->bitnum = bitnum;
    bar->unitnum = (bitnum + UNITBIT - 1) / UNITBIT;
    
    bar->bitarr = koszmalloc(bar->unitnum * UNITBYTE);
    memset(bar->bitarr, 0, bar->unitnum * UNITBYTE);

    return bar;  
}

bitarr_t * bitarr_from_fixmem (void * pmem, int memlen, int bitnum, int * psize)
{
    bitarr_t * bar = NULL;

    if (psize) *psize = sizeof(*bar) + (bitnum + UNITBIT - 1) / UNITBIT * UNITBYTE;

    if (!pmem || memlen < UNITBYTE || memlen < (int)sizeof(*bar) + bitnum / 8)
        return NULL;

    bar = (bitarr_t *)pmem;

    bar->osalloc = 2;
    bar->bitnum = bitnum;
    bar->unitnum = (bitnum + UNITBIT - 1) / UNITBIT;

    bar->bitarr = (uint64 *)((uint8 *)pmem + sizeof(*bar));
    memset(bar->bitarr, 0, bar->unitnum * UNITBYTE);

    return bar;
}


bitarr_t * bitarr_resize (bitarr_t * bar, int bitnum)
{
    int   unitnum = 0;
    int   arrind = 0;
    int   bitoff = 0;
 
    if (!bar) {
        return bitarr_alloc(bitnum);
    }
 
    if (bitnum == bar->bitnum)
        return bar;
 
    unitnum = (bitnum + UNITBIT - 1) / UNITBIT;
 
    if ((int)bar->unitnum < unitnum) {
        if (bar->osalloc == 2)
            return bar;
        else if (bar->osalloc == 1)
            bar->bitarr = kosrealloc(bar->bitarr, unitnum * UNITBYTE);
        else
            bar->bitarr = krealloc(bar->bitarr, unitnum * UNITBYTE);
        bar->unitnum = unitnum;
    }
 
    /* all new allocated bits must be set 0 */

    arrind = bar->bitnum >> DIVBIT;
    bitoff = bar->bitnum & MODBITS;

    if (bitoff > 0)
        bar->bitarr[arrind] &= (uint64)~0 >> (UNITBIT - bitoff);

    for (arrind++ ; arrind < (int)bar->unitnum; arrind++) {
        bar->bitarr[arrind] = 0;
    }

    bar->bitnum = bitnum;
 
    return bar;
}

void bitarr_free (bitarr_t * bar)
{
    if (!bar) return;
 
    if (bar->bitarr) {
        if (bar->osalloc == 2) {
        } else if (bar->osalloc == 1)
            kosfree(bar->bitarr);
        else
            kfree(bar->bitarr);
        bar->bitarr = NULL;
    }
 
    bar->bitnum = 0;
    bar->unitnum = 0;

    if (bar->osalloc == 2) {
    } else if (bar->osalloc == 1)
        kosfree(bar);
    else
        kfree(bar);
}


int bitarr_set (bitarr_t * bar, int ind)
{
    int  arrind = 0;
    int  bitoff = 0;
 
    if (!bar) return -1;
 
    if (ind < 0) return -2;
 
    if (ind >= (int)bar->bitnum) {
        bitarr_resize(bar, ind);
    }
 
    arrind = ind >> DIVBIT;
    bitoff = ind & MODBITS;
 
    bar->bitarr[arrind] |= ((uint64)1 << bitoff);
 
    return 0;
}
 
int bitarr_unset (bitarr_t * bar, int ind)
{
    int  arrind = 0;
    int  bitoff = 0;
 
    if (!bar) return -1;
 
    if (ind < 0) return -2;
 
    if (ind >= (int)bar->bitnum) {
        bitarr_resize(bar, ind);
    }
 
    arrind = ind >> DIVBIT;
    bitoff = ind & MODBITS;
 
    bar->bitarr[arrind] &= ~((uint64)1 << bitoff);
 
    return 0;
}
 
int bitarr_get (bitarr_t * bar, int ind)
{
    int  arrind = 0;
    int  bitoff = 0;
 
    if (!bar) return -1;
 
    if (ind < 0) return -2;
 
    if (ind >= (int)bar->bitnum) {
        bitarr_resize(bar, ind);
    }
 
    arrind = ind >> DIVBIT;
    bitoff = ind & MODBITS;
 
    if (bar->bitarr[arrind] & ((uint64)1 << bitoff))
        return 1;
 
    return 0;
}
 
int bitarr_left (bitarr_t * bar, int bits)
{
    int     i = 0;
    uint64  value = 0;
    int     bitoff = 0;
    int     shiftunit = 0;
    int     unitnum = 0;
 
    if (!bar) return -1;
 
    if (bits >= (int)bar->bitnum) {
        memset(bar->bitarr, 0, bar->unitnum * UNITBYTE);
        return 0;
    }

    unitnum = (bar->bitnum + UNITBIT - 1) / UNITBIT;

    shiftunit = bits >> DIVBIT;
    bitoff = bits & MODBITS;
 
    if (shiftunit > 0) {
        for (i = unitnum - 1; i >= shiftunit; i--) {
            bar->bitarr[i] = bar->bitarr[i - shiftunit];
        }

        /* clear the first unused unit */
        for (i = 0; i < shiftunit; i++) {
            bar->bitarr[i] = 0;
        }
    }
 
    for (i = unitnum - 1; bitoff > 0 && i >= shiftunit; i--) {
        bar->bitarr[i] <<= bitoff;

        if (i > shiftunit) {
            value = bar->bitarr[i - 1] >> (UNITBIT - bitoff);
            bar->bitarr[i] |= value;
        }
    }

    /* clear unused bits of last uint64 unit */
    bitoff = bar->bitnum & MODBITS;
    bar->bitarr[unitnum - 1] &= (uint64)~0 >> (UNITBIT - bitoff);

    return 0;
}

int bitarr_right (bitarr_t * bar, int bits)
{
    int     i = 0;
    uint64  value = 0;
    int     bitoff = 0;
    int     shiftunit = 0;
    int     unitnum = 0;
 
    if (!bar) return -1;
 
    if (bits >= (int)bar->bitnum) {
        memset(bar->bitarr, 0, bar->unitnum * UNITBYTE);
        return 0;
    }

    unitnum = (bar->bitnum + UNITBIT - 1) / UNITBIT;

    shiftunit = bits >> DIVBIT;
    bitoff = bits & MODBITS;
 
    if (shiftunit > 0) {
        for (i = 0; i < unitnum - shiftunit; i++) {
            bar->bitarr[i] = bar->bitarr[i + shiftunit];
        }

        /* clear the remaining unit */
        for (i = unitnum - shiftunit; i < unitnum; i++) {
            bar->bitarr[i] = 0;
        }
    }
 
    for (i = 0; bitoff > 0 && i < unitnum - shiftunit; i++) {
        bar->bitarr[i] >>= bitoff;

        if (i < unitnum - shiftunit - 1) {
            value = bar->bitarr[i + 1] << (UNITBIT - bitoff);
            bar->bitarr[i] |= value;
        }
    }

    return 0;
}

int bitarr_and (bitarr_t * dst, bitarr_t * src)
{
    int  unitnum = 0;
    int  bitoff = 0;
    int  i = 0;

    if (!dst || !src) return -1;

    if (dst->bitnum <= src->bitnum) {

        unitnum = (dst->bitnum + UNITBIT - 1) / UNITBIT;
        bitoff = dst->bitnum & MODBITS;

        for (i = 0; i < unitnum; i++) {
            dst->bitarr[i] &= src->bitarr[i];
        }

        if (bitoff > 0)
            dst->bitarr[unitnum - 1] &= (uint64)~0 >> (UNITBIT - bitoff);

    } else {

        unitnum = (src->bitnum + UNITBIT - 1) / UNITBIT;
        bitoff = src->bitnum & MODBITS;
 
        for (i = 0; i < unitnum - 1; i++) {
            dst->bitarr[i] &= src->bitarr[i];
        }
 
        dst->bitarr[unitnum - 1] &= ((uint64)~0 << bitoff) | src->bitarr[unitnum - 1];
    }

    return 0;
}

int bitarr_or (bitarr_t * dst, bitarr_t * src)
{
    int  unitnum = 0;
    int  bitoff = 0;
    int  i = 0;

    if (!dst || !src) return -1;

    if (dst->bitnum <= src->bitnum) {

        unitnum = (dst->bitnum + UNITBIT - 1) / UNITBIT;
        bitoff = dst->bitnum & MODBITS;

        for (i = 0; i < unitnum; i++) {
            dst->bitarr[i] |= src->bitarr[i];
        }

        if (bitoff > 0)
            dst->bitarr[unitnum - 1] &= (uint64)~0 >> (UNITBIT - bitoff);

    } else {

        unitnum = (src->bitnum + UNITBIT - 1) / UNITBIT;
        bitoff = src->bitnum & MODBITS;
 
        for (i = 0; i < unitnum - 1; i++) {
            dst->bitarr[i] |= src->bitarr[i];
        }
 
        dst->bitarr[unitnum - 1] |= ((uint64)~0 >> (UNITBIT - bitoff)) & src->bitarr[unitnum - 1];
    }

    return 0;
}

int bitarr_xor (bitarr_t * dst, bitarr_t * src)
{
    int  unitnum = 0;
    int  bitoff = 0;
    int  i = 0;

    if (!dst || !src) return -1;

    if (dst->bitnum <= src->bitnum) {

        unitnum = (dst->bitnum + UNITBIT - 1) / UNITBIT;
        bitoff = dst->bitnum & MODBITS;

        for (i = 0; i < unitnum; i++) {
            dst->bitarr[i] ^= src->bitarr[i];
        }

        if (bitoff > 0)
            dst->bitarr[unitnum - 1] &= (uint64)~0 >> (UNITBIT - bitoff);

    } else {

        unitnum = (src->bitnum + UNITBIT - 1) / UNITBIT;
        bitoff = src->bitnum & MODBITS;
 
        for (i = 0; i < unitnum - 1; i++) {
            dst->bitarr[i] ^= src->bitarr[i];
        }
 
        dst->bitarr[unitnum - 1] ^= ((uint64)~0 >> (UNITBIT - bitoff)) & src->bitarr[unitnum - 1];
    }

    return 0;
}



int bitarr_cleared (bitarr_t * bar)
{
    int    i = 0, arrind = 0;
    int    restbit = 0;
 
    if (!bar) return -1;
 
    for (i = 0; i < (int)bar->bitnum; i += UNITBIT) {
        restbit = bar->bitnum - i;
 
        if (restbit < UNITBIT)
            restbit = UNITBIT - restbit;
        else
            restbit = 0;
 
        arrind = i / UNITBIT;
 
        if ((bar->bitarr[arrind] & ((uint64)-1 >> restbit)) != 0)
            return 0;
    }
 
    return 1;
}
 
int bitarr_filled (bitarr_t * bar)
{
    int    i = 0, arrind = 0;
    int    restbit = 0;

    if (!bar) return -1;

    for (i = 0; i < (int)bar->bitnum; i += UNITBIT) {
        restbit = bar->bitnum - i;

        if (restbit < UNITBIT)
            restbit = UNITBIT - restbit;
        else
            restbit = 0;

        arrind = i / UNITBIT;

        if (((bar->bitarr[arrind] ^ (uint64)-1) & ((uint64)-1 >> restbit)) != 0)
            return 0;
    }

    return 1;
}

 
int bitarr_zero  (bitarr_t * bar)
{
    if (!bar) return -1;
 
    memset(bar->bitarr, 0, sizeof(uint64) * bar->unitnum);
 
    return 0;
}

int bitarr_fill  (bitarr_t * bar)
{
    if (!bar) return -1;

    memset(bar->bitarr, 0xFF, sizeof(uint64) * bar->unitnum);

    return 0;
}

void bitarr_print (bitarr_t * bar, FILE * fp)
{
    int  i = 0, n = 0;

    if (!bar) return;
    if (!fp) fp = stdout;

    fprintf(fp, "bitarr: bitnum=%d  unitnum=%d\n", bar->bitnum, bar->unitnum);

    printf("----|");
    for (i = 0; i < 32; i++) {
        if (i  % 8 == 0)
            fprintf(fp, "%-2d", i);
        else
            fprintf(fp, "--");
    }
    fprintf(fp, "\n");
    fprintf(fp, " 0  |");

    //for (i = bar->bitnum - 1; i >= 0; i--, n++) {
    for (i = 0; i < (int)bar->bitnum; i++, n++) {
        fprintf(fp, "%d ", bitarr_get(bar, i) == 0 ? 0 : 1);
        if ((n + 1) % 32 == 0) {
            fprintf(fp, "\n");
            fprintf(fp, " %-2d |", (n+1)/32);
        }
    }
    fprintf(fp, "\n");
}

int bit_mask_get (uint32 * bitmap, uint8 ch)
{
    if (!bitmap) return 0;
 
    return bitmap[ch >> 5] & (1U << (ch & 0x1f));
}
 
void bit_mask_set (uint32 * bitmap, uint8 ch)
{
    if (!bitmap) return; 
 
    bitmap[ch >> 5] |= (1U << (ch & 0x1f));
}
 
void bit_mask_unset (uint32 * bitmap, uint8 ch)
{
    if (!bitmap) return;
 
    bitmap[ch >> 5] &= ~(1U << (ch & 0x1f));
}

