/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#include "btype.h"
#include "memory.h"
#include "mthread.h"
#include "dynarr.h"
#include "mpool.h"

#ifdef _MEMDBG

CRITICAL_SECTION  kmemCS;
uint64            kmemid = 0;
arr_t           * kmem_list = NULL;
char              kmem_init = 0;

uint16  mflag = 0xb05a;

#pragma pack(1)

typedef struct kmemhdr_ {
    uint64          memid;
    uint32          size;
    uint16          kflag;
    int             line;
    char            file[32];
    uint16          reallocate;
} KmemHdr;

#pragma pack()

uint64 getmemid() {
    uint64 id = 0;
    EnterCriticalSection(&kmemCS);
    id = kmemid++;
    LeaveCriticalSection(&kmemCS);
    return id;
}

int kmem_cmp_kmem_by_id (void * a, void * b) {
    KmemHdr * mema = (KmemHdr *)a;
    KmemHdr * memb = (KmemHdr *)b;

    if (!a) return -1;
    if (!b) return 1;

    if (mema->memid > memb->memid) return 1;
    if (mema->memid < memb->memid) return -1;
    return 0;
}
int kmem_add(void * ptr) {
    KmemHdr * hdr = (KmemHdr *)ptr;
    if (kmem_init == 0) {
        kmem_init = 1;
        InitializeCriticalSection(&kmemCS);
        kmem_list = arr_new(8*1024*1024);
        kmemid = 0;
    }
    if (!ptr) return -1;
    EnterCriticalSection(&kmemCS);
    arr_insert_by(kmem_list, hdr, kmem_cmp_kmem_by_id);
    LeaveCriticalSection(&kmemCS);
    return 0;
}
int kmem_del(void * ptr) {        
    KmemHdr * hdr = (KmemHdr *)ptr;
    if (!ptr) return -1;
    EnterCriticalSection(&kmemCS);
    arr_delete_by(kmem_list, hdr, kmem_cmp_kmem_by_id);
    LeaveCriticalSection(&kmemCS);
    return 0;
}        
void kmem_print() {
    time_t curt = time(0);
    FILE  * fp = NULL;
    int     i, num;
    char  file[32];
    KmemHdr * hdr = NULL;
    uint64    msize = 0;

    sprintf(file, "kmem-%lu.txt", curt);
    fp = fopen(file, "w");
    EnterCriticalSection(&kmemCS);
    num = arr_num(kmem_list);
    for (i = 0; i < num; i++) {
        hdr = arr_value(kmem_list, i);
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
void kmem_print() {
}
#endif


void * kosmalloc (size_t size)
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

void * kosrealloc (void * ptr, size_t size)
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

void kosfree (void * ptr)
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

#ifdef _MEMDBG
    KmemHdr * hdr = NULL;

    if (size <= 0) return NULL;

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

    ptr = kosmalloc(size);

    return ptr;
#endif
}


void * kzalloc_dbg (size_t size, char * file, int line)
{
    void * ptr = NULL;

#ifdef _MEMDBG
    KmemHdr * hdr = NULL;

    if (size <= 0) return NULL;

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

    ptr = kosmalloc(size);
    if (ptr) memset(ptr, 0, size);

    return ptr;
#endif
} 

void * krealloc_dbg(void * ptr, size_t size, char * file, int line)
{
#ifdef _MEMDBG
    KmemHdr * hdr = NULL;
    uint64    memid = 0;

    if (ptr != NULL) {
        hdr = (KmemHdr *)(ptr - sizeof(KmemHdr)); 
        memid = hdr->memid;
        kmem_del(hdr);

        if (hdr->kflag != mflag)
            tolog("###Panic: %s:%d krealloc %ld bytes, old %p:%llu %u bytes alloc by %s:%d ruined\n",
                     file, line, size, ptr, memid, hdr->size, hdr->file, hdr->line);

        ptr = kosrealloc((uint8 *)ptr - sizeof(KmemHdr), size + sizeof(KmemHdr));

    } else {
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
            tolog("###Panic: %s:%d kfree %p:%llu %u bytes alloc by %s:%d ruined\n", 
                  file, line, ptr, hdr->memid, hdr->size, hdr->file, hdr->line);
            return;
        }
        kosfree(hdr);

#else
        kosfree(ptr);
#endif
    }
}


#pragma pack(push ,1)

typedef struct mem_block_ {
    size_t   totalsize;
    size_t   availsize;
    size_t   allocunit;
    size_t   allocsize; 

    CRITICAL_SECTION memCS;

    size_t   redline;    
    uint8    leaked;       //indicate if leaked memory unit exist
    
    uint8  * pstart;
    size_t   cur;
    uint8    zerodot[1];
} MemBlk;

#pragma pack(pop)


void * mem_unit_init (void * psb, size_t totalsize)
{
    MemBlk  * mblk = NULL;

    if (!psb) return NULL;

    mblk = (MemBlk *)psb;

    mblk->totalsize = totalsize;
    mblk->availsize = totalsize - sizeof(MemBlk) + 1;
    mblk->allocunit = 0;
    mblk->allocsize = 0;

    InitializeCriticalSection(&mblk->memCS);

    mblk->redline = 0;
    mblk->leaked = 0;

    mblk->pstart = &mblk->zerodot[0];
    mblk->cur = 0;

    return mblk;
}

