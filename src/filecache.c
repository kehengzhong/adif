/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include "btype.h"
#include "memory.h"
#include "dynarr.h"
#include "mthread.h"
#include "bpool.h"
#include "nativefile.h"

#include "filecache.h"

typedef int FCCDNRead (void * pmedia, uint8 * pbuf, uint32 * readsize, int64 offset);


typedef struct file_cache { 
     
    CRITICAL_SECTION   cacheCS; 
     
    CRITICAL_SECTION   fpCS;
    uint8              mediatype;    //0-memory 1-local file 2-cdn media
    char               filename[64];
    void             * hfile; 

    void             * pmedia; 
    FCCDNRead        * cdnread;

    int64              offset; 
    int64              length; 
 
    uint8              bufalloc;
    void             * pbuf;
    int                buflen;
    int                actlen;
    int                packsize;
    int                packnum;
    arr_t            * pack_list;
 
    float              prefix_ratio;
    int                prefix;
    int                suffix;
 
    int                packtotal;
    int                residual;
 
    int                bgn_pack;
    int                bgn_pack_max;
    int                seek_pack;
    int64              seekpos;
 
    bpool_t          * pack_pool;
 
    int                bufsize;
    int                bufpack;
 
    uint32             totalread;
    uint32             failread;
 
    void             * avail_event;
 
} FileCache;
 

int    file_cache_load_all (void * vcache);
int    file_cache_load_one (void * vcache);
 
int64  file_cache_offset  (void * vcache);
int    file_cache_packsize(void * vcache);
int    file_cache_packnum (void * vcache);
int    file_cache_filled  (void * vcache);
 
 
int    file_cache_len (void * vcache, int * successive);
void * file_cache_ptr (void * vcache);
 
int file_cache_buffering_size  (void * vcache, int bufsize);
int file_cache_buffering_ratio (void * vcache, int * ratio);


#define PACK_NULL       0
#define PACK_INIT       1
#define PACK_RCVING     2
#define PACK_SUCC       3
typedef struct file_pack {
    void             * res[2];
    void             * cache;
    int                index;
    uint8            * pbyte;    //buffer for the data
    uint32             length;   //size planning to load
 
    void             * unitarr[8];
    int                unitnum;
 
    CRITICAL_SECTION   packCS;
    int                packind;
    uint8              state;    //current operation state
    int                rcvlen;   //actual loaded length
} FilePack;
 
 
int file_pack_init (void * vpack);
int file_pack_free (void * vpack);
 
void * file_pack_fetch (void * vcache);
int    file_pack_recycle(void * vpack);
 
void * file_pack_buf (void * vpack);
int    file_pack_size(void * vpack);
int    file_pack_len (void * vpack);
 
int file_pack_recv (void * vpack, SOCKET fd);
 
int file_pack_exit_load(void * vpack);
 
int file_pack_state_to (void * vpack, int state);
int file_pack_ready_notify (void * vpack);
 
int file_pack_bind_unit  (void * vpack, void * vunit);
int file_pack_unbind_unit(void * vpack, void * vunit);
int file_pack_unbind_all (void * vpack);
 
 
int file_pack_reload (void * vpack, int reason, void * curt);


static void * file_cache_get_idle_pack (void * vcache);
static int file_cache_seek_to (void * vcache, int64 offset);
static int file_cache_load_pack (void * vcache, void * vpack);


void * file_cache_init (int packnum, int packsize)
{
    FileCache * cache = NULL;
    uint8     * pbuf = NULL;
    int         buflen = 0;

    cache = kzalloc(sizeof(*cache));
    if (!cache) return NULL;

    InitializeCriticalSection(&cache->cacheCS);
    InitializeCriticalSection(&cache->fpCS);
    cache->offset = 0;
    cache->length = 0;

    cache->prefix_ratio = (float)0.1;

    cache->pbuf = NULL;
    cache->buflen = cache->actlen = 0;
    cache->packsize = 0;
    cache->packnum = 0;
    cache->pack_list = arr_new(16);

    cache->packtotal = 0;
    cache->residual = 0;

    cache->bgn_pack = 0;
    cache->bgn_pack_max = 0;
    cache->seek_pack = 0;
    cache->seekpos = 0;

    cache->bufsize = 0;
    cache->bufpack = 0;

    cache->totalread = 0;
    cache->failread = 0;

    cache->avail_event = event_create();

    if (packsize <= 0) packsize = 8192;
    buflen = packnum * packsize;
    if (packnum > 0) pbuf = kalloc(buflen);
    if (pbuf) {
        if (file_cache_setbuf(cache, pbuf, buflen, packsize) < 0) {
            file_cache_clean(cache);
            return NULL;
        }
        cache->bufalloc = 1;
    }

    return cache;
}

