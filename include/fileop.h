/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#ifndef _FILEOP_H_
#define _FILEOP_H_ 
    
#include "btype.h"

#include <sys/stat.h>
#ifdef UNIX
#include <unistd.h>
#include <sys/mman.h>
#endif

#define SENDFILE_MAXSIZE  2147479552L

#ifdef __cplusplus
extern "C" {
#endif 

#if defined(_WIN32) || defined(_WIN64)
int    file_handle_to_fd (HANDLE hfile);
HANDLE fd_to_file_handle (int fd);

time_t FileTimeToUnixTime (FILETIME ft);
#endif

int filefd_read   (int fd, void * pbuf, int size);
int filefd_write  (int fd, void * pbuf, int size);
int filefd_writev (int fd, void * piov, int iovcnt, int64 * actnum);
int filefd_copy   (int fdin, int64 offset, int64 length, int fdout, int64 * actnum);

long  file_read  (FILE * fp, void * buf, long readlen);
long  file_write (FILE * fp, void * buf, long writelen);
int64 file_seek  (FILE * fp, int64 pos, int whence);
int   file_valid (FILE * fp);

int64 file_size  (char * file);
int   file_stat  (char * file, void * pfs);
int   file_exist (char * file);
int   file_is_regular (char * file);
int   file_is_dir (char * file);
int64 file_attr (char * file, long * inode, int64 * size, time_t * atime, time_t * mtime, time_t * ctime);

/* recursively create directory, including it's all parent directories */
int file_dir_create (char * path, int hasfilename);

int file_rollover (char * fname, int line);
int file_lines (char * file);

int file_copy (char * srcfile, int64 offset, int64 length, char * dstfile, int64 * actnum);
int file_copy2fp (char * srcfile, int64 offset, int64 length, FILE * fpout, int64 * actnum);

int file_conv_charset (char * srcchst, char * dstchst, char * srcfile, char * dstfile);

int WinPath2UnixPath (char * path, int len);
int UnixPath2WinPath (char * path, int len);
#if defined(_WIN32) || defined(_WIN64)
char * realpath (char * path, char * resolvpath);
#endif

char * file_extname (char * file);
char * file_basename (char * file);
int file_abspath (char * file, char * path, int pathlen);

int file_get_absolute_path (char * relative, char * abs, int abslen);
int file_mime_type (void * mimemgmt, char * fname, char * pmime, uint32 * mimeid, uint32 * appid);

#ifdef UNIX
void * file_mmap (void * addr, int fd, int64 offset, int64 length, int prot, int flags,
                  void ** ppmap, int64 * pmaplen, int64 * pmapoff);
int file_munmap (void * pmap, int64 maplen);
#endif

#if defined(_WIN32) || defined(_WIN64)
void * file_mmap (void * addr, HANDLE hfile, int64 offset, int64 length, char * mapname,
                  HANDLE * phmap, void ** ppmap, int64 * pmaplen, int64 * pmapoff);
int file_munmap (HANDLE hmap, void * pmap);
#endif

void * fbuf_init (char * fname, int pagecount);
void   fbuf_free (void * vfb);

int    fbuf_fd   (void * vfb);
int64  fbuf_size (void * vfb);

int    fbuf_mmap (void * vfb, int64 pos);

int    fbuf_at   (void * vfb, int64 pos);
void * fbuf_ptr  (void * vfb, int64 pos, void ** ppbuf, int * plen);
int    fbuf_read (void * vfb, int64 pos, void * pbuf, int len);

long fbuf_skip_to  (void * vfb, long pos, int skiplimit, void * pat, int patlen);
long fbuf_rskip_to (void * vfb, long pos, int skiplimit, void * pat, int patlen);

long fbuf_skip_over  (void * vfb, long pos, int skiplimit, void * pat, int patlen);
long fbuf_rskip_over (void * vfb, long pos, int skiplimit, void * pat, int patlen);

long fbuf_skip_quote_to (void * vfb, long pos, int skiplimit, void * pat, int patlen);
long fbuf_skip_esc_to   (void * vfb, long pos, int skiplimit, void * pat, int patlen);

#ifdef __cplusplus
}
#endif 


#endif