void mem_unit_reset (void * psb)
{
    MemBlk  * mblk = NULL;
 
    if (!psb) return;
 
    mblk = (MemBlk *)psb;
 
    EnterCriticalSection(&mblk->memCS);

    mblk->allocunit = 0;
    mblk->allocsize = 0;
 
    mblk->redline = 0;
    mblk->leaked = 0;
 
    mblk->pstart = &mblk->zerodot[0];
    mblk->cur = 0;

    LeaveCriticalSection(&mblk->memCS);
}

static uint32 getuint32 (uint8 * pbgn)
{
    uint32 val = 0;

    val = pbgn[0] & 0xFF;

    val <<= 8;
    val += (pbgn[1] & 0xFF);

    val <<= 8;
    val += (pbgn[2] & 0xFF);

    val <<= 8;
    val += (pbgn[3] & 0xFF);

    return val;
}

static void putuint32 (uint8 * pbyte, uint32 val)
{
    if (!pbyte)
        return;

    pbyte[0] = (uint8)(val >> 24);
    pbyte[1] = (uint8)(val >> 16);
    pbyte[2] = (uint8)(val >> 8);
    pbyte[3] = (uint8)val;
}

int mem_unit_scan (void * vmblk)
{
    MemBlk   * mblk = (MemBlk *)vmblk;
    int        leaknum = 0;
    int        allocunit = 0;
    size_t     allocsize = 0;
    size_t     maxidle = 0;
    size_t     unitlen = 0;
    uint32     flag = 0;
    uint32     tmpflag = 0;
    uint8    * piter = NULL;
 
    if (!mblk) return -1;
 
    EnterCriticalSection(&mblk->memCS);

    for ( piter = mblk->pstart;
          piter < mblk->pstart + mblk->cur; )
    {
        flag = getuint32(piter);
 
        /* the size of current idle memory unit */
        unitlen = flag & 0x7FFFFFFF;
 
        /* merge a series of contiguous unit blocks in idle state
           into current found idle block */
 
        while ((flag & 0x80000000) == 0 &&
               piter + 4 + unitlen < mblk->pstart + mblk->cur)
        {
 
            /* get the flag of next neighbour memory unit */
            tmpflag = getuint32(piter + 4 + unitlen);
 
            if (tmpflag & 0x80000000) //not idle, allocated
                break;
 
            /* accumulate the size of continuous idle memory unit */
            unitlen += 4 + (tmpflag & 0x7FFFFFFF);
 
            /* update the flag of merged memory units */
            putuint32(piter, (unitlen & 0x7FFFFFFF));
        }
 
        if ((flag & 0x80000000) == 0) {

            if (piter + 4 + unitlen == mblk->pstart + mblk->cur) {
                mblk->cur -= 4 + unitlen;
                break;
            }

            if (unitlen > maxidle)
                maxidle = unitlen;

            leaknum++;
        } else {
            allocsize += unitlen;
        }

        allocunit++;
        piter += 4 + unitlen;
    }
 
    if (leaknum > 0)
        mblk->leaked = 1;
    else
        mblk->leaked = 0;

    if (maxidle < mblk->availsize - mblk->cur)
        maxidle = mblk->availsize - mblk->cur;

    mblk->redline = maxidle + 1;
    mblk->allocunit = allocunit;
    mblk->allocsize = allocsize;

    LeaveCriticalSection(&mblk->memCS);

    return 0;
}

static void * mem_unit_prev (void * vmblk, void * pmem)
{
    MemBlk   * mblk = (MemBlk *)vmblk;
    size_t     unitlen = 0;
    uint32     flag = 0;
    uint32     tmpflag = 0;
    uint8    * piter = NULL;

    if (!mblk)
        return NULL;

    for ( piter = mblk->pstart;
          piter < mblk->pstart + mblk->cur; )
    {
        flag = getuint32(piter);
 
        /* the size of current idle memory unit */
        unitlen = flag & 0x7FFFFFFF;
 
        if (piter + 4 + unitlen == (uint8 *)pmem)
            return piter;

        /* merge a series of contiguous unit blocks in idle state 
           into current found idle block */
 
        while ((flag & 0x80000000) == 0 && 
               piter + 4 + unitlen < mblk->pstart + mblk->cur)
        {
 
            /* get the flag of next neighbour memory unit */
            tmpflag = getuint32(piter + 4 + unitlen);
 
            if (tmpflag & 0x80000000) //not idle, allocated
                break;
 
            /* accumulate the size of continuous idle memory unit */
            unitlen += 4 + (tmpflag & 0x7FFFFFFF);
 
            /* update the flag of merged memory units */
            putuint32(piter, (unitlen & 0x7FFFFFFF));
            mblk->allocunit--;

            if (piter + 4 + unitlen == (uint8 *)pmem)
                return piter;
        }

        piter += 4 + unitlen;
        if (piter > (uint8 *)pmem)
            break;
    }

    return NULL;
}

