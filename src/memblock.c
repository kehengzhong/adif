/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include <stdlib.h>
#include <string.h>

#include "memblock.h"


typedef struct MemBlock_ {
    int      totalsize;
    int      unused;
    int      size;
    int      start;
    int      unitsize;
    int      qlen;
    uint8  * storage;
    int      arr[1];
} MemBlock;


int mem_block_init (void * psb, int totalsize, int unitsize)
{
    MemBlock  * block = NULL;
    int         arsize = 0;
    int         i = 0;

    if (!psb)
        return -1;

    if (totalsize <= 0)
        return -2;

    if (unitsize <= 0)
        return -3;

    arsize = totalsize - sizeof(MemBlock) + sizeof(int);
    arsize /= (unitsize + 2 * sizeof(int));
    if (arsize < 1) return -4;

    block = (MemBlock *)psb;

    block->totalsize = totalsize;
    block->unused = totalsize - 
                    ( sizeof(MemBlock) + 
                      arsize * unitsize + 
                      arsize*2*sizeof(int) - 
                      sizeof(int) );
    block->size = arsize;
    block->start = 0;
    block->unitsize = unitsize;
    block->qlen = 0;

    for (i = 0; i < arsize; i++) {
        block->arr[i] = i;
    }

    for (i = arsize; i < 2*arsize; i++) {
        block->arr[i] = -1;
    }

    block->storage = (uint8 *)psb +
                     sizeof(MemBlock) + 
                     (2 * arsize - 1) * sizeof(int);

    return 0;
}

int mem_block_total (void * psb)
{
    MemBlock * block = (MemBlock *)psb;

    if (!block) return 0;

    return block->size;
}

int mem_block_restnum (void * psb)
{
    MemBlock * block = (MemBlock *)psb;

    if (!block) return 0;

    return block->size - block->start;
}

void * mem_block_alloc (void * psb)
{
    MemBlock * block = NULL;
    int         ind = 0;
    uint8     * pmem = NULL;

    if (!psb)
        return NULL;

    block = (MemBlock *)psb;
    if (block->start >= block->size) 
        return NULL;

    ind = block->arr[block->start];
    block->arr[block->start++] = -1;

    pmem = block->storage + ind * block->unitsize;
    memset(pmem, 0, block->unitsize);

    mem_block_unit_append(block, ind);

    return pmem;
}

int mem_block_free (void * psb, void * pmem)
{
    MemBlock * block = NULL;
    int         offset = 0;
    int         ind = 0;

    if (!psb) return -1;
    if (!pmem) return -2;

    block = (MemBlock *)psb;
    if (block->start <= 0) return -50;

    offset = (uint8 *)pmem - block->storage;
    if (offset < 0) return -100;

    if (offset % block->unitsize != 0) return -200;
    
    ind = offset / block->unitsize;
    if (ind >= block->size) return -300;

    mem_block_unit_remove(block, ind);

    //return mem_block_bsearch_recycle(block, ind);
    return mem_block_sort_recycle(block, ind);
}



static int mem_block_key_cmp_unit (const void * a, const void * b)
{
    int val = *(int *)a;
    int ubval = *(int *)b;

    if (val == ubval) return 0;
    if (val > ubval) return 1;
    return -1;
}

/* find if the key does exist, if exist, just return position. 
 * if not exist, insert the key to correct position, return it */
int mem_block_bsearch_recycle (void * psb, int memind)
{
    MemBlock  * block = (MemBlock *)psb;
    uint8     * pbgn  = NULL;
    int         width = sizeof(int);
    uint8     * lo    = NULL;
    uint8     * hi    = NULL;
    uint8     * mid   = NULL;
    uint8     * insp  = NULL;
    int         insind = 0;
    int         num   = 0;
    int         half  = 0;
    int         result = 0;

    if (!block)
        return -1;

    if (block->start == block->size) {
        block->arr[--block->start] = memind;
        return 0;
    }

    pbgn = (uint8 *)&block->arr[block->start];

    lo = (uint8 *)&block->arr[block->start];
    hi = (uint8 *)&block->arr[block->size-1];
    num = block->size - block->start;

    while (lo <= hi) {

        if ((half = num / 2) > 0) {

            mid = lo + ((num & 1) ? half : (half - 1)) * width;

            if (!(result = mem_block_key_cmp_unit(&memind, mid)))
                return (mid - pbgn)/width;

            else if (result < 0) {
                hi = mid - width;
                num = num & 1 ? half : half-1;

                if (lo > hi) {
                    insp = lo;
                    break;
                }

            } else {
                lo = mid + width;
                num = half;

                if (lo > hi) {
                    insp = hi;
                    break;
                }
            }

        } else if (num > 0) {

            if (!(result = mem_block_key_cmp_unit(&memind, lo)))
                return (lo - pbgn) / width;

            if (result > 0)
                insp = lo + width;

            else
                insp = lo;

            break;

        } else {
            break;
        }
    }

    if (!insp)
        return -100;

    insind = (insp - pbgn)/width;

    if (insind < 0 || insind > num) 
        return -200;

    if (insind > 0) {
        memmove( &block->arr[block->start-1],
                 &block->arr[block->start],
                 insind * width);
    }

    block->start--;
    block->arr[block->start + insind] = memind;

    return insind;
}