int file_cache_clean (void * vcache)
{
    FileCache * cache = (FileCache *)vcache;

    if (!cache) return -1;

    if (cache->avail_event) {
        event_destroy(cache->avail_event);
        cache->avail_event = NULL;
    }

    DeleteCriticalSection(&cache->cacheCS);

    DeleteCriticalSection(&cache->fpCS);
    if (cache->mediatype == 1 && cache->hfile) {
        native_file_close(cache->hfile);
        cache->hfile = NULL;
    }

    while (arr_num(cache->pack_list) > 0)
        file_pack_free(arr_pop(cache->pack_list));
    arr_free(cache->pack_list);
    cache->pack_list = NULL;

    if (cache->bufalloc) {
        if (cache->pbuf) kfree(cache->pbuf);
        cache->pbuf = NULL;
    }

    kfree(cache);

    return 0;
}

int file_cache_set_prefix_ratio (void * vcache, float ratio)
{
    FileCache * cache = (FileCache *)vcache;

    if (!cache) return -1;

    cache->prefix_ratio = ratio;

    cache->prefix = (int)((float)cache->packnum * ratio);
    if (cache->prefix > cache->packnum) cache->prefix = cache->packnum;
    if (cache->prefix < 0) cache->prefix = 0;

    return 0;
}

int file_cache_setbuf (void * vcache, void * pbuf, int buflen, int packsize)
{
    FileCache * cache = (FileCache *)vcache;
    FilePack  * pack = NULL;
    int         i = 0;

    if (!cache) return -1;

    if (!pbuf) return -2;
    if (buflen <= 0) return -3;
    if (packsize <= 0) packsize = 8192;
    if (buflen < packsize) return -4;

    EnterCriticalSection(&cache->cacheCS);
    /* clear the old pack list */
    for (i=0; i<cache->packnum; i++) {
        file_pack_recycle(arr_pop(cache->pack_list));
    }
    arr_zero(cache->pack_list);

    if (cache->pbuf && cache->bufalloc) {
        kfree(cache->pbuf);
        cache->bufalloc = 0;
    }
    cache->pbuf = pbuf;
    cache->buflen = buflen;
    cache->packsize = packsize;

    cache->packnum = buflen/packsize;
    cache->actlen = cache->packnum * packsize;

    cache->prefix = (int)((float)cache->packnum * cache->prefix_ratio);
    cache->suffix = (int)((float)cache->packnum * cache->prefix_ratio);

    for (i=0; i<cache->packnum; i++) {
        pack = file_pack_fetch(cache);
        memset(pack->unitarr, 0, sizeof(pack->unitarr));
        pack->unitnum = 0;

        pack->index = i;
        pack->pbyte = (uint8 *)cache->pbuf + i*cache->packsize;
        pack->length = cache->packsize;

        pack->packind = -1;
        pack->state = PACK_NULL;
        pack->rcvlen = 0;
        arr_push(cache->pack_list, pack);
    }

    LeaveCriticalSection(&cache->cacheCS);

    return 0;
}

int file_cache_packlist_prepare (void * vcache)
{
    FileCache * cache = (FileCache *)vcache;
    FilePack  * pack = NULL;
    int         i = 0;

    if (!cache) return -1;

    EnterCriticalSection(&cache->cacheCS);

    if (cache->packsize > 0) {
        cache->packtotal = (int)((cache->length + cache->packsize - 1)/cache->packsize);
        cache->residual = (int)(cache->length % cache->packsize);
    } else {
        cache->packtotal = 0;
        cache->residual = 0;
    }
    cache->bgn_pack = 0;
    if (cache->packtotal > cache->packnum)
        cache->bgn_pack_max = cache->packtotal - cache->packnum;
    else
        cache->bgn_pack_max = 0;
    cache->seek_pack = 0;
    cache->seekpos = 0;

    for (i=0; i<cache->packnum; i++) {
        pack = arr_value(cache->pack_list, (i+cache->bgn_pack)%cache->packnum);
        pack->packind = i + cache->bgn_pack;
        pack->state = PACK_NULL;
        pack->rcvlen = 0;
    }

    LeaveCriticalSection(&cache->cacheCS);

    return 0;
}


int file_cache_setfile (void * vcache, char * file, int64 offset)
{ 
    FileCache * cache = (FileCache *)vcache;
     
    if (!cache) return -1;
    if (!file) return -2;
 
    cache->mediatype = 1;

    if (file) strncpy(cache->filename, file, sizeof(cache->filename)-1);
 
    if (cache->hfile) native_file_close(cache->hfile);
    cache->hfile = native_file_open(cache->filename, NF_READ);
    if (cache->hfile == NULL) return -100;
 
    native_file_attr(cache->hfile, &cache->length, NULL, NULL, NULL);
    cache->offset = offset;
 
    return file_cache_packlist_prepare(cache);
}