static void * mem_unit_next (void * vmblk, void * pmem)
{
    MemBlk   * mblk = (MemBlk *)vmblk;
    uint8    * piter = (uint8 *)pmem;
    size_t     unitlen = 0;
    uint32     flag = 0;
    uint32     tmpflag = 0;
 
    if (!mblk)
        return NULL;
 
    if (piter < mblk->pstart || piter >= mblk->pstart + mblk->cur)
        return NULL;

    /* the flag of current idle memory unit */
    flag = getuint32(piter);
    unitlen = flag & 0x7FFFFFFF;

    if (piter + 4 + unitlen >= mblk->pstart + mblk->cur)
        return NULL;

    /* skip to next memory unit and fetch next flag */
    piter += 4 + unitlen;

    flag = getuint32(piter);
    unitlen = flag & 0x7FFFFFFF;

    /* if the unit is in idle, merge subsequent contiguous unit
       blocks in idle state into the unit */
 
    while ((flag & 0x80000000) == 0 &&
           piter + 4 + unitlen < mblk->pstart + mblk->cur)
    {

        /* get the flag of next neighbour memory unit */
        tmpflag = getuint32(piter + 4 + unitlen);

        if (tmpflag & 0x80000000) //not idle, allocated
            break;

        /* accumulate the size of continuous idle memory unit */
        unitlen += 4 + (tmpflag & 0x7FFFFFFF);

        /* update the flag of merged memory units */
        putuint32(piter, (unitlen & 0x7FFFFFFF));
        mblk->allocunit--;
    }
 
    return piter;
}


static void * mem_unit_alloc_one (void * vmblk, size_t size)
{
    MemBlk   * mblk = (MemBlk *)vmblk;
    size_t     unitlen = 0;
    uint32     flag = 0;
    uint32     tmpflag = 0;
    uint8    * piter = NULL;
    int        leaknum = 0;

    if (!mblk || size <= 0)
        return NULL;

    /* firstly, search the idle freed hole memory units 
       via traversing the memory unit from starting location. */

    for ( piter = mblk->pstart;
          mblk->leaked && piter < mblk->pstart + mblk->cur; )
    {
        flag = getuint32(piter);

        /* the size of current idle memory unit */
        unitlen = flag & 0x7FFFFFFF;

        if (flag & 0x80000000) {
            /* the most significant bit indicates if the unit is allocated before.
             * the remaining 31 bits stores the size of the unit */

            piter += 4 + unitlen;
            continue;
        }

        /* merge a series of contiguous unit blocks in idle state
           into current found idle block */

        while (piter + 4 + unitlen < mblk->pstart + mblk->cur) {

            /* get the flag of next neighbour memory unit */
            tmpflag = getuint32(piter + 4 + unitlen);

            if (tmpflag & 0x80000000) //not idle, allocated
                break; 

            /* accumulate the size of continuous idle memory unit */
            unitlen += 4 + (tmpflag & 0x7FFFFFFF);
            mblk->allocunit--;

            /* update the flag of merged memory units */
            putuint32(piter, (unitlen & 0x7FFFFFFF));
        }
        
        /* found one idle memory unit. if the memory unit extends
           to unallocated memory location, set cur new value */

        if (piter + 4 + unitlen >= mblk->pstart + mblk->cur) {
            mblk->cur = piter - mblk->pstart;
            break;
        }

        if (size == unitlen) {

            /* requested size is just equal to current idle memory unit */

            flag = size;
            flag |= 0x80000000;
            putuint32(piter, flag);

            mblk->allocsize += size;

            return piter + 4;

        } else if (size > unitlen) {

            /* requested size is greater than current idle memory unit.
               skip to next memory unit */

            piter += 4 + unitlen;
            leaknum++;

            continue;

        } else if (size < unitlen) {

            /* requested size is smaller than current idle memory unit. */

            if (size + 4 >= unitlen) {

                /* requested size plus 4-bytes flag is greater than
                   the size of current idle memory unit. The rest space
                   with surplus 1-4 bytes of the idle memory block can not be 
                   separated into one available idle memory unit, and 
                   just attach surplus 1-4 bytes to the memory unit to be allocated. */

                flag = unitlen;
                flag |= 0x80000000;
                putuint32(piter, flag);

                mblk->allocsize += unitlen;

                return piter + 4;

            } else {
                /* allocate the memory unit with requested size */
                flag = size;
                flag |= 0x80000000;
                putuint32(piter, flag);

                /* set the flag of remaining space to idle */
                flag = unitlen - size - 4;
                flag &= 0x7FFFFFFF;
                putuint32(piter+4+size, flag);

                leaknum++;

                mblk->allocunit++;
                mblk->allocsize += size;

                return piter + 4;
            }
        }
    } //end of for

    if (leaknum > 0)
        mblk->leaked = 1;
    else
        mblk->leaked = 0;

    /* check the remaining space of available memory block */

    if (size + 4 <= mblk->availsize - mblk->cur) {
        piter = mblk->pstart + mblk->cur;
        flag = size;
        flag |= 0x80000000;
        putuint32(piter, flag);

        mblk->allocunit++;
        mblk->allocsize += size;
        mblk->cur += 4 + size;

        return piter + 4;
    }

    /* not enough memory for allocation */
    return NULL;
}