static int mem_block_find_inserloc (void * pbgn, void * pend, int width, int insval)
{
    uint8    * lo = NULL;
    uint8    * hi = NULL;
    uint8    * mid = NULL;
    uint8    * insp = NULL;
    int        insind = 0;
    int        num = 0;
    int        half = 0;
    int        result = 0;

    if (!pbgn)
        return -1;

    if (!pend)
        return -2;

    if (width <= 0)
        return -3;

    num = ((uint8 *)pend - (uint8 *)pbgn)/width;
    if (num < 0)
        return -4;

    if (num == 0)
        return 0;

    lo = (uint8 *)pbgn;
    hi = (uint8 *)pend - width;

    while (lo <= hi) {

        if ((half = num / 2) > 0) {

            mid = lo + ((num & 1) ? half : (half - 1)) * width;

            if (!(result = mem_block_key_cmp_unit (&insval, mid)))
                return (mid - (uint8 *)pbgn) / width;

            else if (result < 0) {
                hi = mid - width;
                num = num & 1 ? half : half-1;

                if (lo > hi) {
                    insp = lo;
                    break;
                }

            } else {
                lo = mid + width;
                num = half;

                if (lo > hi) {
                    insp = hi;
                    break;
                }
            }

        } else if (num > 0) {

            if (!(result = mem_block_key_cmp_unit(&insval, lo)))
                return (lo - (uint8 *)pbgn) / width;

            if (result > 0)
                insp = lo + width;

            else
                insp = lo;

            break;

        } else {
            break;
        }
    }

    if (!insp)
        return -100;

    insind = (insp - (uint8 *)pbgn) / width;

    if (insind < 0 || insind > num) 
        return -200;

    return insind;
}


int mem_block_sort_recycle (void * psb, int memind)
{
    MemBlock * block = (MemBlock *)psb;
    int        insind = -1;
    int        width = sizeof(int);

    if (!block)
        return -1;

    if (block->start == block->size) {
        block->arr[--block->start] = memind;
        return 0;
    }

    insind = mem_block_find_inserloc( &block->arr[block->start],
                                      &block->arr[block->size],
                                      width, memind);
    if (insind < 0)
        return insind;

    if (insind > 0)
        memmove(&block->arr[block->start-1],
                &block->arr[block->start],
                insind * width);

    block->start--;
    block->arr[block->start + insind] = memind;

    return insind;
}


int mem_block_unit_append (void * psb, int memind)
{
    MemBlock * block = (MemBlock *)psb;
    int         insind = -1;
    int         width = sizeof(int);

    if (!block) return -1;

    if (block->qlen == 0) {
        block->arr[block->qlen++] = memind;
        return 0;
    }

    insind = mem_block_find_inserloc(&block->arr[block->size],
                                     &block->arr[block->size+block->qlen],
                                     width, memind);
    if (insind < 0)
         return insind;

    if (insind > block->qlen)
        insind = block->qlen; 

    if (insind < block->qlen)
        memmove(&block->arr[block->size+insind+1],
                &block->arr[block->size+insind],
                (block->qlen-insind) * width);

    block->qlen++;
    block->arr[block->size + insind] = memind;

    return insind;
}


int mem_block_unit_remove (void * psb, int memind)
{
    MemBlock * block = (MemBlock *)psb;
    uint8    * insp = NULL;
    uint8    * pbgn = NULL;
    int        ind;

    if (!block) return -1;
    if (memind < 0 || memind >= block->size) return -100;

    if (block->qlen <= 0) return -200;

    pbgn = (uint8 *)&block->arr[block->size];

    insp = bsearch(&memind, pbgn, block->qlen,
                   sizeof(int), mem_block_key_cmp_unit);
    if (!insp)
        return 0;

    ind = (insp - pbgn)/sizeof(int);
    if (ind < 0 || ind >= block->qlen)
        return -300;

    memmove(&block->arr[block->size+ind],
            &block->arr[block->size+ind+1],
            (block->qlen-ind-1)*sizeof(int));

    block->arr[block->qlen-1] = -1;
    block->qlen -= 1;

    return 0;
}


