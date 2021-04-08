/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 *
 * memory block management for persistent storage by 
 * inter-process shared memory 
 */

#ifndef _MEM_BLOCK_H_
#define _MEM_BLOCK_H_

#include "btype.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialize a big memory block for allocation of fixed-size memory unit
 *
 * Memory block is divided into 2 parts: 
 *
 * The first one is header information of memory unit managment,
 * including location of unit, unit allocation status, unallocated
 * memory unit array, allocated memory unit array, etc.
 *
 * These two arrays store the index of memory unit as ascending order.
 * 
 * The second part is the area of memory units allocated for storage.
 *
 * When a memory block initialized, calculate total unit number and
 * the size of header space, construct the 2 arrays: the unallocated 
 * memory unit array and allocated memory unit array.
 *
 * Note: all memory unit have the fixed size.
 */

int mem_block_init (void * psb, int totalsize, int unitsize);

/* return total number of memory unit in this block.*/
int mem_block_total (void * psb);

/* return available number of memory unit rested in this block. */
int mem_block_restnum (void * psb);

/* allocate one memory unit from the available memory unit list.
 * if all units are exhausted, return NULL. otherwise, take the first
 * unit away, memset the memory unit to zero.
 * before return the available memory unit, insert the index value of memory unit into 
 * allocated unit array with the ascending order. 
 */
void * mem_block_alloc (void * psb);

/* recycle the memory unit into the unallocated unit array.
 * check the memory pointer if it is in the range within the block, and if it is the 
 * memory unit margin location. if not, return error.
 * 
 * remove the index value of the unit from the allocated unit array, and insert the index value
 * into unallocated memory unit array with the ascending order.
 */
int mem_block_free (void * psb, void * pmem);

/* find if the unit index does exist in the unallocated unit array, if exist, just return position. 
 * if not exist, insert the index to correct position, return the position */
int mem_block_bsearch_recycle (void * psb, int memind);

/* similar implementation as above api */
int mem_block_sort_recycle (void * psb, int memind);

/* find if the unit index does exist in the allocated unit array, if exist, just return position. 
 * if not exist, insert the index to correct position, return the position */
int mem_block_unit_append (void * psb, int memind);

/* find the unit in the allocated unit array with binary search. if found, just remove it */
int mem_block_unit_remove (void * psb, int memind);

/* return the actual number of allocated memory unit */
int mem_block_unit_num (void * psb);

/* acquire the memory unit pointer from the allocated unit array, using array index. */
void * mem_block_unit_get (void * psb, int qind);

/* print the detailed profile of the memory block */
void mem_block_print (void * psb, FILE * fp);

#ifdef __cplusplus
}
#endif

#endif

