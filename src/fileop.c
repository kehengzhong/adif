/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */
 
#include "btype.h"
#include "fileop.h"
#include "strutil.h"
#include "memory.h"
#include "filecache.h"
#include "mimetype.h"
 
#ifdef UNIX
#include <iconv.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/uio.h>

#ifdef _LINUX_
#include <sys/sendfile.h>
#endif
#ifdef _FREEBSD_
#include <sys/types.h>
#include <sys/socket.h>
#endif

#endif
 
#if defined(_WIN32) || defined(_WIN64)
#include <direct.h>
#include <tchar.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
 
 
int file_handle_to_fd (HANDLE hfile)
{
    if (!hfile) return -1;
 
    return _open_osfhandle((intptr_t)hfile, _O_APPEND | _O_RDONLY);
}
 
HANDLE fd_to_file_handle (int fd)
{
    return (HANDLE)_get_osfhandle(fd);
}
 
time_t FileTimeToUnixTime (FILETIME ft)
{
    ULARGE_INTEGER ui;
 
    ui.LowPart  =  ft.dwLowDateTime;
    ui.HighPart =  ft.dwHighDateTime;
 
    return ( (ui.QuadPart  -  116444736000000000LL)  /  10000000 );
}
 
#endif
 
 
int filefd_read (int fd, void * pbuf, int size)
{
    int          ret = 0;
    int          len = 0;
 
    if (fd < 0) return -1;
    if (!pbuf) return -2;
    if (size < 0) return -3;
 
    for (len = 0; len < size; ) {
        ret = read(fd, (uint8 *)pbuf + len, size - len);
        if (ret < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                SLEEP(10); //sleep 1 milli-seconds
                continue;
            }
            return -100;
 
        } else if (ret == 0) { //reach end of the file
            break;
        }
 
        len += ret;
    }
 
    return len;
}
 
int filefd_write (int fd, void * pbuf, int size)
{
    int          ret = 0;
    int          len = 0;
 
    if (fd < 0) return -1;
    if (!pbuf) return -2;
    if (size < 0) return -3;
 
    for (len = 0; len < size; ) {
        ret = write(fd, (uint8 *)pbuf + len, size - len);
        if (ret < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                SLEEP(10);
                continue;
            }
            return -100;
        }
        len += ret;
    }
 
    return len;
}
 
int filefd_writev (int fd, void * piov, int iovcnt, int64 * actnum)
{
#ifdef UNIX
    struct iovec * iov = (struct iovec *)piov;
    int  ret, errcode;
    int  ind, wlen, sentnum = 0;
 
    if (actnum) *actnum = 0;
 
    if (fd < 0) return -1;
    if (!iov || iovcnt <= 0) return 0;
 
    for (ind = 0, sentnum = 0; ind < iovcnt; ) {
        ret = writev(fd, iov + ind, iovcnt - ind);
        if (ret < 0) {
            errcode = errno;
 
            if (errcode == EINTR) {
                continue;
 
            } else if (errcode == EAGAIN || errcode == EWOULDBLOCK) {
                return 0;
            }
 
            return -30;
        }
 
        sentnum += ret;
        if (actnum) *actnum += ret;
 
        for(wlen = ret; ind < iovcnt && wlen >= iov[ind].iov_len; ind++)
            wlen -= iov[ind].iov_len;
 
        if (ind >= iovcnt) break;
 
        iov[ind].iov_base = (char *)iov[ind].iov_base + wlen;
        iov[ind].iov_len -= wlen;
    }
 
    return sentnum;
 
#else
 
    struct iovec * iov = (struct iovec *)piov;
    int  i, ret, errcode;
    int  wlen = 0;
 
    if (actnum) *actnum = 0;
 
    if (fd < 0) return -1;
    if (!iov || iovcnt <= 0) return 0;
 
    for (i = 0; i < iovcnt; i++) {
        ret = write(fd, iov[i].iov_base, iov[i].iov_len);
        if (ret > 0) {
            wlen += ret;
            if (actnum) *actnum += ret;
            continue;
        }
 
        if (ret == -1) {
            errcode = WSAGetLastError();
 
#if defined(_WIN32) || defined(_WIN64)
            if (errcode == WSAEINTR || errcode == WSAEWOULDBLOCK) {
                return wlen;
            }
#endif
            return -30;
        }
    }
 
    return wlen;
#endif
}
 
int filefd_copy (int fdin, int64 offset, int64 length, int fdout, int64 * actnum)
{
    int64        wlen = 0;
    int64        toread = 0;
    struct stat  st;

#ifdef UNIX
    off_t        offval = offset;
    ssize_t      ret;
#else
    int          len = 0;
    uint8        inbuf[16384];
#endif
 
    if (actnum) *actnum = 0;
 
    if (fdin < 0) return -1;
    if (fdout < 0) return -2;
 
    if (fstat(fdin, &st) < 0)
        return -10;
 
    if (offset < 0) offset = 0;
    if (offset >= st.st_size)
        return 0;
 
    if (length < 0)
        length = st.st_size - offset;
    else if (length > st.st_size - offset)
        length = st.st_size - offset;
 
#ifdef UNIX

    for (wlen = 0; wlen < length; ) {
        toread = min(length - wlen, SENDFILE_MAXSIZE);

#ifdef _FREEBSD_
        offval = 0;
        ret = sendfile(fdin, fdout, offset, toread, NULL, &offval, 0);

        if (offval > 0) {
            wlen += offval;
            offset += offval;
        }

#elif defined(_OSX_)
        offval = 0;
        ret = sendfile(fdin, fdout, offset, &offval, NULL, 0);

        if (offval > 0) {
            wlen += offval;
            offset += offval;
        }

#else
        ret = sendfile(fdout, fdin, &offval, toread);
#endif
        if (ret < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(50);
                continue;
            }
            if (actnum) *actnum = wlen;
            return -400;
        }
#if defined(_FREEBSD_) || defined(_OSX_) || defined(_MACOS_)
        else {
            /* do nothing */
        }
#else
        else if (ret == 0) {
            /* if sendfile returns zero, then someone has truncated the file,
             * so the offset became beyond the end of the file */

            if (actnum) *actnum = wlen;
            return -500;
 
        } else {
            wlen += ret;
        }
#endif
    }
 
