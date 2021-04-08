/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 *
 * File Cache management structure and interface definition
 * Try a small buffer to hold file or net stream
 */
 

#ifndef _FILE_CACHE_H_
#define _FILE_CACHE_H_
 
#ifdef __cplusplus
extern "C" {
#endif


void * file_cache_init (int packnum, int packsize);
int    file_cache_clean(void * vcache);

int file_cache_set_prefix_ratio (void * vcache, float ratio);
int file_cache_setbuf  (void * vcache, void * pbuf, int buflen, int packsize);

int file_cache_setfile (void * vcache, char * file, int64 offset);
int file_cache_setcdn (void * vcache, void * pmedia, int64 offset, uint64 msize, void * cdnread);

int file_cache_seek (void * vcache, int64 offset);
int file_cache_at   (void * vcache, int64 offset);
int file_cache_read (void * vcache, void * destbuf, int len, int nbmode);
int file_cache_recv (void * vcache, void * pbuf, int length, int waitms);

int64  file_cache_readpos (void * vcache);
int64  file_cache_filesize(void * vcache);
int    file_cache_eof (void * vcache);

long file_cache_skip_quote_to (void * vcache, long pos, int skiplimit, void * pat, int patlen);
long file_cache_skip_esc_to (void * vcache, long pos, int skiplimit, void * pat, int patlen);

long file_cache_skip_to (void * vcache, long pos, int skiplimit, void * pat, int patlen);
long file_cache_rskip_to (void * vcache, long pos, int skiplimit, void * pat, int patlen);

long file_cache_skip_over (void * vcache, long pos, int skiplimit, void * pat, int patlen);
long file_cache_rskip_over (void * vcache, long pos, int skiplimit, void * pat, int patlen);

/* usage example:
    void * fca = NULL;

    fca = file_cache_init(12, 8192);
    file_cache_setfile(fca, "./zhizhuxia.mp4", 0);

    file_cache_seek(fca, 133489L);
    file_cache_recv(fca, buf, 16384, 0);

    file_cache_clean(fca);
 */
#ifdef __cplusplus
}
#endif
 
#endif
 
