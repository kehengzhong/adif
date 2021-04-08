/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include "btype.h"
#include "memory.h"
#include "nativefile.h"

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
 
int native_file_attr (void * vhfile, uint64 * msize, time_t * mtime, long * inode, uint32 * mimeid)
{
    NativeFile * hfile = (NativeFile *)vhfile;
     
    if (!hfile) return -1;
     
    if (msize) *msize = (uint64)hfile->size;
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


#ifdef _WIN32

extern void ansi_2_unicode (wchar_t * wcrt, char * pstr);

void  FileTimeToTime_t (FILETIME  ft, time_t * t)
{
    int64          ll;
    ULARGE_INTEGER ui;

    ui.LowPart  =  ft.dwLowDateTime;
    ui.HighPart =  ft.dwHighDateTime;
    ll =  ft.dwHighDateTime  <<  32  +  ft.dwLowDateTime;
    *t =  ((LONGLONG)(ui.QuadPart  -  116444736000000000)  /  10000000);
}


void * native_file_open (char * nfile, int flag)
{
    NativeFile * hfile = NULL;
    int          ret = 0;
    struct stat  st;

    if (!nfile) return NULL;

    memset(&st, 0, sizeof(st));
    ret = _stat(nfile, &st); 

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
    hfile->mode = S_IREAD | S_IWRITE ;
    if (flag & NF_EXEC) hfile->mode |= S_IEXEC;

    hfile->offset = 0;
    if (ret >= 0) {
        hfile->size = file_size(nfile);
        hfile->mtime = st.st_mtime;
        hfile->ctime = st.st_ctime;
        hfile->inode = st.st_ino;
    } else {
        hfile->size = 0;
        hfile->mtime = 0;
        hfile->ctime = 0;
        hfile->inode = 0;
    }

    if ((flag & NF_MASK_R) && (flag & NF_MASK_W)) hfile->oflag = O_RDWR;
    else {
        if (flag & NF_READPLUS || flag & NF_WRITEPLUS) hfile->oflag = O_RDWR;
        else if (flag & NF_READ) hfile->oflag = O_RDONLY;
        else if (flag & NF_WRITE) hfile->oflag = O_WRONLY;
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

int native_file_attr (void * vhfile, uint64 * msize, time_t * mtime, long * inode, uint32 * mimeid)
{
    NativeFile * hfile = (NativeFile *)vhfile;

    if (!hfile) return -1;

    if (msize) *msize = (uint64)hfile->size;
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
    for (len=0; len < size; ) {
        ret = read(hfile->fd, pbuf+len, size-len);
        if (ret < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                Sleep(0.5); //sleep 0.5 milli-seconds
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

    for (len=0; len < size; ) { 
        ret = write(hfile->fd, pbuf+len, size-len);
        if (ret < 0) { 
            if (errno == EINTR || errno == EAGAIN) {
                Sleep(0.5);
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
        fpos = _lseeki64(hfile->fd, offset, SEEK_SET);
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

int native_file_resize (void * vhfile, int64 newsize)
{
    NativeFile * hfile = (NativeFile *)vhfile;
    int          ret = 0;
    int          times = 0;

    if (!hfile) return -1;
    if (newsize == hfile->size) return 0;

    EnterCriticalSection(&hfile->fileCS);

    for (times = 0; times < 3; times++) {
        ret = _chsize_s(hfile->fd, newsize);
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

    ret = _stat(nfile, &st);
    if (ret < 0) return ret;

    if (stbuf) *stbuf = st;
    return ret;
}

int native_file_remove (char * nfile)
{
    if (!nfile || strlen(nfile) < 0)
        return -1;

    return DeleteFile(nfile);
}


#if 0
/*void * native_file_open   (char * nfile, int flag)
{
    NativeFile * hfile = NULL;
    int          ret = 0;
    struct stat  st;

    if (!nfile) return NULL;

    memset(&st, 0, sizeof(st));
    ret = _stat(nfile, &st); 

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
    //hfile->mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    //if (flag & NF_EXEC) hfile->mode |= (S_IXUSR | S_IXGRP | S_IXOTH);

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

//     if ((flag & NF_MASK_R) && (flag & NF_MASK_W)) hfile->oflag = O_RDWR;
//     else {
//         if (flag & NF_READPLUS || flag & NF_WRITEPLUS) hfile->oflag = O_RDWR;
//         else if (flag & NF_READ) hfile->oflag = O_RDONLY;
//         else if (flag & NF_WRITE) hfile->oflag = O_WRONLY;
//     }

    if (ret < 0) { //file not exist, need create it
        hfile->hfile = CreateFile(nfile,GENERIC_READ | GENERIC_WRITE,FILE_SHARE_READ,NULL,
            CREATE_ALWAYS, NULL, NULL);
    } else {
        hfile->hfile = CreateFile(nfile,GENERIC_READ | GENERIC_WRITE,FILE_SHARE_READ,NULL,
            OPEN_EXISTING, NULL, NULL);
    }
    if (hfile->hfile < 0) {
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

    if (hfile->hfile >= 0) {
        CloseHandle(hfile->hfile);
        hfile->hfile = -1;
    }

    kfree(hfile);
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
    for (len=0; len < size; ) {
        ReadFile(hfile->hfile, pbuf+len, size-len, &ret, NULL);
        if (ret < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                Sleep(1); //sleep 0.5 milli-seconds
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

int native_file_write (void * vhfile, void * buf, int size)
{
    NativeFile * hfile = (NativeFile *)vhfile;
    int          ret = 0; 
    int          len = 0;

    if (!hfile) return -1;
    if (!buf) return -2; 
    if (size < 0) return -3;

    EnterCriticalSection(&hfile->fileCS);
    for (len=0; len < size; ) { 
        WriteFile(hfile->hfile, buf+len, size-len,&ret,NULL );
        if (ret < 0) { 
            if (errno == EINTR || errno == EAGAIN) {
                Sleep(1);
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
*/
#endif

#endif