int file_cache_setcdn (void * vcache, void * pmedia, int64 offset, uint64 msize, void * cdnread)
{
    FileCache * cache = (FileCache *)vcache;
 
    if (!cache) return -1;
    if (!pmedia) return -2;
    if (!cdnread) return -3;
 
    cache->mediatype = 2;
 
    cache->pmedia = pmedia;
    cache->cdnread = (FCCDNRead *)cdnread;
 
    cache->offset = offset;
    cache->length = msize;
 
    return file_cache_packlist_prepare(cache);
}


int file_cache_eof (void * vcache)
{
    FileCache * cache = (FileCache *)vcache;

    if (!cache) return -1;

    if (cache->seekpos >= cache->length) return 1;
    return 0;
}

int64 file_cache_filesize (void * vcache)
{
    FileCache * cache = (FileCache *)vcache;

    if (!cache) return -1;

    return (int64)cache->length;
}


static void * file_cache_get_idle_pack (void * vcache)
{
    FileCache * cache = (FileCache *)vcache;
    FilePack  * pack = NULL;
    int         packind = 0;
    int         i;

    if (!cache) return NULL;

    EnterCriticalSection(&cache->cacheCS);
    for (i = cache->seek_pack; 
         i < cache->bgn_pack + cache->packnum && i < cache->packtotal;
         i++)
    {
        packind = i % cache->packnum;
        pack = arr_value(cache->pack_list, packind);

        if (pack->state == PACK_NULL) {
            LeaveCriticalSection(&cache->cacheCS);
            return pack;
        }
    }
    LeaveCriticalSection(&cache->cacheCS);

    return NULL;
}


static int file_cache_seek_to (void * vcache, int64 offset)
{
    FileCache * cache = (FileCache *)vcache;
    FilePack  * pack = NULL;
    int         seekpack = 0;
    int         prepack = 0;
    int         packs = 0;
    int         newbgn = 0;
    int         i, packind;

    if (!cache) return -1;

    if (offset > cache->length) offset = cache->length;

    cache->seekpos = offset;
    cache->seek_pack = seekpack = (int)(offset/cache->packsize);

    if (cache->seek_pack > cache->bgn_pack && 
        cache->bgn_pack == cache->bgn_pack_max) return 0;

    prepack = seekpack - cache->prefix;
    if (prepack < 0) prepack = 0;

    if (seekpack >= cache->bgn_pack && seekpack < cache->bgn_pack + cache->packnum) {
        packs = seekpack - cache->bgn_pack;
        if (packs <= cache->prefix) return 0;
        newbgn = prepack;

    } else if (seekpack < cache->bgn_pack) {
        newbgn = seekpack;

    } else { //if (seekpack >= cache->bgn_pack + cahe->packnum)
        if (prepack < cache->bgn_pack + cache->packnum) 
            newbgn = prepack;
        else 
            newbgn = seekpack;
    }

    if (newbgn > cache->bgn_pack_max) 
        newbgn = cache->bgn_pack_max;

    for (i = 0; i < cache->packnum; i++) {
        packind = i + newbgn;
        if (packind >= cache->bgn_pack && packind < cache->bgn_pack + cache->packnum)
            continue;

        pack = arr_value(cache->pack_list, packind % cache->packnum);
        file_pack_exit_load(pack);
        pack->packind = packind;
    }

    cache->bgn_pack = newbgn;

    return 0;
}

int file_cache_seek (void * vcache, int64 offset)
{
    FileCache * cache = (FileCache *)vcache;
    int         ret = 0;

    if (!cache) return -1; 
 
    if (offset >= cache->length) 
        offset = cache->length - 1;
 
    if (cache->seekpos == offset) return 0;

    EnterCriticalSection(&cache->cacheCS);
    ret = file_cache_seek_to(cache, offset);
    LeaveCriticalSection(&cache->cacheCS); 

    if (ret >= 0) ret = file_cache_load_all(cache);

    return ret; 

}

long file_cache_skip_over (void * vcache, long pos, int skiplimit, void * vpat, int patlen)
{
    FileCache * cache = (FileCache *)vcache;
    uint8     * pat = (uint8 *)vpat;
    int         fch = 0;
    long        i, fsize = 0;
    int         j = 0;

    if (!cache) return -1;
    if (!pat || patlen <= 0) return pos;

    fsize = cache->length - pos;
    if (skiplimit >= 0 && skiplimit < fsize)
        fsize = skiplimit;

    for (i = 0; i < fsize; i++) {
        fch = file_cache_at(cache, pos + i);

        for (j = 0; j < patlen; j++) {
            if (pat[j] == fch) break;
        }

        if (j >= patlen) return pos+i;
    }

    return pos + i;
}

