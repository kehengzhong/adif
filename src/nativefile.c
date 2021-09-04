/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include "btype.h"
#include "memory.h"
#include "nativefile.h"
#include "fileop.h"

#ifdef UNIX

void * native_file_open (char * nfile, int flag)
{
    NativeFile * hfile = NULL;
    int          ret = 0;
    struct stat  st;

    if (!nfile) return NULL;

    memset(&st, 0, sizeof(st));
    ret = lstat(nfile, &st); 

    if (ret < 0) {
        if ((flag & NF_MASK_RW) == NF_READ)
            return NULL; //only read existing file 
        if ((flag & NF_MASK_RW) == NF_READPLUS)
            return NULL;//not create it; read/write existing file
    } else { //file exist
        if ((flag & NF_MASK_RW) == NF_WRITE) {
            unlink(nfile);
            ret = -100;
        }
    }

    hfile = kzalloc(sizeof(*hfile));
    if (!hfile) return NULL;

    InitializeCriticalSection(&hfile->fileCS);

    strncpy(hfile->name, nfile, sizeof(hfile->name)-1);
    hfile->flag = flag;
    hfile->oflag = 0;

    hfile->mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    if (flag & NF_EXEC)
        hfile->mode |= (S_IXUSR | S_IXGRP | S_IXOTH);

    hfile->offset = 0;
    if (ret >= 0) {
        hfile->size = st.st_size;
        hfile->mtime = st.st_mtime;
        hfile->ctime = st.st_ctime;
        hfile->inode = st.st_ino;
    } else {
        hfile->size = 0;
        hfile->mtime = 0;
        hfile->ctime = 0;
        hfile->inode = 0;
    }

    if ((flag & NF_MASK_R) && (flag & NF_MASK_W)) {
        hfile->oflag = O_RDWR;
    } else {
        if (flag & NF_READPLUS || flag & NF_WRITEPLUS)
            hfile->oflag = O_RDWR;
        else if (flag & NF_READ)
            hfile->oflag = O_RDONLY;
        else if (flag & NF_WRITE)
            hfile->oflag = O_WRONLY;
    }

    if (ret < 0) { //file not exist, need create it
        hfile->oflag |= O_CREAT;
        hfile->fd = open(nfile, hfile->oflag, hfile->mode);
    } else {
        hfile->fd = open(nfile, hfile->oflag);
    }

    if (hfile->fd < 0) {
        kfree(hfile);
        return NULL;
    }

    return hfile;
}


void * native_file_create (char * nfile, int flag)
{
    return native_file_open(nfile, flag|NF_WRITE);
}

int native_file_close (void * vhfile)
{
    NativeFile * hfile = (NativeFile *)vhfile;

    if (!hfile) return -1;

    DeleteCriticalSection(&hfile->fileCS);

    if (hfile->fd >= 0) {
        close(hfile->fd);
        hfile->fd = -1;
    }

    kfree(hfile);
    return 0;
}
 
int native_file_attr (void * vhfile, int64 * msize, time_t * mtime, long * inode, uint32 * mimeid)
{
    NativeFile * hfile = (NativeFile *)vhfile;
     
    if (!hfile) return -1;
     
    if (msize) *msize = hfile->size;
    if (mtime) *mtime = hfile->mtime;
    if (inode) *inode = hfile->inode;
    if (mimeid) *mimeid = hfile->mimeid; 
    return 0;
}    

int native_file_read (void * vhfile, void * pbuf, int size)
{
    NativeFile * hfile = (NativeFile *)vhfile;
    int          ret = 0;
    int          len = 0;

    if (!hfile) return -1;
    if (!pbuf) return -2;
    if (size < 0) return -3;

    EnterCriticalSection(&hfile->fileCS);

    for (len = 0; len < size; ) {
        ret = read(hfile->fd, pbuf + len, size - len);
        if (ret < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                usleep(500); //sleep 0.5 milli-seconds
                continue;
            }
            LeaveCriticalSection(&hfile->fileCS);
            return -100;

        } else if (ret == 0) { //reach end of the file
            break;
        }

        len += ret;
        hfile->offset += ret;
    }

    LeaveCriticalSection(&hfile->fileCS);

    return len;
}

