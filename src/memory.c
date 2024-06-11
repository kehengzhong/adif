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
#include "mthread.h"
#include "trace.h"
#include "kemalloc.h"

void * g_kmempool = NULL;
uint8  g_kmempool_init = 0;

#ifdef _MEMDBG
#include "dynarr.h"
#include "dlist.h"

CRITICAL_SECTION  kmemCS;
uint64            kmemid = 0;
dlist_t         * kmem_list = NULL;
char              kmem_init = 0;

uint16  mflag = 0xb05a;

typedef struct kmemhdr_ {
    void          * res[2];
    uint64          memid;
    uint32          size;
    uint16          kflag;
    int             line;
    char            file[32];
    uint16          reallocate;
} KmemHdr;


uint64 getmemid()
{
    uint64 id = 0;

    EnterCriticalSection(&kmemCS);
    id = kmemid++;
    LeaveCriticalSection(&kmemCS);

    return id;
}

int kmem_cmp_kmem_by_id (void * a, void * b)
{
    KmemHdr * mema = (KmemHdr *)a;
    KmemHdr * memb = (KmemHdr *)b;

    if (!a) return -1;
    if (!b) return 1;

    if (mema->memid > memb->memid) return 1;
    if (mema->memid < memb->memid) return -1;

    return 0;
}

int kmem_add (void * ptr)
{
    KmemHdr * hdr = (KmemHdr *)ptr;

    if (kmem_init == 0) {
        kmem_init = 1;

        InitializeCriticalSection(&kmemCS);
        kmem_list = lt_new();
        kmemid = 0;
    }

    if (!ptr) return -1;

    EnterCriticalSection(&kmemCS);
    lt_append(kmem_list, hdr);
    LeaveCriticalSection(&kmemCS);

    return 0;
}

int kmem_del (void * ptr)
{        
    KmemHdr * hdr = (KmemHdr *)ptr;

    if (!ptr) return -1;

    EnterCriticalSection(&kmemCS);
    lt_delete_ptr(kmem_list, hdr);
    LeaveCriticalSection(&kmemCS);

    return 0;
}        

void kmem_print ()
{
    time_t curt = time(0);
    FILE  * fp = NULL;
    int     i, num;
    char  file[32];
    KmemHdr * hdr = NULL;
    uint64    msize = 0;

    sprintf(file, "kmem-%lu.txt", curt);
    fp = fopen(file, "w");

    EnterCriticalSection(&kmemCS);

    num = lt_num(kmem_list);
    hdr = lt_first(kmem_list);

    for (i = 0; hdr && i < num; i++) {
        hdr = lt_get_next(hdr);
        if (!hdr) continue;

        if (hdr->kflag != mflag) {
            fprintf(fp, "%p, %uB, is corrupted\n", hdr, hdr->size);
            continue;
        }

        fprintf(fp, "%p, %uB, id=%llu, re=%u, line=%u, file=%s\n",
                hdr, hdr->size, hdr->memid,
                hdr->reallocate, hdr->line, hdr->file);

        msize += hdr->size;
    }

    fprintf(fp, "Total Number: %u    Total MemSize: %llu\n", num, msize);

    LeaveCriticalSection(&kmemCS);

    fclose(fp);
}

#else

void kmem_print ()
{
}

#endif

void kmem_alloc_init (size_t size)
{
    g_kmempool = kempool_alloc(size, 0);
    if (g_kmempool) g_kmempool_init = 1;

#ifdef _MEMDBG
    if (kmem_init == 0) {
        kmem_init = 1;
        InitializeCriticalSection(&kmemCS);
        kmem_list = lt_new();
        kmemid = 0;
    }
#endif
}

void kmem_alloc_free ()
{
    g_kmempool_init = 0;

#ifdef _MEMDBG
    if (kmem_init == 1) {
        kmem_init = 0;
        DeleteCriticalSection(&kmemCS);
        lt_free(kmem_list);
    }
#endif

    if (g_kmempool) {
        kempool_free(g_kmempool);
        g_kmempool = NULL;
    }
}


void * kosmalloc_dbg (size_t size, char * file, int line)
{
    void * ptr = NULL;

    if (size <= 0) return NULL;

#if defined(_WIN32) || defined(_WIN64)
    ptr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
#else
    ptr = malloc(size);
#endif

    return ptr;
}

void * koszmalloc_dbg (size_t size, char * file, int line)
{
    void * ptr = NULL;

    if (size <= 0) return NULL;

#if defined(_WIN32) || defined(_WIN64)
    ptr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
#else
    ptr = malloc(size);
#endif
    if (ptr) memset(ptr, 0, size);

    return ptr;
}

void * kosrealloc_dbg (void * ptr, size_t size, char * file, int line)
{
    void  * oldp = ptr;

#if defined(_WIN32) || defined(_WIN64)
    if (ptr != NULL)
        ptr = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ptr, size);
    else
        ptr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);

    if (ptr == NULL) {
        if (oldp) HeapFree(GetProcessHeap(), 0, oldp);
    }

#else
    ptr = realloc(ptr, size);

    if (ptr == NULL) {
        if (oldp) free(oldp);
    }
#endif
 
    return ptr;
}

void kosfree_dbg (void * ptr, char * file, int line)
{
#if defined(_WIN32) || defined(_WIN64)
    if (ptr) HeapFree(GetProcessHeap(), 0, ptr);
#else
    if (ptr) free(ptr);
#endif
}