long file_cache_rskip_over (void * vcache, long pos, int skiplimit, void * vpat, int patlen)
{    
    FileCache * cache = (FileCache *)vcache;
    uint8     * pat = (uint8 *)vpat;
    int         fch = 0; 
    long        i; 
    int         j = 0;  
         
    if (!cache) return -1; 
    if (!pat || patlen <= 0) return pos;
 
    if (pos <= 0) return pos;

    if (pos >= cache->length) pos = cache->length - 1;

    for (i = 0; i <= pos; i++) {
        if (skiplimit >= 0 && i >= skiplimit)
            break;

        fch = file_cache_at(cache, pos-i);

        for (j = 0; j < patlen; j++) {
            if (pat[j] == fch) break;
        }

        if (j >= patlen) return pos - i;
    }

    return pos - i;
}

static long file_cache_quotedstrlen (void * vcache, long pos, int skiplimit)
{
    FileCache * cache = (FileCache *)vcache;
    int         fch = 0;
    int         quote = 0;
    long        fsize = 0;
    long        i = 0;

    if (!cache) return 0;

    fsize = cache->length;
    if (pos >= fsize) return 0;

    quote = file_cache_at(cache, pos);
    if (quote != '"' && quote != '\'') return 0;

    for (i = 1; i < skiplimit && i < fsize - pos; i++) {
        fch = file_cache_at(cache, pos+i);
        if (fch == '\\') i++;
        else if (fch == quote) return i+1;
    }

    return 1;
}

long file_cache_skip_quote_to (void * vcache, long pos, int skiplimit, void * vpat, int patlen)
{
    FileCache * cache = (FileCache *)vcache;
    uint8     * pat = (uint8 *)vpat;
    int         fch = 0;
    long        fsize = 0;
    long        i = 0, qlen = 0;
    int         j = 0;
 
    if (!cache) return -1;
    if (!pat || patlen <= 0) return pos;
 
    fsize = cache->length - pos;
    if (skiplimit >= 0 && skiplimit < fsize)
        fsize = skiplimit;

    for (i = 0; i < fsize; ) {
        fch = file_cache_at(cache, pos + i);

        if (fch == '\\' && i + 1 < fsize) {
            i += 2;
            continue;
        }

        for (j = 0; j < patlen; j++) {
            if (pat[j] == fch) return pos + i;
        }

        if (fch == '"' || fch == '\'') {
            qlen = file_cache_quotedstrlen(cache, pos + i, fsize - i);
            i += qlen;
            continue;
        }
        i++;
    }

    return pos + i;
}
 

long file_cache_skip_to (void * vcache, long pos, int skiplimit, void * vpat, int patlen)
{ 
    FileCache * cache = (FileCache *)vcache;
    uint8     * pat = (uint8 *)vpat;
    int         fch = 0; 
    long        fsize = 0;
    long        i = 0, j = 0;
     
    if (!cache) return -1;
    if (!pat || patlen <= 0) return pos;
     
    fsize = cache->length;
    for (i = 0; i < fsize - pos; i++) {
        if (skiplimit >= 0 && i >= skiplimit)
            break;

        fch = file_cache_at(cache, pos + i); 
        for (j = 0; j < patlen; j++) {  
            if (pat[j] == fch) return pos + i;
        }    
    }    
    return pos + i;
}    

long file_cache_rskip_to (void * vcache, long pos, int skiplimit, void * vpat, int patlen)
{
    FileCache * cache = (FileCache *)vcache;
    uint8     * pat = (uint8 *)vpat;
    int         fch = 0;
    long        fsize = 0;
    long        i = 0, j = 0;
     
    if (!cache) return -1;
    if (!pat || patlen <= 0) return pos;
 
    fsize = cache->length;
    if (pos < 0) return 0;
    if (pos >= fsize) pos = fsize - 1;

    for (i = 0; i <= pos; i++) { 
        if (skiplimit >= 0 && i >= skiplimit)
            break;

        fch = file_cache_at(cache, pos-i);
        for (j = 0; j < patlen; j++) {
            if (pat[j] == fch) return pos - i;
        }     
    }     

    return pos - i; 
}    

long file_cache_skip_esc_to (void * vcache, long pos, int skiplimit, void * vpat, int patlen)
{  
    FileCache * cache = (FileCache *)vcache;
    uint8     * pat = (uint8 *)vpat;
    int         fch = 0;  
    long        fsize = 0;
    long        i = 0, j = 0;
      
    if (!cache) return -1;
    if (!pat || patlen <= 0) return pos;
      
    fsize = cache->length;
    for (i = 0; i < fsize - pos; i++) {
        if (skiplimit >= 0 && i >= skiplimit)
            break;

        fch = file_cache_at(cache, pos+i);  
        if (fch == '\\') { i++; continue; }

        for (j = 0; j < patlen; j++) {   
            if (pat[j] == fch) return pos + i;
        }
    }

    return pos + i;
}
 