void * mem_unit_alloc (void * vmblk, size_t size)
{
    MemBlk   * mblk = (MemBlk *)vmblk;
    uint8    * pmem = NULL;

    if (!mblk || size <= 0) return NULL;

    EnterCriticalSection(&mblk->memCS);

    if (mblk->redline > 0 && size >= mblk->redline) {
        LeaveCriticalSection(&mblk->memCS);
        return NULL;
    }

    pmem = mem_unit_alloc_one(mblk, size);
    if (!pmem) {
        mblk->redline = size;
    }

    LeaveCriticalSection(&mblk->memCS);

    if (pmem)
        memset(pmem, 0, size);

    return pmem;
}

static void * mem_unit_realloc_one (void * vmblk, void * pmemp, size_t size)
{
    MemBlk   * mblk = (MemBlk *)vmblk;
    uint8    * pmem = (uint8 *)pmemp;
    uint8    * pnew = NULL;

    uint32     flag = 0;
    size_t     unitlen = 0;

    uint8    * pnext = NULL;
    uint32     nextflag = 0;
    size_t     nextlen = 0;

    uint8    * prev = NULL;
    uint32     prevflag = 0;
    size_t     prevlen = 0;

    size_t     dif = 0;

    if (!mblk) return NULL;

    if (!pmem)
        return mem_unit_alloc(mblk, size);

    if (pmem - 4 < mblk->pstart || pmem >= mblk->pstart + mblk->cur)
        return NULL;
 
    flag = getuint32(pmem - 4);
    unitlen = flag & 0x7FFFFFFF;

    if ((flag & 0x80000000) == 0)
        return mem_unit_alloc(mblk, size);

    if (size == unitlen)
        return pmem;

    /* if current memory unit is adjacent to the boundary of unallocated area */

    if (pmem + unitlen >= mblk->pstart + mblk->cur) {
        if (size < unitlen) {
            dif = unitlen - size;
            putuint32(pmem - 4, (size | 0x80000000));

            mblk->cur -= dif;
            mblk->allocsize -= dif;

            return pmem;
        }

        dif = size - unitlen;
        nextlen = mblk->availsize - mblk->cur;

        if (dif <= nextlen) {
            putuint32(pmem - 4, (size | 0x80000000));

            mblk->cur += dif;
            mblk->allocsize += dif;

            return pmem;
        }

        /* if dif > nextlen */

        goto check_prev_unit;
    }

    if (size < unitlen) {

        /* the reallocated size is smaller than the size of current 
           memory unit. check if the next neighbour memory unit is idle */

        dif = unitlen - size;

        pnext = mem_unit_next(mblk, pmem - 4);

        nextflag = getuint32(pnext);
        nextlen = nextflag & 0x7FFFFFFF;

        if (pnext && (nextflag & 0x80000000) == 0) {
            /* the next unit is idle, merge the surplus bytes into the idle unit */

            putuint32(pmem - 4, (size | 0x80000000));

            nextlen += dif;
            putuint32(pmem + size, (nextlen & 0x7FFFFFFF));

            mblk->allocsize -= dif;

            return pmem;
        }

        /* the next unit is in use, separate the surplus bytes
           and set as a idle memory unit */

        if (unitlen - size <= 4) {
            /* surplus bytes are not enough to form one idle memory unit */
            return pmem;
        }

        nextlen = dif - 4;
        putuint32(pmem + size, (nextlen & 0x7FFFFFFF));
        mblk->allocunit++;

        putuint32(pmem - 4, (size | 0x80000000));

        mblk->allocsize -= dif;
        mblk->leaked = 1;

        return pmem;
    }

    /* following is size > unitlen
     * the reallocated size if greater than the size of current memory unit */

    pnext = mem_unit_next(mblk, pmem - 4);
    if (!pnext)
        goto check_prev_unit;

    nextflag = getuint32(pnext);
    nextlen = nextflag & 0x7FFFFFFF;

    if ((nextflag & 0x80000000) == 0) {

        /* the next unit is idle, check if it has enough bytes for
           previous one's extending. */

        dif = size - unitlen;

        if (dif < nextlen) {
            /* next neighbour memory unit is big enough, 
               separate it into 2 parts */

            putuint32(pmem - 4, (size | 0x80000000));

            nextlen -= dif;
            putuint32(pmem + size, (nextlen & 0x7FFFFFFF));

            mblk->allocsize += dif;

            memset(pmem + unitlen, 0, dif);

            return pmem;
        }

        else if (dif <= nextlen + 4) {
            /* next neighbour memory unit has similar size, 
               completely merge it into previous unit */

            memset(pmem + unitlen, 0, nextlen + 4);

            unitlen += nextlen + 4;
            putuint32(pmem - 4, (unitlen | 0x80000000));
            mblk->allocunit--;

            mblk->allocsize += nextlen + 4;

            return pmem;
        } 

        /* if size - unitlen > nextlen + 4 and next unit is idle,  go to check_prev_unit */
    }

    /* if size > unitlen and next unit is not idle, goto check_prev_unit */

check_prev_unit:

    prev = mem_unit_prev(mblk, pmem - 4);

    if (prev) {
        prevflag = getuint32(prev);
        prevlen = prevflag & 0x7FFFFFFF;

        if ((prevflag & 0x80000000) == 0) {
            /* previous unit is idle state */

            if (prevlen + unitlen + 4 < size) {
                if (nextflag == 0) {
                    /* adjacent to boundary of unallocated area */

                    if (prevlen + unitlen + 4 + nextlen >= size) {

                        pnew = prev + 4;
                        memmove(pnew, pmem, unitlen);

                        putuint32(prev, size | 0x80000000);
                        mblk->allocunit--;

                        dif = size - (prevlen + unitlen + 4);
                        mblk->cur += dif;

                        mblk->allocsize += size - unitlen;

                        return pnew;
                    }
                    /* else, prevlen + unitlen + 4 + nextlen < size, goto alloc_new_unit */
                }

                else if ((nextflag & 0x80000000) == 0) {
                    /* next unit is idle. if the size of 3 continuous 
                       units is greater than or equal to realloc size */

                    if (prevlen + unitlen + 4 + nextlen + 4 >= size) {
                        pnew = prev + 4;
                        memmove(pnew, pmem, unitlen);

                        dif = prevlen + unitlen + 4 + nextlen + 4 - size;
                        if (dif <= 4) { //3 contiguous units merged into one
                            size += dif;
                            putuint32(prev, size | 0x80000000);
                            mblk->allocunit -= 2;

                            mblk->allocsize += size - unitlen;
                        } else { // 3 contiguous units merged into 2
                            putuint32(prev, size | 0x80000000);
                            mblk->allocsize += size - unitlen;

                            nextlen = dif - 4;
                            putuint32(prev + 4 + size, nextlen & 0x7FFFFFFF);
                            mblk->allocunit--;
                        }

                        return pnew;
                    }
                }
                /* else: next unit is not idle and the size of prev unit
                   plus current unit is smaller than the reallocated size */
            }

            else if (prevlen + unitlen + 4 <= size + 4) {
                /* size <= prevlen + unitlen + 4 <= size + 4
                   merge the previous unit and current unit into one.
                   the total space of previous unit plus current unit is 
                   used as new reallocated unit */

                size = prevlen + unitlen + 4;
                putuint32(prev, size | 0x80000000);
                mblk->allocunit--;

                mblk->allocsize += size - unitlen;
                
                pnew = prev + 4;
                memmove(pnew, pmem, unitlen);

                return pnew;
            }

            else { //if (prevlen + unitlen + 4 > size + 4) {
                putuint32(prev, size | 0x80000000);
                mblk->allocsize += size - unitlen;

                dif = prevlen + unitlen + 4 - size;
                nextlen = dif - 4;
                putuint32(prev + 4 + size, nextlen & 0x7FFFFFFF);

                pnew = prev + 4;
                memmove(pnew, pmem, unitlen);

                return pnew;
            }
        }
        /* previous unit is not idle, no extra space allocated */
    }

    /* up to now, two-sides of the unit does not exist extra space to hold new size.
       need to find a new memory unit to hold the requested size.
       set the current unit to idle state */

    pnew = mem_unit_alloc_one(mblk, size);
    if (pnew) {
        mblk->allocsize += size - unitlen;
        putuint32(pmem - 4, (flag & 0x7FFFFFFF));

        memmove(pnew, pmem, unitlen);
    }

    return pnew;
}