int native_file_write (void * vhfile, void * pbuf, int size)
{
    NativeFile * hfile = (NativeFile *)vhfile;
    int          ret = 0; 
    int          len = 0;
     
    if (!hfile) return -1;
    if (!pbuf) return -2; 
    if (size < 0) return -3;
     
    EnterCriticalSection(&hfile->fileCS);

    for (len = 0; len < size; ) { 
        ret = write(hfile->fd, pbuf + len, size - len);
        if (ret < 0) { 
            if (errno == EINTR || errno == EAGAIN) {
                usleep(500);
                continue;
            }
            LeaveCriticalSection(&hfile->fileCS);
            return -100;
        }

        len += ret;
        hfile->offset += ret;
    }

    if (hfile->offset > hfile->size)
        hfile->size = hfile->offset;

    LeaveCriticalSection(&hfile->fileCS);
 
    return len;
}

int native_file_seek (void * vhfile, int64 offset)
{
    NativeFile * hfile = (NativeFile *)vhfile;
    int          times = 0;
    int64        fpos = 0;

    if (!hfile) return -1;

    EnterCriticalSection(&hfile->fileCS); 
    if (offset < 0) offset = hfile->size;

    if (offset == hfile->offset) {
        LeaveCriticalSection(&hfile->fileCS); 
        return 0;
    }
 
    for (times=0; times<3; times++) {
        fpos = lseek(hfile->fd, offset, SEEK_SET);
        if (fpos < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
        }
        if (fpos < 0 || fpos != offset) {
            if (hfile->fd >= 0) close(hfile->fd);
            hfile->fd = open(hfile->name, hfile->oflag);
        } else break; 
    }
 
    if (fpos < 0 || fpos != offset) {
        if (errno == EBADF) {
            LeaveCriticalSection(&hfile->fileCS);
            return -100;
        } else {
            LeaveCriticalSection(&hfile->fileCS);
            return -101;
        }
    }

    hfile->offset = offset;

    LeaveCriticalSection(&hfile->fileCS);

    return 0;
}

int native_file_copy (void * vsrc, int64 offset, int64 length, void * vdst, int64 * actnum)
{
    NativeFile * hsrc = (NativeFile *)vsrc;
    NativeFile * hdst = (NativeFile *)vdst;

    if (!hsrc || !hdst) return -1;

    return filefd_copy(native_file_fd(hsrc), offset, length, native_file_fd(hdst), actnum);
}

int native_file_resize (void * vhfile, int64 newsize)
{
    NativeFile * hfile = (NativeFile *)vhfile;
    int          ret = 0;
    int          times = 0;

    if (!hfile) return -1;
    if (newsize == hfile->size) return 0;

    EnterCriticalSection(&hfile->fileCS);

    for (times = 0; times < 3; times++) {
        ret = ftruncate(hfile->fd, newsize);
        if (ret < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
        }
        if (ret < 0) {
            if (hfile->fd >= 0) close(hfile->fd);
            hfile->fd = open(hfile->name, hfile->oflag);
        } else break; 
    }
    
    if (ret < 0) {
        LeaveCriticalSection(&hfile->fileCS);
        return -100;
    }

    hfile->size = newsize;

    LeaveCriticalSection(&hfile->fileCS);

    return 0;
}

char * native_file_name (void * vhfile)
{
    NativeFile * hfile = (NativeFile *)vhfile;

    if (!hfile) return "";

    return hfile->name;
}

int native_file_fd (void * vhfile)
{
    NativeFile * hfile = (NativeFile *)vhfile;

    if (!hfile) return -1;

    return hfile->fd;
}

int64 native_file_size (void * vhfile)
{
    NativeFile * hfile = (NativeFile *)vhfile;

    if (!hfile) return 0;

    return hfile->size;
}