int file_cache_at (void * vcache, int64 offset)
{
    FileCache * cache = (FileCache *)vcache;
    int64       seekpos = 0;
    int         seekpack = 0;
    FilePack  * pack = NULL;
    int         packpos = 0;
    int         ret = 0;
    long        acctime = 0;
    struct timeval t1, tmid;
 
    if (!cache) return -1;
 
    if (offset >= cache->length) return -1;
 
    if (cache->seekpos != offset) {
        EnterCriticalSection(&cache->cacheCS);
        ret = file_cache_seek_to(cache, offset);
        LeaveCriticalSection(&cache->cacheCS);
    }

    seekpos = cache->seekpos;
    seekpack = (int)(seekpos/cache->packsize);
    pack = arr_value(cache->pack_list, seekpack % cache->packnum);
    if (!pack) return -201;

    if (pack->state == PACK_NULL)
        file_cache_load_pack(cache, pack);

    packpos = (int)(seekpos % cache->packsize);

    acctime = 0;
    while (pack->rcvlen-packpos < 1 && pack->state != PACK_SUCC) {
       gettimeofday(&tmid, NULL);
       ret = event_wait(cache->avail_event, 500);
       gettimeofday(&t1, NULL);
       acctime += tv_diff_us(&tmid, &t1);
 
       if (pack->state == PACK_SUCC) break;
 
       if (acctime > 0 && acctime > 500*1000) {
           acctime = 0;
           //if (pack->rcvlen < pack->length/10)
                file_pack_reload(pack, 1, &t1);
        }
    }
 
    if (pack->rcvlen-packpos < 1) return -200;

    ret = pack->pbyte[packpos];

    return ret;
}

int file_cache_read (void * vcache, void * pbuf, int length, int nbmode)
{
    FileCache * cache = (FileCache *)vcache;
    FilePack  * pack = NULL;
    int64       seekpos = 0;
    int         seekpack = 0;
    int         packpos = 0;
    int         iter = 0, cplen = 0;
    long        acctime = 0;
    int         failed = 0;
    struct timeval t1, tmid;

    if (!cache) return -1;
    if (cache->seekpos >= cache->length) return -200;

    cache->totalread++;

    seekpos = cache->seekpos;

    while (iter < length && seekpos < cache->length) {
        seekpack = (int)(seekpos/cache->packsize);
        pack = arr_value(cache->pack_list, seekpack % cache->packnum);
        if (!pack) return -201;

        if (pack->state == PACK_NULL) 
            file_cache_load_pack(cache, pack);

        packpos = (int)(seekpos % cache->packsize);
        cplen = length - iter;

        if ((int)pack->rcvlen-packpos <= 0 && nbmode) {
            break;
        }

        acctime = 0;
        while ((int)pack->rcvlen-packpos < cplen && pack->state != PACK_SUCC && !nbmode) {
            gettimeofday(&tmid, NULL);
            event_wait(cache->avail_event, 1*1000);
            gettimeofday(&t1, NULL);
            acctime += tv_diff_us(&tmid, &t1);

            if (pack->state == PACK_SUCC) break;

            if (acctime > 0 && acctime > 100*1000) {
                acctime = 0;
                //if (pack->rcvlen < pack->length/10) 
                    file_pack_reload(pack, 1, &t1);
            }
        }
        if (acctime > 500000 || (int)pack->rcvlen <= packpos) failed++;

        if ((int)pack->rcvlen-packpos < cplen) 
            cplen = (int)pack->rcvlen-packpos;

        if (cplen > 0) {
            if (pbuf)
                memcpy((uint8 *)pbuf+iter, (uint8 *)pack->pbyte+packpos, cplen); 
            iter += cplen;
            seekpos += cplen;

            EnterCriticalSection(&cache->cacheCS);
            file_cache_seek_to(cache, seekpos);
            LeaveCriticalSection(&cache->cacheCS);
        }
    }

    if (iter < length) cache->failread++;
    file_cache_load_all(cache);

    return iter;
}