void * kalloc_dbg (size_t size, char * file, int line)
{
    void * ptr = NULL;

    if (size > 128*1024)
        tolog(1, "kalloc: big memory allocation %ld from %s:%d\n", size, file, line);

#ifdef _MEMDBG
    KmemHdr * hdr = NULL;

    if (size <= 0) return NULL;

    if (g_kmempool_init)
        ptr = kem_alloc_dbg(g_kmempool, size + sizeof(KmemHdr), file, line);
    else
        ptr = kosmalloc(size + sizeof(KmemHdr));

    hdr = (KmemHdr *)ptr;
    memset(hdr, 0, sizeof(*hdr));
    hdr->size = size;
    hdr->memid = getmemid(); 
    hdr->kflag = mflag;
    hdr->line = line;
    strncpy(hdr->file, file, sizeof(hdr->file)-1);
    kmem_add(ptr); 

    return (uint8 *)ptr + sizeof(KmemHdr);

#else
    if (size <= 0) return NULL;

    if (g_kmempool_init)
        ptr = kem_alloc_dbg(g_kmempool, size, file, line);
    else
        ptr = kosmalloc(size);

    return ptr;
#endif
}


void * kzalloc_dbg (size_t size, char * file, int line)
{
    void * ptr = NULL;

    if (size > 128*1024)
        tolog(1, "kzalloc: big memory allocation %ld from %s:%d\n", size, file, line);

#ifdef _MEMDBG
    KmemHdr * hdr = NULL;

    if (size <= 0) return NULL;

    if (g_kmempool_init)
        ptr = kem_alloc_dbg(g_kmempool, size + sizeof(KmemHdr), file, line);
    else
        ptr = kosmalloc(size + sizeof(KmemHdr));

    if (ptr)
        memset(ptr, 0, size + sizeof(KmemHdr));

    hdr = (KmemHdr *)ptr;
    hdr->size = size;
    hdr->memid = getmemid();
    hdr->kflag = mflag;
    hdr->line = line;
    strncpy(hdr->file, file, sizeof(hdr->file)-1);
    kmem_add(ptr);

    return (uint8 *)ptr + sizeof(KmemHdr);

#else
    if (size <= 0) return NULL;

    if (g_kmempool_init)
        ptr = kem_alloc_dbg(g_kmempool, size, file, line);
    else
        ptr = kosmalloc(size);
    if (ptr) memset(ptr, 0, size);

    return ptr;
#endif
} 

void * krealloc_dbg(void * ptr, size_t size, char * file, int line)
{
    if (size > 1024*1024)
        tolog(1, "krealloc: big memory reallocation %ld from %s:%d\n", size, file, line);

#ifdef _MEMDBG
    KmemHdr * hdr = NULL;
    uint64    memid = 0;

    if (ptr != NULL) {
        hdr = (KmemHdr *)(ptr - sizeof(KmemHdr)); 
        memid = hdr->memid;
        kmem_del(hdr);

        if (hdr->kflag != mflag)
            tolog(1, "###Panic: %s:%d krealloc %ld bytes, old %p:%llu %u bytes alloc by %s:%d ruined\n",
                     file, line, size, ptr, memid, hdr->size, hdr->file, hdr->line);

        if (g_kmempool_init)
            ptr = kem_realloc_dbg(g_kmempool, (uint8 *)ptr - sizeof(KmemHdr), size + sizeof(KmemHdr), file, line);
        else
            ptr = kosrealloc((uint8 *)ptr - sizeof(KmemHdr), size + sizeof(KmemHdr));

    } else {
        if (g_kmempool_init)
            ptr = kem_alloc_dbg(g_kmempool, size + sizeof(KmemHdr), file, line);
        else
            ptr = kosmalloc(size + sizeof(KmemHdr));
    }
 
    if (ptr == NULL) {
        return NULL;
    }

    hdr = (KmemHdr *)ptr; 
    hdr->size = size;
    hdr->memid = memid > 0 ? memid : getmemid(); 
    hdr->kflag = mflag;  
    hdr->line = line;
    strncpy(hdr->file, file, sizeof(hdr->file)-1); 
    hdr->reallocate++;
    kmem_add(ptr); 

    return (uint8 *)ptr + sizeof(KmemHdr);

#else
    if (g_kmempool_init)
        return kem_realloc_dbg(g_kmempool, ptr, size, file, line);
    else
        return kosrealloc(ptr, size);
#endif
}


void kfree_dbg (void * ptr, char * file, int line)
{
    if (ptr) {
#ifdef _MEMDBG
        KmemHdr * hdr = (KmemHdr *)((uint8 *)ptr - sizeof(KmemHdr));
        kmem_del(hdr);
        if (hdr->kflag != mflag) {
            tolog(1, "###Panic: %s:%d kfree %p:%llu %u bytes alloc by %s:%d ruined\n", 
                  file, line, ptr, hdr->memid, hdr->size, hdr->file, hdr->line);
            return;
        }
        if (g_kmempool_init)
            kem_free_dbg(g_kmempool, hdr, file, line);
        else
            kosfree(hdr);

#else
        if (g_kmempool_init)
            kem_free_dbg(g_kmempool, ptr, file, line);
        else
            kosfree(ptr);
#endif
    }
}

void kfreeit (void * ptr)
{
    kfree(ptr);
}