#else
 
    lseek(fdin, offset, SEEK_SET);
 
    for (wlen = 0; wlen < length; ) {
        toread = min(length - wlen, sizeof(inbuf));
 
        len = filefd_read(fdin, inbuf, toread);
        if (len > 0) {
            len = filefd_write(fdout, inbuf, len);
            if (len > 0) {
                wlen += len;
            }
        }
 
        if (len < 0) {
            if (actnum) *actnum = wlen;
            return -60;
        }
    }
#endif
 
    if (actnum) *actnum = wlen;

    return 0;
}
 
 
long file_read (FILE * fp, void * buf, long readlen)
{
    long i, iRet = 0;
 
    if (!fp) return -1;
    if (!buf) return -2;
    if (readlen <= 0) return -3;
    
    for (i = 0; i < readlen && !feof(fp); ) {
        iRet = fread((uint8 *)buf + i, 1, readlen - i, fp);
        i += iRet;
    }
 
    return i;
}
 
 
 
long file_write (FILE * fp, void * buf, long writelen)
{
    long i, iRet = 0;
 
    if (!fp) return -1;
    if (!buf) return -2;
    if (writelen <= 0) return -3;
    
    for (i = 0; i < writelen; ) {
        iRet = fwrite((uint8 *)buf + i, 1, writelen - i, fp);
        i += iRet;
    }
    fflush(fp);
 
    return i;
}
 
int64 file_seek (FILE * fp, int64 pos, int whence)
{
    int   ret = 0;
 
    if (!fp) return -1;
 
#ifdef UNIX
    ret = fseeko(fp, pos, whence);
    if (ret >= 0) return ftello(fp);
#endif
 
#if defined(_WIN32) || defined(_WIN64)
    ret = fseek(fp, (long)pos, whence);
    if (ret >= 0) return (int64)ftell(fp);
#endif
 
    return -100;
}
 
 
int file_valid (FILE * fp)
{
#ifdef UNIX
    struct stat st;
#endif
#if defined(_WIN32) || defined(_WIN64)
    struct _stat st;
#endif
 
    if (!fp) return 0;
 
    memset(&st, 0, sizeof(st));
 
#ifdef UNIX
    if (fstat(fileno(fp), &st) != 0) return 0;
    if (S_ISREG(st.st_mode)) return 1;
#endif
#if defined(_WIN32) || defined(_WIN64)
    if (_fstat(fileno(fp), &st) != 0) return 0;
    if (!(_S_IFDIR & st.st_mode)) return 1;
#endif
 
    return 0;
}
 
 
int64 file_size (char * file)
{
#ifdef UNIX
    struct stat fs;
#endif
#if defined(_WIN32) || defined(_WIN64)
    HANDLE       hFile = NULL;
    int64        nSize = 0;
    ulong        dwHigh = 0;
    ulong        dwSize = 0;
#endif
 
    if (!file) return -1;
 
#ifdef UNIX
    if (stat(file, &fs)== -1) {
        return -2;
    }
 
    return (int64)fs.st_size;
#endif
 
#if defined(_WIN32) || defined(_WIN64)
    hFile = (HANDLE)CreateFile(file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        dwSize = GetFileSize(hFile, &dwHigh);
        nSize = dwHigh;
        nSize <<= 32;
        nSize |= dwSize;
        CloseHandle(hFile);
        return nSize;
    } else {
        return -2;
    }
#endif
}
 
 
int file_stat (char * file, void * pfs)
{
#ifdef UNIX
    struct stat fs;
 
    if (!file) return -1;
 
    if (stat(file, &fs) < 0) {
        return -2;
    }
 
    if (pfs) *(struct stat *)pfs = fs;
#endif
 
#if defined(_WIN32) || defined(_WIN64)
    struct _stat fs;
    WIN32_FILE_ATTRIBUTE_DATA  wfad;
 
    if (!file) return -1;
 
    if (GetFileAttributesEx(file, GetFileExInfoStandard, &wfad) == 0) {
        return -2;
    }
 
    memset(&fs, 0, sizeof(fs));
 
    fs.st_ctime = FileTimeToUnixTime(wfad.ftCreationTime);
    fs.st_atime = FileTimeToUnixTime(wfad.ftLastAccessTime);
    fs.st_mtime = FileTimeToUnixTime(wfad.ftLastWriteTime);
 
    fs.st_size = wfad.nFileSizeHigh;
    fs.st_size <<= 32;
    fs.st_size |= wfad.nFileSizeLow;
 
    if (pfs) *(struct _stat *)pfs = fs;
#endif
 
    return 0;
}
 