int file_cache_recv (void * vcache, void * pbuf, int length, int waitms)
{
    FileCache * cache = (FileCache *)vcache;
    FilePack  * pack = NULL;
    int64       seekpos = 0;
    int         seekpack = 0;
    int         packpos = 0;
    int         iter = 0, cplen = 0;
    long        acctime = 0;
    int         failed = 0;
    struct timeval t1, tmid;
 
    if (!cache) return -1;
    if (cache->seekpos >= cache->length) return -200;
 
    cache->totalread++;

    seekpos = cache->seekpos;

    while (iter < length && seekpos < cache->length) {
        seekpack = (int)(seekpos/cache->packsize);
        pack = arr_value(cache->pack_list, seekpack % cache->packnum);
        if (!pack) return -201;
 
        if (pack->state == PACK_NULL)
            file_cache_load_pack(cache, pack);
 
        packpos = (int)(seekpos % cache->packsize);
        cplen = length - iter;
         
        if ((int)pack->rcvlen-packpos < cplen && waitms <= 0) {
            break; 
        }    

        acctime = 0;
        while ((int)pack->rcvlen-packpos < cplen && pack->state != PACK_SUCC && waitms > 0) {
            gettimeofday(&tmid, NULL);
            event_wait(cache->avail_event, waitms);

            gettimeofday(&t1, NULL);
            acctime += tv_diff_us(&tmid, &t1);
            waitms -= tv_diff_us(&tmid, &t1)/1000;
 
            if (pack->state == PACK_SUCC) break;

            if (acctime > 0 && acctime > 100*1000) {
                acctime = 0;
                //if (pack->rcvlen < pack->length/10)
                    file_pack_reload(pack, 1, &t1);
            }
        }
        if (acctime > 500000 || (int)pack->rcvlen <= packpos) failed++;
 
        if ((int)pack->rcvlen-packpos < cplen)
            cplen = (int)pack->rcvlen-packpos;
 
        if (cplen > 0) {
            if (pbuf)
                memcpy((uint8 *)pbuf+iter, (uint8 *)pack->pbyte+packpos, cplen); 
            iter += cplen;
            seekpos += cplen;
 
            EnterCriticalSection(&cache->cacheCS);
            file_cache_seek_to(cache, seekpos);
            LeaveCriticalSection(&cache->cacheCS);
        }
    }
 
    if (iter < length) cache->failread++;
    file_cache_load_all(cache);

    return iter;
}
 

static int file_cache_load_pack (void * vcache, void * vpack)
{
    FileCache * cache = (FileCache *)vcache;
    FilePack  * pack = (FilePack *)vpack;
    int64       offset = 0;
    
    if (!cache) return -1;
    
    EnterCriticalSection(&pack->packCS);
    pack->state = PACK_INIT;
    pack->rcvlen = 0;

    if (pack->packind >= cache->packtotal-1)
        pack->length = cache->residual;
    else
        pack->length = cache->packsize;
    LeaveCriticalSection(&pack->packCS);
    
    offset = cache->offset + (int64)pack->packind * (int64)cache->packsize;

    EnterCriticalSection(&cache->fpCS);

    if (cache->mediatype == 1) {
        native_file_seek(cache->hfile, offset);
        pack->rcvlen = native_file_read(cache->hfile, pack->pbyte, pack->length);

    } else if (cache->mediatype == 2 && cache->cdnread) {
        uint32  readsize = pack->length;
        (*cache->cdnread)(cache->pmedia, pack->pbyte, &readsize, offset);
        pack->rcvlen = readsize;
    }

    LeaveCriticalSection(&cache->fpCS);

    pack->state = PACK_SUCC;

    return 0;
}


int file_cache_load_all (void * vcache)
{
    FileCache * cache = (FileCache *)vcache;
    FilePack  * pack = NULL;
    int         i = 0;
    int         nodenum = 0;

    if (!cache) return -1;

    if (nodenum < 8) nodenum = 8;

    for (i = 0; i < nodenum && i < cache->packnum && i < cache->packtotal; i++) {
        pack = file_cache_get_idle_pack(cache);
        if (!pack) {
            return i;
        }
        file_cache_load_pack(cache, pack);
    }
    
    return i;
}

int file_cache_load_one (void * vcache)
{
    FileCache * cache = (FileCache *)vcache;
    FilePack  * pack = NULL;
    
    if (!cache) return -1;
    
    pack = file_cache_get_idle_pack(cache);
    if (!pack) {
        return 0;
    }

    file_cache_load_pack(cache, pack);

    return 1;
}


int64 file_cache_offset (void * vcache)
{
    FileCache * cache = (FileCache *)vcache;

    if (!cache) return 0;

    return cache->offset;
}

int file_cache_packsize (void * vcache)
{
    FileCache * cache = (FileCache *)vcache;

    if (!cache) return 0;

    return cache->packsize;
}

int file_cache_packnum (void * vcache)
{
    FileCache * cache = (FileCache *)vcache;

    if (!cache) return 0;

    return cache->packnum;
}