void * mem_unit_realloc (void * vmblk, void * pmemp, size_t size)
{
    MemBlk   * mblk = (MemBlk *)vmblk;
    void     * pnew = NULL;

    if (!mblk) return NULL;

    EnterCriticalSection(&mblk->memCS);
    pnew = mem_unit_realloc_one(mblk, pmemp, size);
    LeaveCriticalSection(&mblk->memCS);

    return pnew;
}

int mem_unit_free (void * vmblk, void * pmemp)
{
    MemBlk   * mblk = (MemBlk *)vmblk;
    uint8    * pmem = (uint8 *)pmemp;
    uint8    * pbgn = NULL;
    uint8    * piter = NULL;
    uint8    * pend = NULL;
    uint32     flag = 0;
    uint32     tmpflag = 0;
    size_t     unitlen = 0;
    uint8      released = 0;
    size_t     thissize = 0;

    if (!mblk)
        return -1;

    if (!pmem)
        return -2;

    pbgn = mblk->pstart;
    pend = mblk->pstart + mblk->cur;

    /* each memory unit has a 4-bytes preceding header */
    if (pmem - 4 < pbgn || pmem >= pend)
        return -100;

    EnterCriticalSection(&mblk->memCS);

    flag = getuint32(pmem - 4);
    thissize = unitlen = flag & 0x7FFFFFFF;
    if ((flag & 0x80000000) == 0) {
        /* the memory unit has been released before */
        released = 1;
    }

    while (pmem + unitlen < mblk->pstart + mblk->cur) {

        /* check if the next neighbour unit is idle. merge the 
           continuous idle memory unit inot one */

        tmpflag = getuint32(pmem + unitlen);
        if (tmpflag & 0x80000000)
            break; 

        unitlen += 4 + (tmpflag & 0x7FFFFFFF);
        putuint32(pmem - 4, (unitlen & 0x7FFFFFFF));
        mblk->allocunit--;
    }

    if (released) {
        LeaveCriticalSection(&mblk->memCS);
        return 0;
    }
        
    flag = getuint32(pmem-4);
    unitlen = flag & 0x7FFFFFFF;

    piter = pmem + unitlen;
    if (piter > pend)
        return -110;

    if (piter == pend) {
        putuint32(pmem - 4, 0);
        mblk->allocunit--;
        mblk->allocsize -= thissize;
        mblk->cur = pmem - 4 - mblk->pstart;

        mblk->redline = 0;

        LeaveCriticalSection(&mblk->memCS);

        return 1;
    }

    flag &= 0x7FFFFFFF;
    putuint32(pmem-4, flag);

    mblk->allocsize -= thissize;

    mblk->leaked = 1;

    mblk->redline = 0;

    LeaveCriticalSection(&mblk->memCS);

    return 1;
}