int mem_block_unit_num (void * psb)
{
    MemBlock * block = (MemBlock *)psb;
    if (!block) return -1;

    return block->qlen;
}

void * mem_block_unit_get (void * psb, int qind)
{
    MemBlock * block = (MemBlock *)psb;
    int        memind = 0;
    uint8    * pmem = NULL;
    
    if (!block)
        return NULL;

    if (qind < 0 || qind >= block->qlen)
        return NULL;

    memind = block->arr[block->size + qind];
    if (memind < 0 || memind >= block->size)
        return NULL;

    pmem = block->storage + memind * block->unitsize;

    return pmem;
}


void mem_block_print (void * psb, FILE * fp)
{
    MemBlock * block = (MemBlock *)psb;
    int         i, line, totalline, ind;
    char        buf[128];
    char        tmpb[32], tmpc[16];
    int         CHonLine = 100;
    int         BGN = 4;
    int         WIDTH = 6;

    if (!block) return;
    if (!fp) fp = stdout;

    fprintf(fp, "\n____________________________________________Memory Block____________________________________________\n");
    fprintf(fp, "Control Header Basic:\n");
    fprintf(fp, "    Total Size      : %d bytes\n", block->totalsize);
    fprintf(fp, "    Unused Size     : %d bytes\n", block->unused);
    fprintf(fp, "    Header Size     : %d bytes\n", 
             (int)(sizeof(*block)-sizeof(int)+2*block->size*sizeof(int)));
    fprintf(fp, "    MemUnit Size    : %d bytes\n", block->unitsize);
    fprintf(fp, "    MemUnit Number  : %d\n", block->size);
    fprintf(fp, "    MemUnit RestNum : %d\n", block->size-block->start);
    fprintf(fp, "    MemUnit AllocNum: %d\n", block->qlen);

    fprintf(fp, "Unallocated Memory Unit Map:\n");
    memset(buf, 0, sizeof(buf));

    memset(buf, ' ', CHonLine); 
    for (i=0; i<15; i++) {
        if (i<10) sprintf(tmpb, "     %d", i);
        else sprintf(tmpb, "    %d", i);
        memcpy(&buf[BGN+i*WIDTH], tmpb, WIDTH);
    }
    fprintf(fp, "%s\n", buf);

    totalline = (block->size + 15)/16;
    for (line=0; line<totalline; line++) {
        memset(buf, ' ', CHonLine); 
        sprintf(tmpb, "%04d", line);
        memcpy(buf, tmpb, 4);
        for (i=0; i<15; i++) {
            ind = 16 * line + i;
            if (ind >= block->size) break;
            sprintf(tmpb, "%d", block->arr[ind]);
            memset(tmpc, 0, sizeof(tmpc)); memset(tmpc, ' ', WIDTH);
            memcpy(tmpc+WIDTH-strlen(tmpb), tmpb, strlen(tmpb));
            memcpy(&buf[BGN+i*WIDTH], tmpc, WIDTH);
        }
        fprintf(fp, "%s\n", buf);
        if (ind >= block->size) break;
    }

    fprintf(fp, "Allocated Memory Unit Map:\n");
    memset(buf, 0, sizeof(buf));

    memset(buf, ' ', CHonLine); 
    for (i=0; i<15; i++) {
        if (i<10) sprintf(tmpb, "     %d", i);
        else sprintf(tmpb, "    %d", i);
        memcpy(&buf[BGN+i*WIDTH], tmpb, WIDTH);
    }
    fprintf(fp, "%s\n", buf);

    totalline = (block->size + 15)/16;
    for (line=0; line<totalline; line++) {
        memset(buf, ' ', CHonLine); 
        sprintf(tmpb, "%04d", line);
        memcpy(buf, tmpb, 4);
        for (i=0; i<15; i++) {
            ind = 16 * line + i;
            if (ind >= block->size) break;
            sprintf(tmpb, "%d", block->arr[block->size+ind]);
            memset(tmpc, 0, sizeof(tmpc)); memset(tmpc, ' ', WIDTH);
            memcpy(tmpc+WIDTH-strlen(tmpb), tmpb, strlen(tmpb));
            memcpy(&buf[BGN+i*WIDTH], tmpc, WIDTH);
        }
        fprintf(fp, "%s\n", buf);
        if (ind >= block->size) break;
    }

    fprintf(fp, "\n----------------------------------------------------------------------------------------------------\n");
}