int file_cache_filled  (void * vcache)
{
    FileCache * cache = (FileCache *)vcache;
    int         i, packind;
    FilePack  * pack = NULL;

    if (!cache) return 0;

    for (i = cache->seek_pack; i < cache->bgn_pack + cache->packnum && i < cache->packtotal; i++) {
        packind = i % cache->packnum;
        pack = arr_value(cache->pack_list, packind);

        if (pack->state != PACK_SUCC) return 0;
    }

    return 1;
}

int file_cache_len (void * vcache, int * successive)
{
    FileCache * cache = (FileCache *)vcache;
    int         wraparound = 0;
    int         len = 0;
    int         i, packind, num=0;
    FilePack  * pack = NULL;
    int         packpos = 0;
 
    if (!cache) return 0;
 
    packpos = (int)(cache->seekpos % cache->packsize);
    for (i = cache->seek_pack; i < cache->bgn_pack + cache->packnum && i < cache->packtotal; i++) {
        packind = i % cache->packnum;
        pack = arr_value(cache->pack_list, packind);
        if (pack->state != PACK_SUCC) break;

        len += pack->rcvlen;
        if (i == cache->seek_pack && packpos > 0) len -= packpos;

        if (packind == 0 && num > 0) wraparound = 1;
        num++;
    }

    if (successive) *successive = wraparound;

    return len;
}

void * file_cache_ptr (void * vcache)
{
    FileCache * cache = (FileCache *)vcache;
    FilePack  * pack = NULL;
    int         packpos = 0;

    if (!cache) return NULL;

    pack = arr_value(cache->pack_list, cache->seek_pack%cache->packnum);
    packpos = (int)(cache->seekpos % cache->packsize);
    return (uint8 *)pack->pbyte + packpos;
}

int64 file_cache_readpos (void * vcache)
{
    FileCache * cache = (FileCache *)vcache;

    if (!cache) return 0;

    /* cache->offset is start-point from the media beginning.
     * generally, cche->offset is 0. alternatively, it is set to given value. */
    return cache->seekpos;
}

int file_cache_buffering_size  (void * vcache, int bufsize)
{
    FileCache * cache = (FileCache *)vcache; 

    if (!cache) return -1;

    EnterCriticalSection(&cache->cacheCS);

    if (cache->bufsize != bufsize) {
        cache->bufsize = bufsize;
        if (cache->bufsize < 0) cache->bufsize = 0;
        if (cache->packsize > 0)
            cache->bufpack = (cache->bufsize + cache->packsize - 1)/cache->packsize;

        if (cache->bufpack > cache->packnum - cache->prefix) {
            cache->bufpack = cache->packnum - cache->prefix;
            cache->bufsize = cache->bufpack * cache->packsize;
        }

        cache->suffix = cache->bufpack;
    }

    LeaveCriticalSection(&cache->cacheCS);

    return 0;
}

int file_cache_buffering_ratio (void * vcache, int * ratio)
{
    FileCache * cache = (FileCache *)vcache; 
    FilePack  * pack = NULL; 
    int         i, packind; 
    int         suclen = 0;
         
    if (ratio) *ratio = 0;

    if (!cache) return -1; 

    if (ratio && cache->bufsize <= 0) *ratio = 100;
 
    EnterCriticalSection(&cache->cacheCS);

    for (i = cache->seek_pack; 
         i < cache->bgn_pack+cache->packnum && i < cache->packtotal;
         i++)
    {
        packind = i % cache->packnum;
        pack = arr_value(cache->pack_list, packind);

        if (pack->state != PACK_SUCC) {
            suclen += pack->rcvlen;
            break;
        }

        suclen += pack->rcvlen;
        if (pack->rcvlen < (int)pack->length) break;
        if (suclen >= cache->bufsize) break;
    }

    LeaveCriticalSection(&cache->cacheCS);

    if (ratio && cache->bufsize > 0) {
        if (suclen >= cache->bufsize) *ratio = 100;
        else {
            if (i >= cache->packtotal) *ratio = 100;
            else
                *ratio = (int)((float)suclen * 100. / (float)cache->bufsize);
        }
    }

    return 0;
}


int file_pack_init (void * vpack)
{
    FilePack * pack = (FilePack *)vpack;

    if (!pack) return -1;

    InitializeCriticalSection(&pack->packCS);
    pack->index = -1;
    pack->pbyte = NULL;
    pack->length = 0;

    memset(pack->unitarr, 0, sizeof(pack->unitarr));
    pack->unitnum = 0;

    pack->packind = -1;
    pack->state = PACK_NULL;
    pack->rcvlen = 0;
    return 0;
}

int file_pack_free (void * vpack)
{
    FilePack * pack = (FilePack *)vpack;

    if (!pack) return -1;

    DeleteCriticalSection(&pack->packCS);

    kfree(pack);
    return 0;
}