long mem_unit_size (void * vmblk, void * pmemp)
{
    MemBlk   * mblk = (MemBlk *)vmblk;
    uint8    * pmem = (uint8 *)pmemp;
    uint32     flag = 0;

    if (!mblk) return -1;
 
    if (!pmem) return -2;
 
    /* each memory unit has a 4-bytes preceding header */

    if (pmem - 4 < mblk->pstart || pmem >= mblk->pstart + mblk->cur)
        return -100;
 
    flag = getuint32(pmem - 4);

    return flag & 0x7FFFFFFF;
}

void * mem_unit_by_index (void * vmblk, int ind)
{
    MemBlk   * mblk = (MemBlk *)vmblk;
    uint8    * piter = NULL;
    uint8    * pend = NULL;
    uint32     flag = 0;
    uint32     unitlen = 0;
    int        i;

    if (!mblk) return NULL;

    pend = mblk->pstart + mblk->cur;
    piter = mblk->pstart;

    for (i = 0; piter < pend; i++) {
        flag = getuint32(piter);
        unitlen = flag & 0x7FFFFFFF;

        if (i == ind) return piter + 4;

        piter += 4 + unitlen;
    }

    return NULL;
}


void mem_unit_print (FILE * fp, void * vmblk)
{
    MemBlk  * mblk = (MemBlk *)vmblk;
    uint8   * piter = NULL;
    uint8   * pend = NULL;
    uint32    flag = 0;
    size_t    unitlen = 0;
    int       i;

    if (!mblk) return;

    if (!fp) fp = stdout;

    pend = mblk->pstart + mblk->cur;
    piter = mblk->pstart;

    fprintf(fp, "\nMemory Block Pointer: %p   Unalloc Pointer: %p\n", mblk, pend);
    fprintf(fp, "  total unitsize: %-6ld", mblk->totalsize);
    fprintf(fp, "  avail unitsize: %-6ld", mblk->availsize);
    fprintf(fp, "  unalloc curpos: %-6ld", mblk->cur);
    fprintf(fp, "  unalloc size  : %-6ld\n", mblk->availsize - mblk->cur);
    fprintf(fp, "  allocated unit: %-6ld", mblk->allocunit);
    fprintf(fp, "  allocated size: %-6ld", mblk->allocsize);
    fprintf(fp, "  red line value: %-6ld", mblk->redline);
    fprintf(fp, "  have leak hole: %d\n", mblk->leaked);

    for (i = 0; piter < pend; i++) {
        flag = getuint32(piter);
        unitlen = flag & 0x7FFFFFFF;

        fprintf(fp, "    unit:%3d  %p  size:%5ld \t", i, piter+4, unitlen);

        if (flag & 0x80000000)
            fprintf(fp, "[allocated]\n");
        else
            fprintf(fp, "[         ]\n");

        piter += 4+unitlen;
    }

    fprintf(fp, "\n");
}


void * mem_unit_availp (void * vmblk)
{
    MemBlk * mblk = (MemBlk *)vmblk;

    if (!mblk) return NULL;

    return mblk->pstart + mblk->cur;
}

void * mem_unit_endp (void * vmblk)
{
    MemBlk * mblk = (MemBlk *)vmblk;

    if (!mblk) return NULL;

    return mblk->pstart + mblk->availsize;
}

size_t mem_unit_totalsize (void * vmblk)
{
    MemBlk * mblk = (MemBlk *)vmblk;

    if (!mblk) return 0;

    return mblk->totalsize;
}

size_t mem_unit_availsize (void * vmblk)
{
    MemBlk * mblk = (MemBlk *)vmblk;

    if (!mblk) return 0;

    return mblk->availsize - mblk->cur;
}

size_t mem_unit_usedsize (void * vmblk)
{
    MemBlk * mblk = (MemBlk *)vmblk;

    if (!mblk) return 0;

    return sizeof(MemBlk) - 1 + mblk->cur;
}

size_t mem_unit_allocsize (void * vmblk)
{
    MemBlk * mblk = (MemBlk *)vmblk;

    if (!mblk) return 0;

    return mblk->allocunit * 4 + mblk->allocsize;
}

size_t mem_unit_restsize (void * vmblk)
{
    MemBlk * mblk = (MemBlk *)vmblk;

    if (!mblk) return 0;

    return mblk->availsize - mblk->allocunit * 4 - mblk->allocsize;
}

size_t mem_unit_shrinkto (void * vmblk, size_t newsize)
{
    MemBlk   * mblk = (MemBlk *)vmblk;
    size_t     availsize = 0;
    size_t     difval = 0;

    if (!mblk)
        return 0;

    if (newsize < 0)
        return 0;

    availsize = mblk->availsize - mblk->cur;
    if (newsize > availsize)
        newsize = availsize;

    difval = availsize - newsize;
    mblk->totalsize -= difval;
    mblk->availsize -= difval;

    return mblk->availsize - mblk->cur;
}