int64 native_file_offset (void * vhfile)
{
    NativeFile * hfile = (NativeFile *)vhfile;

    if (!hfile) return 0;

    return hfile->offset;
}

int native_file_eof (void *vhfile)
{
    NativeFile * hfile = (NativeFile *)vhfile;

    if (!hfile) return 0;

    return hfile->offset >= hfile->size ? 1:0;
}

int native_file_stat (char * nfile, struct stat * stbuf)
{
    struct stat   st;
    int           ret = 0;

    if (!nfile || strlen(nfile) < 0)
        return -1;

    ret = lstat(nfile, &st);
    if (ret < 0) return ret;

    if (stbuf) *stbuf = st;
    return ret;
}

int native_file_remove (char * nfile)
{
    if (!nfile || strlen(nfile) < 0)
        return -1;

    return unlink(nfile);
}

#endif

#if defined(_WIN32) || defined(_WIN64)

void * native_file_open   (char * nfile, int flag)
{
    NativeFile * hfile = NULL;
    int          ret = 0;
    WIN32_FILE_ATTRIBUTE_DATA  wfad;
    DWORD        accflag = GENERIC_READ;
    DWORD        shrmode = FILE_SHARE_READ;
 
    if (!nfile) return NULL;

    if (GetFileAttributesEx(nfile, GetFileExInfoStandard, &wfad) == 0) {
        ret = -10;
    }

    if (ret < 0) {
        if ((flag & NF_MASK_RW) == NF_READ)
            return NULL; //only read existing file
        if ((flag & NF_MASK_RW) == NF_READPLUS)
            return NULL;//not create it; read/write existing file
    } else { //file exist
        if ((flag & NF_MASK_RW) == NF_WRITE) {
            DeleteFile(nfile);
            ret = -100;
        }
    }
 
    if ((flag & NF_MASK_R) && (flag & NF_MASK_W)) {
        accflag = GENERIC_READ | GENERIC_WRITE;
        shrmode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    } else {
        if (flag & NF_READPLUS || flag & NF_WRITEPLUS) {
            accflag = GENERIC_READ | GENERIC_WRITE;
            shrmode = FILE_SHARE_READ | FILE_SHARE_WRITE;
        } else if (flag & NF_READ) {
            accflag = GENERIC_READ;
            shrmode = FILE_SHARE_READ;
        } else if (flag & NF_WRITE) {
            accflag = GENERIC_WRITE;
            shrmode = FILE_SHARE_WRITE;
        }
    }

    hfile = kzalloc(sizeof(*hfile));
    if (!hfile) return NULL;
 
    InitializeCriticalSection(&hfile->fileCS);
 
    strncpy(hfile->name, nfile, sizeof(hfile->name)-1);
    hfile->flag = flag;
    hfile->oflag = 0;
    //hfile->mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    //if (flag & NF_EXEC) hfile->mode |= (S_IXUSR | S_IXGRP | S_IXOTH);
 
    hfile->offset = 0;
    if (ret >= 0) {
        hfile->size = wfad.nFileSizeHigh;
        hfile->size <<= 32;
        hfile->size |= wfad.nFileSizeLow;

        hfile->mtime = FileTimeToUnixTime(wfad.ftLastWriteTime);
        hfile->ctime = FileTimeToUnixTime(wfad.ftCreationTime);
        hfile->inode = 0;
    } else {
        hfile->size = 0;
        hfile->mtime = 0;
        hfile->ctime = 0;
        hfile->inode = 0;
    }
 
    /* Opens the file, if it exists. If the file does not exist, the
       function creates the file as if this parameter were set to CREATE_NEW.
     */
    hfile->filehandle = CreateFile(nfile,
                              accflag, //GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, //shrmode,
                              NULL,
                              OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (hfile->filehandle == INVALID_HANDLE_VALUE) {
        kfree(hfile);
        return NULL;
    }
 
    hfile->fd = file_handle_to_fd(hfile->filehandle);

    return hfile;
}
 
void * native_file_create (char * nfile, int flag)
{
    return native_file_open(nfile, flag|NF_WRITE);
}
 
int native_file_close (void * vhfile)
{
    NativeFile * hfile = (NativeFile *)vhfile;
 
    if (!hfile) return -1;
 
    DeleteCriticalSection(&hfile->fileCS);
 
    if (hfile->filehandle >= 0) {
        CloseHandle(hfile->filehandle);
        hfile->filehandle = NULL;
    }
 
    kfree(hfile);
    return 0;
}

int native_file_attr (void * vhfile, int64 * msize, time_t * mtime, long * inode, uint32 * mimeid)
{
    NativeFile * hfile = (NativeFile *)vhfile;

    if (!hfile) return -1;

    if (msize) *msize = hfile->size;
    if (mtime) *mtime = hfile->mtime;
    if (inode) *inode = hfile->inode;
    if (mimeid) *mimeid = hfile->mimeid;
    return 0;
}

int native_file_read (void * vhfile, void * pbuf, int size)
{
    NativeFile * hfile = (NativeFile *)vhfile;
    DWORD        readlen = 0;
    BOOL         ret;
    int          len = 0;
 
    if (!hfile) return -1;
    if (!pbuf) return -2;
    if (size < 0) return -3;
 
    EnterCriticalSection(&hfile->fileCS);
 
    for (len = 0; len < size; ) {
        ret = ReadFile(hfile->filehandle, (uint8 *)pbuf + len, size - len, &readlen, NULL);
        if (readlen == 0 && ret != 0) {
            /* If the return value is nonzero and the number of bytes read
               is zero, the file pointer was beyond the current end of the
               file at the time of the read operation. */
            break;
        }
        if (ret == 0) {
            /* return value is nonzero indicates success. Zero indicates failure. */
            LeaveCriticalSection(&hfile->fileCS);
            return -100;
        }
 
        len += readlen;
        hfile->offset += readlen;
    }
 
    LeaveCriticalSection(&hfile->fileCS);
 
    return len;
}
 
int native_file_write (void * vhfile, void * buf, int size)
{
    NativeFile * hfile = (NativeFile *)vhfile;
    BOOL         ret;
    DWORD        writelen = 0;
    int          len = 0;
 
    if (!hfile) return -1;
    if (!buf) return -2;
    if (size < 0) return -3;
 
    EnterCriticalSection(&hfile->fileCS);
 
    for (len = 0; len < size; ) {
        ret = WriteFile(hfile->filehandle, (uint8 *)buf + len, size - len, &writelen, NULL);
        /* return value: Nonzero indicates success. Zero indicates failure. */
 
        if (ret == 0) {
            LeaveCriticalSection(&hfile->fileCS);
            return -100;
        }
        len += writelen;
        hfile->offset += writelen;
    }
 
    if (hfile->offset > hfile->size)
        hfile->size = hfile->offset;
 
    LeaveCriticalSection(&hfile->fileCS);
 
    return len;
}
 
int native_file_seek (void * vhfile, int64 offset)
{
    NativeFile    * hfile = (NativeFile *)vhfile;
    DWORD           dwRet;
    LARGE_INTEGER   liPos;

    if (!hfile) return -1;

    liPos.QuadPart = offset;

    EnterCriticalSection(&hfile->fileCS);
 
    if (offset < 0) offset = hfile->size;
 
    if (offset == hfile->offset) {
        LeaveCriticalSection(&hfile->fileCS);
        return 0;
    }
 
    dwRet = SetFilePointer(hfile->filehandle, liPos.LowPart, &liPos.HighPart, FILE_BEGIN);
    if( dwRet == ((DWORD)-1) /*INVALID_SET_FILE_POINTER*/ && GetLastError() != NO_ERROR ){
        return -100;
    }
 
    hfile->offset = offset;
 
    LeaveCriticalSection(&hfile->fileCS);
 
    return 0;
}

int native_file_copy (void * vsrc, int64 offset, int64 length, void * vdst, int64 * actnum)
{
    NativeFile * hsrc = (NativeFile *)vsrc;
    NativeFile * hdst = (NativeFile *)vdst;
    int64        fsize = 0;

    static int   mmapsize = 8192 * 1024;
    void       * pbyte = NULL;
    void       * pmap = NULL;
    HANDLE       hmap = NULL;
    int64        maplen = 0;
    int64        mapoff = 0;

    int          onelen = 0;
    int64        wlen = 0;
    int          wbytes = 0;

    int          ret = 0;
    int          errcode = 0;

    if (actnum) *actnum = 0;

    if (!hsrc || !hdst) return -1;

    fsize = native_file_size(hsrc);

    if (offset < 0) offset = 0;
    if (offset >= fsize)
        return -100;

    if (length < 0)
        length = fsize - offset;
    else if (length > fsize - offset)
        length = fsize - offset;

    for (wlen = 0; wlen < length; ) {
        onelen = length - wlen;
        if (onelen > mmapsize) onelen = mmapsize;

        pbyte = file_mmap(NULL, native_file_handle(hsrc), offset + wlen,
                          onelen, NULL, &hmap, &pmap, &maplen, &mapoff);
        if (!pbyte) break;

        for (wbytes = 0; wbytes < onelen; ) {
            ret = native_file_write(hdst, (uint8 *)pbyte + wbytes, onelen - wbytes);
            if (ret > 0) {
                wbytes += ret;
                wlen += ret;
                continue;
            }

            file_munmap(hmap, pmap);

            if (actnum) *actnum = wlen;
            return ret;
        } //end for (wbytes = 0; wbytes < onelen; )

        file_munmap(hmap, pmap);
    }

    if (actnum) *actnum = wlen;

    return (int)wlen;
}

int native_file_resize (void * vhfile, int64 newsize)
{
    NativeFile    * hfile = (NativeFile *)vhfile;
    DWORD           dwRet;
    LARGE_INTEGER   liPos;
 
    if (!hfile) return -1;
 
    if (newsize == hfile->size) return 0;
 
    EnterCriticalSection(&hfile->fileCS);
 
    liPos.QuadPart = newsize;
 
    dwRet = SetFilePointer(hfile->filehandle, liPos.LowPart, &liPos.HighPart, FILE_BEGIN);
    if( dwRet == ((DWORD)-1) /*INVALID_SET_FILE_POINTER*/ && GetLastError() != NO_ERROR ){
        return -1;
    }
 
    SetEndOfFile(hfile->filehandle);
 
    LeaveCriticalSection(&hfile->fileCS);
 
    return 0;
}

char * native_file_name (void * vhfile)
{
    NativeFile * hfile = (NativeFile *)vhfile;
 
    if (!hfile) return "";
 
    return hfile->name;
}

HANDLE native_file_handle (void * vhfile)
{
    NativeFile * hfile = (NativeFile *)vhfile;
 
    if (!hfile) return INVALID_HANDLE_VALUE;
 
    return hfile->filehandle;
}
 
int native_file_fd (void * vhfile)
{
    NativeFile * hfile = (NativeFile *)vhfile;
 
    if (!hfile) return -1;
 
    return hfile->fd;
}
 
int64 native_file_size (void * vhfile)
{
    NativeFile * hfile = (NativeFile *)vhfile;
 
    if (!hfile) return 0;
 
    return hfile->size;
}
 
int64 native_file_offset (void * vhfile)
{
    NativeFile * hfile = (NativeFile *)vhfile;
 
    if (!hfile) return 0;
 
    return hfile->offset;
}
 
int native_file_eof (void *vhfile)
{
    NativeFile * hfile = (NativeFile *)vhfile;
 
    if (!hfile) return 0;
 
    return hfile->offset >= hfile->size ? 1:0;
}
 
int native_file_stat (char * nfile, struct stat * stbuf)
{
    return file_stat(nfile, stbuf);
}
 
int native_file_remove (char * nfile)
{
    if (!nfile || strlen(nfile) < 0)
        return -1;
 
    return DeleteFile(nfile);
}

#endif

