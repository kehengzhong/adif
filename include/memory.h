/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#ifndef _MEMORY_H_
#define _MEMORY_H_

#ifdef __cplusplus
extern "C" {
#endif

void kmem_print();

void * kosmalloc  (size_t size);
void * kosrealloc (void * ptr, size_t size);
void   kosfree    (void * ptr);

void * kalloc_dbg   (size_t size, char * file, int line);
void * kzalloc_dbg  (size_t size, char * file, int line);
void * krealloc_dbg (void * ptr, size_t size, char * file, int line);
void   kfree_dbg    (void * ptr, char * file, int line);

#define kalloc(size)        kalloc_dbg((size), __FILE__, __LINE__)
#define kzalloc(size)       kzalloc_dbg((size), __FILE__, __LINE__)
#define krealloc(ptr, size) krealloc_dbg((ptr), (size), __FILE__, __LINE__)
#define kfree(ptr)          kfree_dbg((ptr), __FILE__, __LINE__)


void * mem_unit_init      (void * psb, size_t totalsize);
void   mem_unit_reset     (void * vpsb);
size_t mem_unit_shrinkto  (void * punit, size_t newsize);

void * mem_unit_alloc     (void * punit, size_t size);
void * mem_unit_realloc   (void * punit, void * pmemp, size_t size);
int    mem_unit_free      (void * punit, void * pmem);

int    mem_unit_scan      (void * punit);

long   mem_unit_size      (void * punit, void * pmemp);
void * mem_unit_by_index  (void * punit, int ind);

void * mem_unit_availp    (void * punit);
void * mem_unit_endp      (void * punit);

size_t mem_unit_totalsize (void * punit);
size_t mem_unit_availsize (void * punit);
size_t mem_unit_usedsize  (void * punit);

size_t mem_unit_allocsize (void * punit);
size_t mem_unit_restsize  (void * punit);

void   mem_unit_print     (FILE * fp, void * punit);

void * mupool_init    (size_t blksize, void * mpool);
void   mupool_clean   (void * mpool);
void   mupool_reset   (void * vpool);

void * mupool_alloc   (void * mpool, size_t num);
void * mupool_realloc (void * mpool, void * pmem, size_t size);
int    mupool_free    (void * mpool, void * pmem);

long   mupool_size    (void * vpool, void * pmem);
void * mupool_by_index (void * vpool, int index);
int    mupool_scan     (void * vpool);

void   mupool_print (FILE * fp, void * vpool);


#ifdef __cplusplus
}
#endif

#endif