typedef struct MemBlkPool_ {
    size_t             blksize;
    arr_t            * mem_blk_list;
    void             * blk_pool;
    CRITICAL_SECTION   blklistCS;
} mupool_t, *mupool_p;
 

int mem_blk_cmp_mem_blk (void * a, void * b)
{
    if (a > b) return 1;
    if (a < b) return -1;
    return 0;
}

int mem_blk_cmp_pmem (void * a, void * b)
{
    MemBlk * mblk = (MemBlk *)a;

    if (a > b) return 1;

    if (b >= a && (uint8 *)b < (uint8 *)a + mblk->totalsize)
        return 0;

    return -1;
}


void * mupool_init (size_t blksize, void * mpool)
{
    mupool_t * pool = NULL;

    pool = kzalloc(sizeof(*pool));
    if (!pool) return NULL;

    pool->blksize = blksize;
    pool->mem_blk_list = arr_new(4);

    pool->blk_pool = mpool;
    if (mpool)
        pool->blksize = mpool_unitsize(pool->blk_pool);

    InitializeCriticalSection(&pool->blklistCS);

    return pool;
}

void mupool_clean (void * vpool)
{
    mupool_t * pool = (mupool_t *)vpool;
    void     * mblk = NULL;
    int        i, num;
 
    if (!pool) return;
 
    EnterCriticalSection(&pool->blklistCS);
 
    num = arr_num(pool->mem_blk_list);
 
    for (i = 0; i < num; i++) {
        mblk = arr_value(pool->mem_blk_list, i);
 
        if (pool->blk_pool)
            mpool_recycle(pool->blk_pool, mblk);
        else
            kfree(mblk);
    }
 
    LeaveCriticalSection(&pool->blklistCS);
 
    arr_free(pool->mem_blk_list);

    DeleteCriticalSection(&pool->blklistCS);
 
    kfree(pool);
}
 
void mupool_reset (void * vpool)
{
    mupool_t * pool = (mupool_t *)vpool;
    void     * mblk = NULL;
    int        i, num;
 
    if (!pool) return;
 
    EnterCriticalSection(&pool->blklistCS);
 
    num = arr_num(pool->mem_blk_list);
 
    for (i = 0; i < num; i++) {
        mblk = arr_value(pool->mem_blk_list, i);
 
        if (pool->blk_pool)
            mpool_recycle(pool->blk_pool, mblk);
        else
            mem_unit_reset(mblk);
    }

    LeaveCriticalSection(&pool->blklistCS);
}


void * mupool_alloc (void * vpool, size_t size)
{
    mupool_t * pool = (mupool_t *)vpool;
    void     * mblk = NULL;
    void     * pmem = NULL;
    int        i, num;

    if (!pool) return NULL;

    EnterCriticalSection(&pool->blklistCS);

    num = arr_num(pool->mem_blk_list);

    for (i = 0; i < num; i++) {
        mblk = arr_value(pool->mem_blk_list, i);

        pmem = mem_unit_alloc(mblk, size);
        if (pmem) {
            LeaveCriticalSection(&pool->blklistCS);
            return pmem;
        }
    }

    if (pool->blk_pool)
        mblk = mpool_fetch(pool->blk_pool);
    else
        mblk = kalloc(pool->blksize);
 
    if (!mblk) {
        LeaveCriticalSection(&pool->blklistCS);
        return NULL;
    }

    mem_unit_init(mblk, pool->blksize);
    arr_insert_by(pool->mem_blk_list, mblk, mem_blk_cmp_mem_blk);

    LeaveCriticalSection(&pool->blklistCS);

    pmem = mem_unit_alloc(mblk, size);

    return pmem;
}

void * mupool_realloc (void * vpool, void * pmem, size_t size)
{
    mupool_t * pool = (mupool_t *)vpool;
    void     * mblk = NULL;
    void     * iblk = NULL;
    void     * remem = NULL;
    int        i, num;
 
    if (!pool) return NULL;
 
    EnterCriticalSection(&pool->blklistCS);
 
    mblk = arr_find_by(pool->mem_blk_list, pmem, mem_blk_cmp_pmem);
    if (!mblk) {
        LeaveCriticalSection(&pool->blklistCS);
        return NULL;
    }

    remem = mem_unit_realloc(mblk, pmem, size);
    if (remem) {
        LeaveCriticalSection(&pool->blklistCS);
        return remem;
    }

    /* now allocate space from other memory block */
    num = arr_num(pool->mem_blk_list);
 
    for (i = 0; i < num; i++) {
        iblk = arr_value(pool->mem_blk_list, i);
        if (iblk == mblk)
            continue;
 
        remem = mem_unit_alloc(iblk, size);
        if (remem) {

            LeaveCriticalSection(&pool->blklistCS);

            /* copy the content of old memory to new allocated memory */
            memmove(remem, pmem, mem_unit_size(mblk, pmem));

            /* free old memory unit */
            mem_unit_free(mblk, pmem);

            return remem;
        }
    }
 
    if (i >= num) { //not found in list
        if (pool->blk_pool)
            iblk = mpool_fetch(pool->blk_pool);
        else
            iblk = kalloc(pool->blksize);

        if (!iblk) {
            LeaveCriticalSection(&pool->blklistCS);
            return NULL;
        }
 
        mem_unit_init(iblk, pool->blksize);
        arr_insert_by(pool->mem_blk_list, iblk, mem_blk_cmp_mem_blk);
    }
 
    LeaveCriticalSection(&pool->blklistCS);

    remem = mem_unit_alloc(iblk, size);
    if (remem) {
        /* copy the content of old memory to new allocated memory */
        memmove(remem, pmem, mem_unit_size(mblk, pmem));

        /* free old memory unit */
        mem_unit_free(mblk, pmem);
    }
 
    return remem;
}