void * file_pack_fetch (void * vcache)
{
    FileCache * cache = (FileCache *)vcache;
    FilePack  * pack = NULL;

    if (!cache) return NULL;

    if (cache->pack_pool) 
        pack = bpool_fetch(cache->pack_pool);
    if (!pack) {
        pack = kzalloc(sizeof(*pack));
        if (pack) file_pack_init(pack);
    }
    if (!pack) return NULL;

    pack->cache = cache;
    return pack;
}

int file_pack_recycle(void * vpack)
{
    FilePack  * pack = (FilePack *)vpack;
    FileCache * cache = NULL;

    if (!pack) return -1;

    cache = (FileCache *)pack->cache;
    if (!cache) return file_pack_free(pack);

    if (!cache->pack_pool) return file_pack_free(pack);

    return bpool_recycle(cache->pack_pool, pack);
}

void * file_pack_buf (void * vpack)
{
    FilePack  * pack = (FilePack *)vpack;

    if (!pack) return NULL;

    return pack->pbyte;
}

int file_pack_size (void * vpack)
{
    FilePack  * pack = (FilePack *)vpack;

    if (!pack) return 0;

    return pack->length;
}

int file_pack_len (void * vpack)
{
    FilePack  * pack = (FilePack *)vpack;

    if (!pack) return 0;

    return pack->rcvlen;
}


int file_pack_exit_load(void * vpack)
{
    FilePack  * pack = (FilePack *)vpack;
 
    if (!pack) return -1;

    EnterCriticalSection(&pack->packCS);
    memset(pack->unitarr, 0, sizeof(pack->unitarr));
    pack->unitnum = 0;

    pack->state = PACK_NULL;
    pack->rcvlen = 0;
    LeaveCriticalSection(&pack->packCS);

    return 0;
}

int file_pack_state_to (void * vpack, int state)
{
    FilePack  * pack = (FilePack *)vpack;
 
    if (!pack) return -1;

    pack->state = state;
    return 0;
}

int file_pack_ready_notify (void * vpack)
{
    FilePack  * pack = (FilePack *)vpack;
    FileCache * cache = NULL;

    if (!pack) return -1;

    cache = (FileCache *)pack->cache;
    if (!cache) return -2;

    event_set(cache->avail_event, 1);

    return 0;
}


int file_pack_bind_unit (void * vpack, void * vunit)
{
    FilePack  * pack = (FilePack *)vpack;
    int         i = 0;

    if (!pack) return -1;
    if (!vunit) return -2;

    for (i=0; i<pack->unitnum; i++) {
        if (pack->unitarr[i] == vunit) return -100;
    }
    if (pack->unitnum >= 5) return -200;

    pack->unitarr[pack->unitnum++] = vunit;
    return 0;
}

int file_pack_unbind_unit (void * vpack, void * vunit)
{
    FilePack  * pack = (FilePack *)vpack;
    int         i, j;
 
    if (!pack) return -1;
    if (!vunit) return -2;
 
    for (i=0; i<pack->unitnum; i++) {
        if (pack->unitarr[i] == vunit) {
            for (j=i; j<pack->unitnum-1; j++) 
                pack->unitarr[j] = pack->unitarr[j+1];
            pack->unitnum--;
            return 1;
        }
    }
    return 0;
}

int file_pack_unbind_all (void * vpack)
{
    FilePack  * pack = (FilePack *)vpack;

    if (!pack) return -1;

    memset(pack->unitarr, 0, sizeof(pack->unitarr));
    pack->unitnum = 0;
    return 0;
}


int file_pack_reload (void * vpack, int reason, void * pcurt)
{
    FilePack  * pack = (FilePack *)vpack;
    FileCache * cache = NULL;
    int64       offset = 0;

    if (!pack) return -1;

    cache = (FileCache *)pack->cache;
    if (!cache) return -2;

    if (pack->unitnum >= 5) return 0;
    if (pack->state == PACK_SUCC) return 0;

    offset = cache->offset + (int64)pack->packind * (int64)cache->packsize;

    EnterCriticalSection(&cache->fpCS);

    if (cache->mediatype == 1) {
        native_file_seek(cache->hfile, offset);
        pack->rcvlen = native_file_read(cache->hfile, pack->pbyte, pack->length);

    } else if (cache->mediatype == 2 && cache->cdnread) {
        uint32 readsize = pack->length;
        (*cache->cdnread)(cache->pmedia, pack->pbyte, &readsize, offset);
        pack->rcvlen = readsize;
    }

    LeaveCriticalSection(&cache->fpCS);

    pack->state = PACK_SUCC;

    return 0;
}