int file_exist (char * file)
{
#ifdef UNIX
    struct stat fs;
#endif
 
#if defined(_WIN32) || defined(_WIN64)
    WIN32_FILE_ATTRIBUTE_DATA  wfad;
#endif
 
    if (!file) return 0;
 
#ifdef UNIX
    if (stat(file, &fs) == -1) {
        if (errno == ENOENT) return 0;
    }
#endif
 
#if defined(_WIN32) || defined(_WIN64)
    if (GetFileAttributesEx(file, GetFileExInfoStandard, &wfad) == 0) {
        return 0;
    }
 
    if ((wfad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        return 1;
 
#endif
 
    return 1;
}
 
 
int file_is_regular (char * file)
{
#ifdef UNIX
    struct stat fs;
#endif
#if defined(_WIN32) || defined(_WIN64)
    WIN32_FILE_ATTRIBUTE_DATA  wfad;
#endif
 
    if (!file) return 0;
 
#ifdef UNIX
    if (stat(file, &fs)== -1) {
        if (errno == ENOENT) return 0;
    }
    if (!S_ISREG(fs.st_mode)) return 0;
 
    return 1;
#endif
 
#if defined(_WIN32) || defined(_WIN64)
    if (GetFileAttributesEx(file, GetFileExInfoStandard, &wfad) == 0) {
        return 0;
    }
 
    if ((wfad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        return 1;
 
    return 0;
#endif
}
 
int file_is_dir (char * file) 
{ 
#ifdef UNIX
    struct stat fs; 
#endif
#if defined(_WIN32) || defined(_WIN64)
    WIN32_FILE_ATTRIBUTE_DATA  wfad;
#endif
 
    if (!file) return 0; 
 
#ifdef UNIX 
    if (stat(file, &fs)== -1) {
        if (errno == ENOENT) return 0;
    } 
    if (S_ISDIR(fs.st_mode)) return 1;
#endif
 
#if defined(_WIN32) || defined(_WIN64)
    if (GetFileAttributesEx(file, GetFileExInfoStandard, &wfad) == 0) {
        return 0;
    }
 
    if (wfad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        return 1;
#endif
 
    return 0;
}
 
 
int64 file_attr (char * file, long * inode, int64 * size, time_t * atime, time_t * mtime, time_t * ctime)
{
    int64       fsize = 0;
#ifdef UNIX
    struct stat fs;
#endif
#if defined(_WIN32) || defined(_WIN64)
    struct _stat fs;
#endif
 
    if (!file) return -1;
 
#ifdef UNIX
    if (stat(file, &fs)== -1) {
        return -2;
    }
#endif
 
#if defined(_WIN32) || defined(_WIN64)
    if (file_stat(file, &fs)== -1) {
        return -2;
    }
#endif
 
    fsize = fs.st_size;
 
    if (size) *size = fs.st_size;
    if (inode) *inode = fs.st_ino;
    if (atime) *atime = fs.st_atime;
    if (mtime) *mtime = fs.st_mtime;
    if (ctime) *ctime = fs.st_ctime;
 
    return fsize;
}
 
 
int file_dir_create (char * path, int hasfilename)
{
    char * p = NULL, * piter = NULL;
    char * subpath = NULL;
    int    sublen = 0;
 
    if (!path) return -1;
 
    p = str_trim(path);
    if (file_exist(p)) return 0;
 
#ifdef UNIX
    piter = strrchr(p, '/');
#endif
#if defined(_WIN32) || defined(_WIN64)
    piter = strrchr(p, '\\');
#endif
 
    sublen = piter - p;
    if (piter && sublen > 0) { 
        subpath = kzalloc(sublen+1);
        strncpy(subpath, p, sublen);
        
        if (!file_exist(subpath)) {
            file_dir_create(subpath, 0);
        }
        if (subpath) kfree(subpath);
    }
 
    if (!hasfilename) {
    #ifdef UNIX
        mkdir(p, 0755);
    #endif
    #if defined(_WIN32) || defined(_WIN64)
        CreateDirectory(p, NULL);
    #endif
    }
 
    return 0;
}
 
int file_rollover (char * fname, int line)
{
    FILE  * fp = NULL;
    FILE  * tmpfp = NULL;
    char    tmpfname[256];
    char    buf[4096];
    int     seqno = 0;
    struct timeval  curt;
 
    if (!fname) return -1;
    if (line <= 0) return -2;
 
    gettimeofday(&curt, NULL);
 
    sprintf(tmpfname, "%s.%05ld", fname, curt.tv_usec);
 
    fp = fopen(fname, "r");
    if (!fp) return -3;
 
    tmpfp = fopen(tmpfname, "w");
    if (!tmpfp) { 
        fclose(fp);
        return -5;
    }
 
    for (seqno = 0; !feof(fp); seqno++) {
        memset(buf, 0, sizeof(buf));
        fgets(buf, sizeof(buf), fp);
        if (seqno < line) continue;
 
        fprintf(tmpfp, "%s", buf);
    }
    fclose(fp);
    fclose(tmpfp);
 
    rename(tmpfname, fname);
    return 0;
}
 
 
int file_lines (char * file)
{
    void   * fca = NULL;
    long     fsize = 0;
    long     iter = 0;
    int      line = 0;
 
    if (!file_is_regular(file)) return 0;
 
    fca = file_cache_init(8, 16384);
    if (!fca) return 0;
 
    file_cache_set_prefix_ratio(fca, 0.5);
    if (file_cache_setfile(fca, file, 0) < 0) {
        file_cache_clean(fca);
        return 0;
    }
 
    fsize = file_cache_filesize(fca);
    if (fsize <= 0) return 0;
 
    for (iter = 0; iter < fsize; iter++) {
        if (file_cache_at(fca, iter) == '\n') {
            line++;
            /* contiguous line is ignored */
            //iter = file_cache_skip_over(fca, iter, 10, "\r\n", 2);
        }
    }
    file_cache_clean(fca);
 
    return line;
}
 
 
int file_copy (char * srcfile, int64 offset, int64 length, char * dstfile, int64 * actnum)
{
    int64     size = 0;
 
#ifdef UNIX
    int       fdin;
    int       fdout;
#else
    int64     toread = 0;
    FILE    * fpin = NULL;
    FILE    * fpout = NULL;
    uint8     inbuf[16384];
    int       ret = 0;
#endif
 
    if (actnum) *actnum = 0;
 
    if (!srcfile) return -1;
    if (!dstfile) return -2;
 
    if ((size = file_size(srcfile)) < 0)
        return -10;
 
    if (offset < 0) offset = 0;
    if (offset >= size)
        return -100;
 
    if (length < 0)
        length = size - offset;
    else if (length > size - offset)
        length = size - offset;
 
#ifdef UNIX
    fdin = open(srcfile, O_RDONLY);
    if (fdin == -1) return -200;
 
    fdout = open(dstfile, O_RDWR|O_CREAT|O_TRUNC, S_IWUSR|S_IRUSR);
    if (fdout == -1) {
        close(fdin);
        return -300;
    }
 
    filefd_copy(fdin, offset, length, fdout, actnum);
 
    close(fdin);
    close(fdout);
#else
    fpin = fopen(srcfile, "rb+");
    if (!fpin) return -200;
 
    fpout = fopen(dstfile, "wb");
    if (!fpout) {
        fclose(fpin);
        return -300;
    }
 
    file_seek(fpin, offset, SEEK_SET);
 
    for (; length > 0; ) {
        if (length > sizeof(inbuf)) toread = sizeof(inbuf);
        else toread = length;
 
        ret = file_read(fpin, inbuf, toread);
        if (ret > 0) {
            ret = file_write(fpout, inbuf, ret);
            if (ret > 0) {
                length -= ret;
                if (actnum) *actnum += ret;
            }
        }
 
        if (ret < 0) return -60;
    }
 
    fclose(fpin);
    fclose(fpout);
#endif
 
    return 0;
}
 
 
int file_copy2fp (char * srcfile, int64 offset, int64 length, FILE * fpout, int64 * actnum)
{ 
    int64     size = 0;
     
#ifdef UNIX
    int       fdin;
#else
    int64     toread = 0;
    FILE    * fpin = NULL;
    char      inbuf[16384];
    int       ret = 0;
#endif
 
    if (actnum) *actnum = 0;
 
    if (!srcfile) return -1;
    if (!fpout) return -2;
     
    if ((size = file_size(srcfile)) < 0)
        return -10;
     
    if (offset < 0) offset = 0;
    if (offset >= size)
        return -100;
 
    if (length < 0) length = size - offset;
    if (length > size - offset)
        length = size - offset;
     
#ifdef UNIX
    fdin = open(srcfile, O_RDONLY);
    if (fdin == -1) return -200;
 
    filefd_copy(fdin, offset, length, fileno(fpout), actnum);
 
    close(fdin);
#else
    fpin = fopen(srcfile, "rb+");
    if (!fpin) return -200;
 
    file_seek(fpin, offset, SEEK_SET);
 
    for (; length > 0; ) {
        if (length > sizeof(inbuf)) toread = sizeof(inbuf);
        else toread = length;
 
        ret = file_read(fpin, inbuf, toread);
        if (ret > 0) {
            ret = file_write(fpout, inbuf, ret);
            if (ret > 0) {
                length -= ret;
                if (actnum) *actnum += ret;
            }
        }
 
        if (ret < 0) return -60;
    }
    fclose(fpin);
#endif
 
    return 0;
}
 
#ifdef UNIX
int file_conv_charset (char * srcchst, char * dstchst, char * srcfile, char * dstfile)
{
    iconv_t   hconv = 0;
    FILE    * fpin = NULL;
    FILE    * fpout = NULL;
    int       readlen = 0;
    char     inbuf[1024];
    size_t    inbuflen = 0;
    char     outbuf[2048];
    size_t    outbuflen = 0;
    int       size = 0; 
    int       acclen = 0;
    int       ret = 0; 
    char   * pin = NULL;
    size_t    inlen = 0;
    char   * pout = NULL;
    size_t    outlen = 0;
 
    if (!srcchst || !dstchst) return -1;
    if (!srcfile || !dstfile) return -2;
 
    if ((size = file_size(srcfile)) < 0) return -10;
 
    hconv = iconv_open(dstchst, srcchst);
    if (hconv == (iconv_t)-1) return -100;
 
    fpin = fopen(srcfile, "rb+");
    fpout = fopen(dstfile, "wb");
 
    inlen = 0; outlen = 0;
    for (acclen = 0; size > 0; ) {
        if (size > sizeof(inbuf) - inlen) readlen = sizeof(inbuf) - inlen;
        else {
            readlen = size;
        }
        readlen = file_read(fpin, inbuf+inlen, readlen);
        size -= readlen;
 
        inbuflen = readlen + inlen;
        outbuflen = sizeof(outbuf);
 
        pin = inbuf; inlen = inbuflen;
        pout = outbuf; outlen = outbuflen;
 
iconv_again:
        ret = iconv(hconv, (char **)&pin, (size_t *)&inlen, (char **)&pout, (size_t *)&outlen);
        if (ret < 0) {
            if (errno == E2BIG) {
                /* The output buffer has no more room
                 * for the next converted character */
                fclose(fpin);
                fclose(fpout);
                iconv_close(hconv);
                return acclen; //return actual converted bytes in orignal stream
 
            } else if (errno == EINVAL || errno == EILSEQ) {
                /* EINVAL: An incomplete multibyte sequence is encountered
                 * in the input, and the input byte sequence terminates after it.*/
                /* EILSEQ: An  invalid multibyte sequence is encountered in the input.
                 * *inbuf  is  left pointing to the beginning of the
                 * invalid multibyte sequence. */
                if (inlen > 3 || size <= 0) {
                    *pout = *pin;
                    pin++; pout++;
                    inlen--; outlen--;
                    goto iconv_again;
                }
 
            } else break;
        }
 
        if (inlen > 0) memmove(inbuf, inbuf+inbuflen-inlen, inlen);
        acclen += inbuflen - inlen;
        file_write(fpout, outbuf, outbuflen-outlen);
    }
 
    fclose(fpin);
    fclose(fpout);
    iconv_close(hconv);
 
    return acclen;
}
#endif
 
int WinPath2UnixPath (char * path, int len)
{   
    int i;
 
    if (!path) return -1;
    if (len < 0) len = (int)strlen(path);
    if (len <= 0) return -2;
 
    for (i = 0; i < len; i++) {
        if (path[i] == '\\') path[i] = '/';
    }
    return i;
}
 
 
int UnixPath2WinPath (char * path, int len)
{
    int i;
 
    if (!path) return -1;
    if (len < 0) len = (int)strlen(path);
    if (len <= 0) return -2;
 
    for (i = 0; i < len; i++) {
        if (path[i] == '/') path[i] = '\\';
    }
    return i;
}
 
char * file_extname (char * file)
{
    char * pbgn = NULL;
    char * pend = NULL;
    char * poct = NULL;
    int     len = 0;
 
    if (!file) return "";
 
    len = strlen(file);
    if (len <= 0) return "";
 
    pbgn = file; pend = pbgn + len;
    poct = rskipTo(pend-1, len, ".", 1);
    if (poct == NULL || poct <= pbgn) return "";
    return poct;
}
 
char * file_basename (char * file)
{
    char * pbgn = NULL;
    char * pend = NULL;
    char * poct = NULL;
    int     len = 0;
 
    if (!file) return "";
 
    len = strlen(file);
    if (len <= 0) return "";
 
    pbgn = file; pend = pbgn + len;
    poct = rskipTo(pend-1, len, "/\\", 2);
    if (poct == NULL || poct <= pbgn) return file;
    return poct+1;
}
 
int file_abspath (char * file, char * path, int pathlen)
{
    char   fpath[1024];
    char * p = NULL;
    int    len = 0;
 
    file_get_absolute_path(file, fpath, sizeof(fpath)-1);
 
    len = strlen(fpath);
    p = rskipTo(fpath + len - 1, len, "/\\", 2);
    if (p && p >= fpath) {
        if (path && pathlen > 0)
            str_secpy(path, pathlen, fpath, p - fpath + 1);
        return p - fpath + 1;
    }
 
    return -100;
}
 
 
int file_get_absolute_path (char * relative, char * abs, int abslen)
{
    char   fpath[512];
    char   file[512];
    char   curpath[512];
    char   destpath[512];
    char * p = NULL;
    int    len = 0;
 
    if (!abs || abslen <= 0) return -2;
 
    memset(fpath, 0, sizeof(fpath));
    memset(curpath, 0, sizeof(curpath));
    memset(destpath, 0, sizeof(destpath));
 
#ifdef UNIX
    getcwd(curpath, sizeof(curpath)-1);
    if (relative) {
        if (file_is_regular(relative)) {
            p = strrchr(relative, '/');
            if (p) {
                strncpy(file, p+1, sizeof(file)-1);
                memcpy(fpath, relative, p - relative);
            } else {
                strncpy(file, relative, sizeof(file)-1);
            }
        } else {
            file[0] = '\0';
            strncpy(fpath, relative, sizeof(fpath)-1);
        }
        chdir(fpath);
        getcwd(destpath, sizeof(destpath)-1);
        chdir(curpath);
 
        memset(abs, 0, abslen);
 
        snprintf(abs, abslen, "%s/%s", destpath, file);
        len = strlen(abs);
    } else {
        memset(abs, 0, abslen);
 
        len = strlen(curpath);
        if (len > abslen) len = abslen;
        memcpy(abs, curpath, len);
    }
#endif
 
#if defined(_WIN32) || defined(_WIN64)
    GetCurrentDirectory(sizeof(curpath)-1, curpath);
    if (relative) {
        if (file_is_regular(relative)) {
            p = strrchr(relative, '/'); 
            if (p) { 
                strncpy(file, p+1, sizeof(file)-1);
                memcpy(fpath, relative, p - relative);
            } else { 
                strncpy(file, relative, sizeof(file)-1);
            }    
        } else {
            file[0] = '\0';
            strncpy(fpath, relative, sizeof(fpath)-1);
        }    
        SetCurrentDirectory(fpath);
        GetCurrentDirectory(sizeof(destpath)-1, destpath);
        SetCurrentDirectory(curpath);
 
        memset(abs, 0, abslen);
 
        snprintf(abs, abslen, "%s/%s", destpath, file);
        len = strlen(abs);
    } else {
        len = strlen(curpath);
        if (len > abslen) len = abslen;
        memcpy(abs, curpath, len);
    }
#endif
 
    return len;
}
 
#if defined(_WIN32) || defined(_WIN64)
char * realpath (char * path, char * resolvpath)
{
    char  rpath[4096];
    DWORD ret = 0;
 
    ret = GetFullPathName(path, sizeof(rpath), rpath, NULL);
    if (ret <= 0) return NULL;
 
    rpath[ret] = '\0';
 
    if (resolvpath) {
        strcpy(resolvpath, rpath);
        return resolvpath;
    }
 
    resolvpath = malloc(ret + 1);
    str_secpy(resolvpath, ret, rpath, ret);
 
    return resolvpath;
}
#endif
 
int file_mime_type (void * mimemgmt, char * fname, char * pmime, uint32 * mimeid, uint32 * appid)
{
    FILE   * fp = NULL;
    char     mime[128];
    char   * p = NULL;
    char   * poct = NULL;
    char     cmd[256];
 
    if (pmime) strcpy(pmime, "application/octet-stream");
    if (mimeid) *mimeid = 30440;
    if (appid) *appid = 30440;
 
    if (!file_is_regular(fname)) return -100;
 
    sprintf(cmd, "file -bi %s", fname);
    memset(mime, 0, sizeof(mime));
#ifdef UNIX
    fp = popen(cmd, "r");
#endif
#if defined(_WIN32) || defined(_WIN64)
    fp = _popen(cmd, "r");
#endif
    if (fp && !feof(fp)) fgets(mime, sizeof(mime)-1, fp);
#ifdef UNIX
    if (fp) pclose(fp);
#endif
#if defined(_WIN32) || defined(_WIN64)
    if (fp) _pclose(fp);
#endif
 
    p = str_trim(mime);
    if (pmime) strcpy(pmime, p);
    poct = skipTo(p, strlen(p), ",; \t", 4);
    if (poct) *poct = '\0';
 
    return mime_type_get_by_mime(mimemgmt, p, NULL, mimeid, appid);
}
 
#ifdef UNIX
 
void * file_mmap (void * addr, int fd, int64 offset, int64 length, int prot, int flags,
                  void ** ppmap, int64 * pmaplen, int64 * pmapoff)
{
    struct stat   st;
    void        * pmap = NULL;
    size_t        maplen;
    off_t         pa_off = 0;
    off_t         pagesize = sysconf(_SC_PAGE_SIZE);
 
    if (!fd || fstat(fd, &st) < 0)
        return NULL;
 
    if (length <= 0) return NULL;
 
    if (offset >= st.st_size) return NULL;
 
    if (prot == 0) prot = PROT_READ | PROT_WRITE;
    if (flags == 0) flags = MAP_SHARED;
 
    pa_off = offset & ~(pagesize - 1);
    maplen = length + offset - pa_off;
 
    if (pa_off + maplen > st.st_size)
        maplen = st.st_size - pa_off;
 
    pmap = mmap(addr, maplen, prot, flags, fd, pa_off);
    if (pmap == MAP_FAILED)
        return NULL;
 
    if (ppmap) *ppmap = pmap;
    if (pmaplen) *pmaplen = maplen;
    if (pmapoff) *pmapoff = pa_off;
 
    return pmap + offset - pa_off;
}
 
int file_munmap (void * pmap, int64 maplen)
{
    return munmap(pmap, maplen);
}
 
#endif
 
#if defined(_WIN32) || defined(_WIN64)
 
void * file_mmap (void * addr, HANDLE hfile, int64 offset, int64 length, char * mapname,
                  HANDLE * phmap, void ** ppmap, int64 * pmaplen, int64 * pmapoff)
{
    BY_HANDLE_FILE_INFORMATION bhfi;
    int64         fsize = 0;
    SYSTEM_INFO   si;
    HANDLE        hmap;
    void        * pmap = NULL;
    int64         maplen;
    int64         pa_off = 0;
    int64         pagesize = 0;
 
    if (GetFileInformationByHandle(hfile, &bhfi) == 0)
        return NULL;
 
    fsize = bhfi.nFileSizeHigh;
    fsize <<= 32;
    fsize |= bhfi.nFileSizeLow;
 
    if (length <= 0) return NULL;
 
    if (offset >= fsize) return NULL;
 
    GetSystemInfo(&si);
    pagesize = si.dwPageSize;
 
    pa_off = offset & ~(pagesize - 1);
    maplen = length + offset - pa_off;
 
    if (pa_off + maplen > fsize)
        maplen = fsize - pa_off;
 
    hmap = OpenFileMapping(
                   PAGE_READONLY,         //FILE_MAP_ALL_ACCESS,   /* read/write access */
                   FALSE,                 /* do not inherit the name */
                   mapname );             /* name of mapping object */
    if (!hmap) {
        hmap = CreateFileMapping(
                   hfile,                       /* file handle intended to map */
                   NULL,                        /* default security */
                   PAGE_READONLY,               //PAGE_READWRITE | SEC_COMMIT, /* read/write access */
                   maplen >> 32,                /* maximum object size (high-order DWORD) */
                   maplen & 0xFFFFFFFF,         /* maximum object size (low-order DWORD) */
                   mapname);                    /* name of mapping object */
 
        if (!hmap) {
#ifdef _DEBUG
            printf( "CreateFileMapping error, Last error = %d\n", GetLastError() );
#endif
            return NULL;
        }
    }
 
    pmap = MapViewOfFileEx(
                   hmap,                   /* handle to map object */
                   FILE_MAP_READ,          /* read/write permission */
                   pa_off >> 32,           /* high-order DWORD of the file offset */
                   pa_off & 0xFFFFFFFF,    /* low-order DWORD of offset where mapping is to begin */
                   maplen,                 /* number of bytes of a file mapping to map */
                   addr);                  /* memory address where mapping begins */
 
    if (!pmap) {
#ifdef _DEBUG
        printf( "MapViewOfFile error, Last error = %d\n", GetLastError() );
#endif
        CloseHandle(hmap);
        return NULL;
    }
 
    if (phmap) *phmap = hmap;
    if (ppmap) *ppmap = pmap;
    if (pmaplen) *pmaplen = maplen;
    if (pmapoff) *pmapoff = pa_off;
 
    return (uint8 *)pmap + offset - pa_off;
}
 
int file_munmap (HANDLE hmap, void * pmap)
{
    UnmapViewOfFile(pmap);
    CloseHandle(hmap);
    return 0;
}
 
#endif
 
 
typedef struct file_buf_s {
    char        * fname;
    int           fd;
#if defined(_WIN32) || defined(_WIN64)
    HANDLE        hfile;
#endif
 
    int64         fsize;
 
    int           pagecount;   //how many memory pages used, passed by initializing
    int           pagesize;
    int           mapsize;
 
    int64         mapoff;
    int64         maplen;
    uint8       * pbyte;
    uint8       * mapaddr;
#if defined(_WIN32) || defined(_WIN64)
    HANDLE        hmap;
#endif
 
} fbuf_t;
 
 
void * fbuf_init (char * fname, int pagecount)
{
#ifdef UNIX
    fbuf_t * fbf = NULL;
    int      fd = -1;
    struct stat st;
 
    fd = open(fname, O_RDONLY);
    if (fd < 0)
        return NULL;
 
    fstat(fd, &st);
 
    fbf = kzalloc(sizeof(*fbf));
    if (!fbf) {
        close(fd);
        return NULL;
    }
 
    if (pagecount < 8) pagecount = 8;
    fbf->pagecount = pagecount;
 
    fbf->fname = str_dup(fname, strlen(fname));
    fbf->fd = fd;
    fbf->fsize = st.st_size;
 
    fbf->pagesize = sysconf(_SC_PAGE_SIZE);
    if (fbf->pagesize < 512)
        fbf->pagesize = 4096;
 
    fbf->mapsize = fbf->pagesize * fbf->pagecount; //1024;
 
    fbf->mapoff = fbf->maplen = 0;
    fbf->pbyte = NULL;
    fbf->mapaddr = NULL;
 
    return fbf;
#endif
 
#if defined(_WIN32) || defined(_WIN64)
    fbuf_t     * fbf = NULL;
    SYSTEM_INFO  si;
    HANDLE       hfile;
    DWORD        dwSize, dwHigh;
    int64        fsize = 0;
 
    hfile = CreateFile(fname, GENERIC_READ,
                       FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, 0, NULL);
    if (hfile == INVALID_HANDLE_VALUE) return NULL;
 
    dwSize = GetFileSize(hfile, &dwHigh);
    fsize = dwHigh;  fsize <<= 32;
    fsize |= dwSize;
 
    fbf = kzalloc(sizeof(*fbf));
    if (!fbf) {
        CloseHandle(hfile);
        return NULL;
    }
 
    if (pagecount < 8) pagecount = 8;
    fbf->pagecount = pagecount;
 
    fbf->fname = str_dup(fname, strlen(fname));
    fbf->hfile = hfile;
    fbf->fsize = fsize;
 
    GetSystemInfo(&si);
    fbf->pagesize = si.dwPageSize;
    if (fbf->pagesize < 512)
        fbf->pagesize = 4096;
 
    fbf->mapsize = fbf->pagesize * fbf->pagecount; //1024;
 
    fbf->mapoff = fbf->maplen = 0;
    fbf->pbyte = NULL;
    fbf->mapaddr = NULL;
    fbf->hmap = NULL;
 
    return fbf;
#endif
}
 
void fbuf_free (void * vfb)
{
    fbuf_t * fbf = (fbuf_t *)vfb;
 
    if (!fbf) return;
 
    if (fbf->pbyte) {
#ifdef UNIX
        munmap(fbf->mapaddr, fbf->maplen);
#endif
#if defined(_WIN32) || defined(_WIN64)
        file_munmap(fbf->hmap, fbf->mapaddr);
#endif
    }
 
    if (fbf->fd >= 0) {
        close(fbf->fd);
        fbf->fd = -1;
    }
 
    if (fbf->fname) {
        kfree(fbf->fname);
        fbf->fname = NULL;
    }
 
    kfree(fbf);
}
 
int fbuf_fd (void * vfb)
{
    fbuf_t * fbf = (fbuf_t *)vfb;
 
    if (!fbf) return -1;
 
    return fbf->fd;
}
 
int64 fbuf_size (void * vfb)
{
    fbuf_t * fbf = (fbuf_t *)vfb;
 
    if (!fbf) return 0;
 
    return fbf->fsize;
}
 
int fbuf_mmap (void * vfb, int64 pos)
{
    fbuf_t * fbf = (fbuf_t *)vfb;
 
    if (!fbf) return -1;
 
    if (pos < 0 || pos >= fbf->fsize)
        return -2;
 
    if (pos < fbf->mapoff || pos >= fbf->mapoff + fbf->maplen) {
 
        if (fbf->pbyte != NULL) {
#ifdef UNIX
            munmap(fbf->mapaddr, fbf->maplen);
#endif
#if defined(_WIN32) || defined(_WIN64)
            file_munmap(fbf->hmap, fbf->mapaddr);
#endif
            fbf->pbyte = NULL;
        }
 
        fbf->mapoff = pos / fbf->pagesize * fbf->pagesize;
 
        fbf->maplen = fbf->fsize - fbf->mapoff;
        if (fbf->maplen > fbf->mapsize)
            fbf->maplen = fbf->mapsize;
 
#ifdef UNIX
        fbf->pbyte = mmap(fbf->pbyte, fbf->maplen,
                            PROT_READ | PROT_WRITE, MAP_SHARED,
                            fbf->fd, fbf->mapoff);
        if (fbf->pbyte == MAP_FAILED) {
            return -3;
        }
        fbf->mapaddr = fbf->pbyte;
#endif
 
#if defined(_WIN32) || defined(_WIN64)
        fbf->pbyte = file_mmap(fbf->pbyte, fbf->hfile, fbf->mapoff, fbf->maplen, fbf->fname,
                               &fbf->hmap, (void **)&fbf->mapaddr, &fbf->maplen, &fbf->mapoff);
        if (!fbf->pbyte) return -3;
#endif
    }
 
    return 0;
}
 
 
int fbuf_at (void * vfb, int64 pos)
{
    fbuf_t * fbf = (fbuf_t *)vfb;
 
    if (!fbf) return -1;
 
    if (fbuf_mmap(fbf, pos) < 0)
        return -2;
 
    return fbf->pbyte[pos - fbf->mapoff];
}
 
void * fbuf_ptr (void * vfb, int64 pos, void ** ppbuf, int * plen)
{
    fbuf_t * fbf = (fbuf_t *)vfb;
 
    if (!fbf) return NULL;
 
    if (fbuf_mmap(fbf, pos) < 0)
        return NULL;
 
    if (ppbuf) *ppbuf = fbf->pbyte + pos - fbf->mapoff;
    if (plen) *plen = fbf->maplen - (pos - fbf->mapoff);
 
    return fbf->pbyte + pos - fbf->mapoff;
}
 
int fbuf_read (void * vfb, int64 pos, void * pbuf, int len)
{
    fbuf_t * fbf = (fbuf_t *)vfb;
    int      alen = 0;
 
    if (!fbf) return -1;
 
    if (!pbuf || len <= 0) return -1;
 
    if (fbuf_mmap(fbf, pos) < 0)
        return -2;
 
    alen = fbf->maplen - (pos - fbf->mapoff);
 
    if (len > alen) len = alen;
 
    memcpy(pbuf, fbf->pbyte + pos - fbf->mapoff, len);
 
    return len;
}
 
 
long fbuf_skip_to (void * vfb, long pos, int skiplimit, void * vpat, int patlen)
{
    fbuf_t * fbf = (fbuf_t *)vfb;
    uint8  * pat = (uint8 *)vpat;
    int      fch = 0, j;
    long     i = 0, fsize;
 
    if (!fbf) return -1;
    if (!pat || patlen <= 0) return pos;
 
    fsize = fbf->fsize - pos;
    if (skiplimit >= 0 && skiplimit < fsize)
        fsize = skiplimit;
 
    for (i = 0; i < fsize; i++) {
        fch = fbuf_at(fbf, pos + i);
        if (fch < 0) break;
 
        for (j = 0; j < patlen; j++) {
            if (pat[j] == fch) return pos + i;
        }
    }
 
    return pos + i;
}
 
long fbuf_rskip_to (void * vfb, long pos, int skiplimit, void * vpat, int patlen)
{
    fbuf_t * fbf = (fbuf_t *)vfb;
    uint8  * pat = (uint8 *)vpat;
    int      fch = 0;
    long     i = 0, j = 0;
 
    if (!fbf) return -1;
    if (!pat || patlen <= 0) return pos;
 
    if (pos < 0) return 0;
    if (pos >= fbf->fsize) pos = fbf->fsize - 1;
 
    for (i = 0; i <= pos; i++) {
        if (skiplimit >= 0 && i >= skiplimit)
            break;
 
        fch = fbuf_at(fbf, pos - i);
        if (fch < 0) break;
 
        for (j = 0; j < patlen; j++) {
            if (pat[j] == fch) return pos - i;
        }
    }
 
    return pos - i;
}
 
long fbuf_skip_over (void * vfb, long pos, int skiplimit, void * vpat, int patlen)
{
    fbuf_t * fbf = (fbuf_t *)vfb;
    uint8  * pat = (uint8 *)vpat;
    int      fch = 0;
    long     i, fsize = 0;
    int      j = 0;
 
    if (!fbf) return -1;
    if (!pat || patlen <= 0) return pos;
 
    fsize = fbf->fsize - pos;
    if (skiplimit >= 0 && skiplimit < fsize)
        fsize = skiplimit;
 
    for (i = 0; i < fsize; i++) {
        fch = fbuf_at(fbf, pos + i);
        if (fch < 0) break;
 
        for (j = 0; j < patlen; j++) {
            if (pat[j] == fch) break;
        }
 
        if (j >= patlen) return pos + i;
    }
 
    return pos + i;
}
 
long fbuf_rskip_over (void * vfb, long pos, int skiplimit, void * vpat, int patlen)
{
    fbuf_t * fbf = (fbuf_t *)vfb;
    uint8  * pat = (uint8 *)vpat;
    int      fch = 0;
    long     i;
    int      j = 0;
 
    if (!fbf) return -1;
    if (!pat || patlen <= 0) return pos;
 
    if (pos <= 0) return pos;
    if (pos >= fbf->fsize) pos = fbf->fsize - 1;
 
    for (i = 0; i <= pos; i++) {
        if (skiplimit >= 0 && i >= skiplimit)
            break;
 
        fch = fbuf_at(fbf, pos - i);
        if (fch < 0) break;
 
        for (j = 0; j < patlen; j++) {
            if (pat[j] == fch) break;
        }
        if (j >= patlen) return pos - i;
    }
 
    return pos - i;
}
 
static long fbuf_quotedstrlen (void * vfb, long pos, int skiplimit)
{
    fbuf_t * fbf = (fbuf_t *)vfb;
    int      fch = 0;
    int      quote = 0;
    long     fsize = 0;
    long     i = 0;
 
    if (!fbf) return 0;
 
    fsize = fbf->fsize;
    if (pos >= fsize) return 0;
 
    quote = fbuf_at(fbf, pos);
    if (quote != '"' && quote != '\'') return 0;
 
    for (i = 1; i < skiplimit && i < fsize - pos; i++) {
        fch = fbuf_at(fbf, pos + i);
        if (fch < 0) return i;
 
        if (fch == '\\') i++;
        else if (fch == quote) return i + 1;
    }
 
    return 1;
}
 
long fbuf_skip_quote_to (void * vfb, long pos, int skiplimit, void * vpat, int patlen)
{
    fbuf_t * fbf = (fbuf_t *)vfb;
    uint8  * pat = (uint8 *)vpat;
    int      fch = 0;
    long     fsize = 0;
    long     i = 0, qlen = 0;
    int      j = 0;
 
    if (!fbf) return -1;
    if (!pat || patlen <= 0) return pos;
 
    fsize = fbf->fsize - pos;
    if (skiplimit >= 0 && skiplimit < fsize)
        fsize = skiplimit;
 
    for (i = 0; i < fsize; ) {
        fch = fbuf_at(fbf, pos + i);
        if (fch < 0) break;
 
        if (fch == '\\' && i + 1 < fsize) {
            i += 2;
            continue;
        }
 
        for (j = 0; j < patlen; j++) {
            if (pat[j] == fch) return pos + i;
        }
         
        if (fch == '"' || fch == '\'') {
            qlen = fbuf_quotedstrlen(fbf, pos + i, fsize - i);
            i += qlen;
            continue;
        }
        i++;
    }
 
    return pos + i;
}
 
long fbuf_skip_esc_to (void * vfb, long pos, int skiplimit, void * vpat, int patlen)
{
    fbuf_t * fbf = (fbuf_t *)vfb;
    uint8  * pat = (uint8 *)vpat;
    int      fch = 0;
    long     i, fsize = 0;
    int      j = 0;
 
    if (!fbf) return -1;
    if (!pat || patlen <= 0) return pos;
 
    fsize = fbf->fsize - pos;
    if (skiplimit >= 0 && skiplimit < fsize)
        fsize = skiplimit;
 
    for (i = 0; i < fsize; i++) {
        fch = fbuf_at(fbf, pos + i);
        if (fch < 0) break;
 
        if (fch == '\\') {
            i++;
            continue;
        }
 
        for (j = 0; j < patlen; j++) {
            if (pat[j] == fch) return pos + i;
        }
    }
 
    return pos + i;
}