int mupool_free (void * vpool, void * pmem)
{
    mupool_t * pool = (mupool_t *)vpool;
    void     * mblk = NULL;

    if (!pool) return -1;
 
    EnterCriticalSection(&pool->blklistCS);
 
    mblk = arr_find_by(pool->mem_blk_list, pmem, mem_blk_cmp_pmem);

    LeaveCriticalSection(&pool->blklistCS);

    if (!mblk) return -100;

    return mem_unit_free(mblk, pmem);
}

long mupool_size (void * vpool, void * pmem)
{
    mupool_t * pool = (mupool_t *)vpool;
    void     * mblk = NULL;
 
    if (!pool) return -1;
 
    EnterCriticalSection(&pool->blklistCS);
 
    mblk = arr_find_by(pool->mem_blk_list, pmem, mem_blk_cmp_pmem);
 
    LeaveCriticalSection(&pool->blklistCS);
 
    if (!mblk) return -100;
 
    return mem_unit_size(mblk, pmem);
}

void * mupool_by_index (void * vpool, int index)
{
    mupool_t * pool = (mupool_t *)vpool;
    MemBlk   * iblk = NULL;
    int        i, num;
    int        unitnum = 0;

    if (!pool || index < 0) return NULL;

    EnterCriticalSection(&pool->blklistCS);

    num = arr_num(pool->mem_blk_list);

    for (i = 0; i < num; i++) {
        iblk = arr_value(pool->mem_blk_list, i);
        if (!iblk) continue;

        if (index < unitnum + (int)iblk->allocunit) {
            return mem_unit_by_index(iblk, index - unitnum);
        }

        unitnum += iblk->allocunit;
    }

    LeaveCriticalSection(&pool->blklistCS);

    return NULL;
}

int mupool_scan (void * vpool)
{ 
    mupool_t * pool = (mupool_t *)vpool;
    MemBlk   * iblk = NULL; 
    int        i, num; 
 
    if (!pool) return -1; 
 
    EnterCriticalSection(&pool->blklistCS);
 
    num = arr_num(pool->mem_blk_list); 
 
    for (i = 0; i < num; i++) {  
        iblk = arr_value(pool->mem_blk_list, i);
        if (!iblk) continue;
 
        mem_unit_scan(iblk);
    }
 
    LeaveCriticalSection(&pool->blklistCS);
 
    return 0;
}


void mupool_print (FILE * fp, void * vpool)
{
    mupool_t * pool = (mupool_t *)vpool;
    MemBlk   * iblk = NULL;
    int        i, j, num, index = 0;
    uint8    * piter = NULL;
    uint8    * pend = NULL;
    uint32     flag = 0;
    size_t     unitlen = 0;
     
    if (!pool) return;
 
    if (!fp) fp = stdout;
     
    EnterCriticalSection(&pool->blklistCS);

    num = arr_num(pool->mem_blk_list);

    fprintf(fp, "\nMemory Pool Chunk Num: %d  Chunk Size: %ld\n\n", num, pool->blksize);

    for (i = 0, index = 0; i < num; i++) {
        iblk = arr_value(pool->mem_blk_list, i);
        if (!iblk) continue;
     
        pend = iblk->pstart + iblk->cur;
        piter = iblk->pstart;

        fprintf(fp, "##MemBlk: %d  Pointer: %p  Unalloc Pointer: %p\n", i, iblk, pend);
        fprintf(fp, "  Chunk Size: %-6ld  Avail Size : %-6ld", iblk->totalsize, iblk->availsize);
        fprintf(fp, "    UnallocLoc: %-6ld  UnallocSize: %ld\n", iblk->cur, iblk->availsize - iblk->cur);
        fprintf(fp, "  Alloc Unit: %-6ld  Alloc Size : %-6ld", iblk->allocunit, iblk->allocsize);
        fprintf(fp, "    Red Line  : %-6ld  Leak Hole  : %d\n", iblk->redline, iblk->leaked);
     
        for (j = 0 ; piter < pend; j++) {
            flag = getuint32(piter);
            unitlen = flag & 0x7FFFFFFF;

            fprintf(fp, "    unit:%-3d %p %-5ld ", index++, piter+4, unitlen);
            if (flag & 0x80000000) fprintf(fp, "[ used ]");
            else fprintf(fp, "[      ]");

            if ((j+1) % 2 == 0) printf("\n");

            piter += 4 + unitlen;
        }
        if (j % 2 == 1) printf("\n");
     
        fprintf(fp, "End Block: %d\n", i);
    }
    fprintf(fp, "\n");

    LeaveCriticalSection(&pool->blklistCS);
}

