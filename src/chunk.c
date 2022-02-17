/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#include "btype.h"
#include "memory.h"
#include "dynarr.h"
#include "fileop.h"
#include "frame.h"
#include "nativefile.h"
#include "strutil.h"
#include "chunk.h"
#include "patmat.h"
#include "tsock.h"
#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#endif

static char * chunk_end_flag = "0\r\n\r\n";

int size_hex_len (int64 size)
{
    int n = 0;

    if (size < 0) {
        size = -size;
        n = 1;
    }

    if (size <= 16) return n + 1;

    for ( ; size != 0; n++)
        size >>= 4;

    return n;
}

void chunk_entity_free (void * pent)
{
    ckent_t  * ent = (ckent_t *)pent;

    if (!ent) return;

    if (ent->cktype == CKT_BUFFER) {
        if (ent->u.buf.pbyte) {
            kfree(ent->u.buf.pbyte);
            ent->u.buf.pbyte = NULL;
        }
 
    } else if (ent->cktype == CKT_FILE_NAME) {
        if (ent->u.filename.pmap) {
#ifdef UNIX
            file_munmap(ent->u.filename.pmap, ent->u.filename.maplen);
#endif
#if defined(_WIN32) || defined(_WIN64)
            file_munmap(ent->u.filename.hmap, ent->u.filename.pmap);
            ent->u.filename.hmap = NULL;
#endif
            ent->u.filename.pbyte = NULL;
            ent->u.filename.pmap = NULL;
            ent->u.filename.maplen = 0;
            ent->u.filename.mapoff = 0;
        }

        if (ent->u.filename.hfile) {
            native_file_close(ent->u.filename.hfile);
            ent->u.filename.hfile = NULL;
        }

        if (ent->u.filename.fname) {
            kfree(ent->u.filename.fname);
            ent->u.filename.fname = NULL;
        }
 
    } else if (ent->cktype == CKT_FILE_PTR) {
        if (ent->u.fileptr.pmap) {
#ifdef UNIX
            file_munmap(ent->u.fileptr.pmap, ent->u.fileptr.maplen);
#endif
#if defined(_WIN32) || defined(_WIN64)
            file_munmap(ent->u.fileptr.hmap, ent->u.fileptr.pmap);
            ent->u.fileptr.hmap = NULL;
#endif
            ent->u.fileptr.pbyte = NULL;
            ent->u.fileptr.pmap = NULL;
            ent->u.fileptr.maplen = 0;
            ent->u.fileptr.mapoff = 0;
        }

    } else if (ent->cktype == CKT_FILE_DESC) {
        if (ent->u.filefd.pmap) {
#ifdef UNIX
            file_munmap(ent->u.filefd.pmap, ent->u.filefd.maplen);
#endif
#if defined(_WIN32) || defined(_WIN64)
            file_munmap(ent->u.filefd.hmap, ent->u.filefd.pmap);
            ent->u.filefd.hmap = NULL;
#endif
            ent->u.filefd.pbyte = NULL;
            ent->u.filefd.pmap = NULL;
            ent->u.filefd.maplen = 0;
            ent->u.filefd.mapoff = 0;
        }

    } else if (ent->cktype == CKT_CALLBACK) {
        if (ent->u.callback.endfunc) {
            (*ent->u.callback.endfunc)(ent->u.callback.endpara, 0);
        }
    }

    kfree(ent);
}


void * chunk_new (int buflen)
{
    chunk_t * ck = NULL;

    ck = kzalloc(sizeof(*ck));
    if (!ck) return NULL;

    ck->entity_list = arr_new(8);
    ck->tail_entity_list = arr_new(2);

    ck->httpchunk = 0;
    ck->rmchunklen = 0;
    ck->chunksize = 5;  //0\r\n\r\n
    ck->chunkendsize = -1;

    ck->rmentlen = 0;
    ck->size = 0;
    ck->endsize = -1;

    ck->filenum = 0;
    ck->bufnum = 0;

    ck->seekpos = 0;

    ck->lblen = 0;
    ck->lbsize = buflen;
    if (ck->lbsize > 0) {
        ck->lbsize = (ck->lbsize + 1023)/1024 * 1024;
    }

    return ck;
}

void chunk_free (void * vck)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int        i, num;

    if (!ck) return;

    if (ck->loadbuf) {
        kfree(ck->loadbuf);
        ck->loadbuf = NULL;
    }

    num = arr_num(ck->entity_list);
    for (i = 0; i < num; i++) {
        ent = arr_value(ck->entity_list, i);
        if (!ent) continue;

        chunk_entity_free(ent);
    }
    arr_free(ck->entity_list);

    arr_pop_free(ck->tail_entity_list, chunk_entity_free);

    kfree(ck);
}

 
void chunk_zero (void * vck)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int        i, num;

    if (!ck) return;

    num = arr_num(ck->entity_list);
    for (i = 0; i < num; i++) {
        ent = arr_value(ck->entity_list, i);
        if (!ent) continue;
 
        chunk_entity_free(ent);
    }
    arr_zero(ck->entity_list);

    num = arr_num(ck->tail_entity_list);
    for (i = 0; i < num; i++) {
        ent = arr_value(ck->tail_entity_list, i);
        if (!ent) continue;

        chunk_entity_free(ent);
    }
    arr_zero(ck->tail_entity_list);

    ck->httpchunk = 0;
    ck->rmchunklen = 0;
    ck->chunksize = 5;  //0\r\n\r\n
    ck->chunkendsize = -1;

    ck->rmentlen = 0;
    ck->size = 0;
    ck->endsize = -1;

    ck->filenum = 0;
    ck->bufnum = 0;

    ck->seekpos = 0;

    ck->lblen = 0;

    ck->procnotify = NULL;
    ck->procnotifypara = NULL;
    ck->procnotifycbval = 0;
}

 
int64 chunk_size (void * vck, int httpchunk)
{
    chunk_t  * ck = (chunk_t *)vck;

    if (!ck) return 0;

    return httpchunk > 0 ? ck->chunksize : ck->size;
}

int64 chunk_rest_size (void * vck, int httpchunk)
{
    chunk_t  * ck = (chunk_t *)vck;
 
    if (!ck) return 0;
 
    return httpchunk > 0 ? ck->chunksize - ck->rmchunklen: ck->size - ck->rmentlen;
}

int64 chunk_startpos (void * vck, int httpchunk)
{
    chunk_t  * ck = (chunk_t *)vck;
 
    if (!ck) return 0;
 
    return httpchunk > 0 ? ck->rmchunklen: ck->rmentlen;
}
 
int64 chunk_seekpos (void * vck)
{
    chunk_t  * ck = (chunk_t *)vck;

    if (!ck) return 0;

    return ck->seekpos;
}

int64 chunk_seek (void * vck, int64 offset)
{
    chunk_t  * ck = (chunk_t *)vck;

    if (!ck) return 0;

    if (offset < 0) offset = 0;
    if (offset > ck->size) offset = ck->size;

    ck->seekpos = offset; 
    return offset;
}
 
int chunk_is_file (void * vck, int64 * fsize, time_t * mtime, long * inode, char ** fname)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int        filenum = 0;
    int        bufnum = 0;
    int        fullfile = 0;
    int        i = 0;

    if (!ck) return 0;

    for (i = 0; i < arr_num(ck->entity_list); i++) {
        ent = arr_value(ck->entity_list, i);
        if (!ent || ent->header) continue;

        if (ent->cktype == CKT_FILE_NAME) {
            if (ent->length == ent->u.filename.fsize && ent->u.filename.offset == 0) {
                if (fsize) *fsize = ent->u.filename.fsize;
                if (mtime) *mtime = ent->u.filename.mtime;
                if (inode) *inode = ent->u.filename.inode;
                if (fname) *fname = ent->u.filename.fname;
                fullfile++;
            }
            filenum++;
    
        } else if (ent->cktype == CKT_FILE_PTR) {
            if (ent->length == ent->u.fileptr.fsize && ent->u.fileptr.offset == 0) {
                if (fsize) *fsize = ent->u.fileptr.fsize;
                if (mtime) *mtime = ent->u.fileptr.mtime;
                if (inode) *inode = ent->u.fileptr.inode;
                fullfile++;
            }
            filenum++;
    
        } else if (ent->cktype == CKT_FILE_DESC) {
            if (ent->length == ent->u.filefd.fsize && ent->u.filefd.offset == 0) {
                if (fsize) *fsize = ent->u.filefd.fsize;
                if (mtime) *mtime = ent->u.filefd.mtime;
                if (inode) *inode = ent->u.filefd.inode;
                fullfile++;
            }
            filenum++;

        } else {
            bufnum++;
        }
    }

    if (fullfile > 0 && filenum > 0 && bufnum <= 0)
        return 1;

    return 0;
}


int chunk_num (void * vck)
{
    chunk_t  * ck = (chunk_t *)vck;

    if (!ck) return 0;

    return arr_num(ck->entity_list);
}

int chunk_has_file (void * vck)
{
    chunk_t  * ck = (chunk_t *)vck;

    if (!ck) return 0;

    return ck->filenum;
}

int chunk_has_buf (void * vck)
{
    chunk_t  * ck = (chunk_t *)vck;

    if (!ck) return 0;

    return ck->bufnum;
}

int chunk_get_end (void * vck, int64 pos, int httpchunk)
{
    chunk_t  * ck = (chunk_t *)vck;

    if (!ck) return 0;

    if (httpchunk) {
        if (ck->chunkendsize <= 0) return 0;
        if (pos < ck->chunkendsize) return 0;

    } else {
        if (ck->endsize <= 0) return 0;
        if (pos < ck->endsize) return 0;
    }

    return 1;
}

int chunk_set_end (void * vck)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int        i, num;

    if (!ck) return 0;

    num = arr_num(ck->tail_entity_list);
    for (i = 0; i < num; i++) {
        ent = arr_value(ck->tail_entity_list, i);
        if (!ent) continue;

        arr_push(ck->entity_list, ent);

        switch (ent->cktype) {
        case CKT_BUFFER_PTR:
        case CKT_BUFFER:
        case CKT_CHAR_ARRAY:
            ck->bufnum++;
            break;
        case CKT_FILE_NAME:
        case CKT_FILE_PTR:
        case CKT_FILE_DESC:
            ck->filenum++;
            break;
        }
        ck->size += ent->length;
        ck->chunksize += ent->length + ent->lenstrlen + ent->trailerlen;
    }

    arr_zero(ck->tail_entity_list);

    ck->endsize = ck->size;
    ck->chunkendsize = ck->chunksize;

    return 1;
}

void chunk_set_httpchunk (void * vck, int httpchunk)
{
    chunk_t  * ck = (chunk_t *)vck;

    if (!ck) return;

    ck->httpchunk = httpchunk > 0 ? 1 : 0;

    return;
}

int chunk_attr (void * vck, int index, int * cktype, int64 * plen)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int        num;

    if (!ck) return -1;
    if (index < 0) return -2;

    num = arr_num(ck->entity_list);
    if (index >= num) return -3;

    ent = arr_value(ck->entity_list, index);
    if (!ent) return -100;

    if (cktype) *cktype = ent->cktype;
    if (plen) *plen = ent->length;
    return 0;
}

int64 chunk_read (void * vck, void * pbuf, int64 offset, int64 length, int httpchunk)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int        i, num, ret;

    int64      accentlen = 0;

    int64      readpos = 0;
    int64      readlen = 0;
    int64      curpos = 0;
    int64      curlen = 0;

    if (!ck) return -1;

    if (httpchunk) {
        if (offset < ck->rmchunklen)
            offset = ck->rmchunklen;

        if (offset >= ck->chunksize) return -3;
 
        if (length < 0)
            length = ck->chunksize - offset;

        if (length > ck->chunksize - offset)
            length = ck->chunksize - offset;
 
        accentlen = ck->rmchunklen;
 
    } else {
        if (offset < ck->rmentlen)
            offset = ck->rmentlen;

        if (offset >= ck->size) return -3;

        if (length < 0)
            length = ck->size - offset; 

        if (length > ck->size - offset)
            length = ck->size - offset; 

        accentlen = ck->rmentlen;
    }

    readpos = offset;
    readlen = 0;
    ck->seekpos = offset;

    num = arr_num(ck->entity_list);
    for (i = 0; i < num; i++) {

        ent = arr_value(ck->entity_list, i);
        if (!ent) continue;

        if (httpchunk) {
            if (ent->lenstrlen > 0 && readpos < accentlen + ent->lenstrlen) {
                curpos = readpos - accentlen;
                curlen = ent->lenstrlen - curpos;
                if (curlen > length - readlen)
                    curlen = length - readlen;
 
                memcpy(pbuf, ent->lenstr + curpos, curlen);
 
                readlen += curlen;
                readpos += curlen;
                if (readlen >= length) {
                    ck->seekpos = readpos;
                    return readlen;
                }
            }
            accentlen += ent->lenstrlen;
        }

        if (ent->length > 0 && readpos < accentlen + ent->length) {
            curpos = readpos - accentlen;
            curlen = ent->length - curpos;
            if (curlen > length - readlen)
                curlen = length - readlen;

            if (ent->cktype == CKT_CHAR_ARRAY) {
                memcpy(pbuf, ent->u.charr.pbyte + curpos, curlen);

            } else if (ent->cktype == CKT_BUFFER) {
                memcpy(pbuf, (uint8 *)ent->u.buf.pbyte + curpos, curlen);

            } else if (ent->cktype == CKT_BUFFER_PTR) {
                memcpy(pbuf, (uint8 *)ent->u.bufptr.pbyte + curpos, curlen);

            } else if (ent->cktype == CKT_FILE_NAME) {
                if (ent->u.filename.hfile == NULL) {
                    ent->u.filename.hfile = native_file_open(ent->u.filename.fname, NF_READ);

                    if (ent->u.filename.fsize != native_file_size(ent->u.filename.hfile))
                        file_attr(ent->u.filename.fname,
                                  &ent->u.filename.inode,
                                  &ent->u.filename.fsize, NULL,
                                  &ent->u.filename.mtime, NULL);
                }

                native_file_seek(ent->u.filename.hfile, ent->u.filename.offset + curpos);

                ret = native_file_read(ent->u.filename.hfile, pbuf, curlen);
                if (ret >= 0) curlen = ret;

            } else if (ent->cktype == CKT_FILE_PTR) {
                file_seek(ent->u.fileptr.fp, ent->u.fileptr.offset + curpos, SEEK_SET);
                ret = file_read(ent->u.fileptr.fp, pbuf, curlen);
                if (ret >= 0) curlen = ret;

            } else if (ent->cktype == CKT_FILE_DESC) {
                lseek(ent->u.filefd.fd, ent->u.filefd.offset + curpos, SEEK_SET);
                ret = filefd_read(ent->u.filefd.fd, pbuf, curlen);
                if (ret >= 0) curlen = ret;

            } else if (ent->cktype == CKT_CALLBACK) {
                void   * pbyte = NULL;
                int64    bytelen = 0;
                int64    onelen = 0;

                while (ent->u.callback.fetchfunc && onelen < curlen) {
                    ret = (*ent->u.callback.fetchfunc)(ent->u.callback.fetchpara, 
                                                       ent->u.callback.offset + curpos + onelen,
                                                       curlen - onelen,
                                                       &pbyte, &bytelen);
                    if (ret >= 0) {
                        memcpy((uint8 *)pbuf + onelen, pbyte, bytelen);
                        onelen += bytelen;
                        continue;
                    }
                    break;
                }
                curlen = onelen;
            }

            readlen += curlen;
            readpos += curlen;
            if (readlen >= length) {
                ck->seekpos = readpos;
                return readlen;
            }
        }
        accentlen += ent->length;

        if (httpchunk) {
            if (ent->trailerlen > 0 && readpos < accentlen + ent->trailerlen) {
                curpos = readpos - accentlen;
                curlen = ent->trailerlen - curpos;
                if (curlen > length - readlen)
                    curlen = length - readlen;
 
                memcpy(pbuf, ent->trailer + curpos, curlen);
 
                readlen += curlen;
                readpos += curlen;
                if (readlen >= length) {
                    ck->seekpos = readpos;
                    return readlen;
                }
            }
            accentlen += ent->trailerlen;
        }
    }

    if (httpchunk && ck->chunkendsize > 0 && readpos + 5 >= ck->chunkendsize) {
        if (readpos < accentlen + 5) { //0\r\n\r\n
            curpos = readpos - accentlen;
            curlen = 5 - curpos;
            if (curlen > length - readlen)
                curlen = length - readlen;
 
            memcpy(pbuf, chunk_end_flag + curpos, curlen);
 
            readlen += curlen;
            readpos += curlen;
            if (readlen >= length) {
                ck->seekpos = readpos;
                return readlen;
            }
        }
        accentlen += 5;
    }

    ck->seekpos = readpos;
    return readlen;
}

int64 chunk_read_ptr (void * vck, int64 offset, int64 length, void ** ppbyte, int64 * pbytelen, int httpchunk)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int        i, num, ret;
 
    int64      accentlen = 0;
 
    int64      readpos = 0;
    int64      readlen = 0;
    int64      curpos = 0;
    int64      curlen = 0;
 
    if (!ck) return -1;
 
    if (httpchunk) {
        if (offset < ck->rmchunklen)
            offset = ck->rmchunklen;

        if (offset >= ck->chunksize) return -3;
 
        if (length < 0)
            length = ck->chunksize - offset;

        if (length > ck->chunksize - offset)
            length = ck->chunksize - offset;
 
        accentlen = ck->rmchunklen;
 
    } else {
        if (offset < ck->rmentlen)
            offset = ck->rmentlen;

        if (offset >= ck->size) return -3;
 
        if (length < 0)
            length = ck->size - offset;

        if (length > ck->size - offset)
            length = ck->size - offset;
 
        accentlen = ck->rmentlen;
    }

    readpos = offset;
    readlen = 0;
    ck->seekpos = offset;

    num = arr_num(ck->entity_list);
    for (i = 0; i < num; i++) {

        ent = arr_value(ck->entity_list, i);
        if (!ent) continue;
 
        if (httpchunk) {
            if (ent->lenstrlen > 0 && readpos < accentlen + ent->lenstrlen) {
                curpos = readpos - accentlen;
                curlen = ent->lenstrlen - curpos;
                if (length > 0 && curlen > length - readlen)
                    curlen = length - readlen;
 
                if (ppbyte) *ppbyte = ent->lenstr + curpos;
                if (pbytelen) *pbytelen = curlen;

                return curlen;
            }
            accentlen += ent->lenstrlen;
        }

        if (ent->length > 0 && readpos < accentlen + ent->length) {
            curpos = readpos - accentlen;
            curlen = ent->length - curpos;
            if (length > 0 && curlen > length - readlen)
                curlen = length - readlen;
 
            if (ent->cktype == CKT_CHAR_ARRAY) {
                if (ppbyte) *ppbyte = ent->u.charr.pbyte + curpos;
                if (pbytelen) *pbytelen = curlen;

                return curlen;

            } else if (ent->cktype == CKT_BUFFER) {
                if (ppbyte) *ppbyte = (uint8 *)ent->u.buf.pbyte + curpos;
                if (pbytelen) *pbytelen = curlen;

                return curlen;

            } else if (ent->cktype == CKT_BUFFER_PTR) {
                if (ppbyte) *ppbyte = (uint8 *)ent->u.bufptr.pbyte + curpos;
                if (pbytelen) *pbytelen = curlen;

                return curlen;

            } else if (ent->cktype == CKT_FILE_NAME) {
                if (ent->u.filename.hfile == NULL) {
                    ent->u.filename.hfile = native_file_open(ent->u.filename.fname, NF_READ);

                    if (ent->u.filename.fsize != native_file_size(ent->u.filename.hfile))
                        file_attr(ent->u.filename.fname,
                                  &ent->u.filename.inode,
                                  &ent->u.filename.fsize, NULL,
                                  &ent->u.filename.mtime, NULL);
                }

                native_file_seek(ent->u.filename.hfile, ent->u.filename.offset + curpos);
                if (curlen > ck->lbsize) curlen = ck->lbsize;

                if (!ck->loadbuf) ck->loadbuf = kalloc(ck->lbsize);
                ret = native_file_read(ent->u.filename.hfile, ck->loadbuf, curlen);
                if (ret >= 0) curlen = ret;

                if (ppbyte) *ppbyte = ck->loadbuf;
                if (pbytelen) *pbytelen = curlen;

                return curlen;

            } else if (ent->cktype == CKT_FILE_PTR) {
                file_seek(ent->u.fileptr.fp, ent->u.fileptr.offset + curpos, SEEK_SET);
                if (curlen > ck->lbsize) curlen = ck->lbsize;

                if (!ck->loadbuf) ck->loadbuf = kalloc(ck->lbsize);
                ret = file_read(ent->u.fileptr.fp, ck->loadbuf, curlen);
                if (ret >= 0) curlen = ret;

                if (ppbyte) *ppbyte = ck->loadbuf;
                if (pbytelen) *pbytelen = curlen;

                return curlen;

            } else if (ent->cktype == CKT_FILE_DESC) {
                lseek(ent->u.filefd.fd, ent->u.filefd.offset + curpos, SEEK_SET);
                if (curlen > ck->lbsize) curlen = ck->lbsize;

                if (!ck->loadbuf) ck->loadbuf = kalloc(ck->lbsize);
                ret = filefd_read(ent->u.filefd.fd, ck->loadbuf, curlen);
                if (ret >= 0) curlen = ret;

                if (ppbyte) *ppbyte = ck->loadbuf;
                if (pbytelen) *pbytelen = curlen;

                return curlen;

            } else if (ent->cktype == CKT_CALLBACK) {
                if (ent->u.callback.fetchfunc) {
                    ret = (*ent->u.callback.fetchfunc)(ent->u.callback.fetchpara,
                                                       ent->u.callback.offset + curpos,
                                                       curlen, ppbyte, pbytelen);
                    curlen = *pbytelen;
                    return curlen;
                }
            }
 
            readlen += curlen;
            readpos += curlen;
        }
        accentlen += ent->length;

        if (httpchunk) {
            if (ent->trailerlen > 0 && readpos < accentlen + ent->trailerlen) {
                curpos = readpos - accentlen;
                curlen = ent->trailerlen - curpos;
                if (length > 0 && curlen > length - readlen)
                    curlen = length - readlen;
 
                if (ppbyte) *ppbyte = ent->trailer + curpos;
                if (pbytelen) *pbytelen = curlen;
 
                return curlen;
            }
            accentlen += ent->trailerlen;
        }
    }
 
    if (httpchunk && ck->chunkendsize > 0 && readpos + 5 >= ck->chunkendsize) {
        if (readpos < accentlen + 5) { //0\r\n\r\n
            curpos = readpos - accentlen;
            curlen = 5 - curpos;
            if (length > 0 && curlen > length - readlen)
                curlen = length - readlen;
 
            if (ppbyte) *ppbyte = chunk_end_flag + curpos;
            if (pbytelen) *pbytelen = curlen;
 
            return curlen;
        }
        accentlen += 5;
    }

    return readlen;
}

int64 chunk_readto_frame (void * vck, frame_p frm, int64 offset, int64 length, int httpchunk)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int        i, num, ret;

    int64      accentlen = 0;

    int64      readpos = 0;
    int64      readlen = 0;
    int64      curpos = 0;
    int64      curlen = 0;

    if (!ck) return -1;

    if (httpchunk) {
        if (offset < ck->rmchunklen)
            offset = ck->rmchunklen;

        if (offset >= ck->chunksize) return -3;

        if (length < 0) length = ck->chunksize - offset;

        if (length > ck->chunksize - offset)
            length = ck->chunksize - offset; 

        accentlen = ck->rmchunklen;

    } else {
        if (offset < ck->rmentlen)
            offset = ck->rmentlen;

        if (offset >= ck->size) return -3;
 
        if (length < 0) length = ck->size - offset;

        if (length > ck->size - offset)
            length = ck->size - offset;
 
        accentlen = ck->rmentlen;
    }

    readpos = offset;
    readlen = 0;
    ck->seekpos = offset;

    num = arr_num(ck->entity_list);
    for (i = 0; i < num; i++) {

        ent = arr_value(ck->entity_list, i);
        if (!ent) continue;

        if (httpchunk) {
            if (ent->lenstrlen > 0 && readpos < accentlen + ent->lenstrlen) {
                curpos = readpos - accentlen;
                curlen = ent->lenstrlen - curpos;
                if (curlen > length - readlen)
                    curlen = length - readlen;

                frame_put_nlast(frm, ent->lenstr + curpos, curlen);

                readlen += curlen;
                readpos += curlen;
                if (readlen >= length) {
                    ck->seekpos = readpos;
                    return readlen;
                }
            }
            accentlen += ent->lenstrlen;
        }

        if (ent->length > 0 && readpos < accentlen + ent->length) {
            curpos = readpos - accentlen;
            curlen = ent->length - curpos;
            if (curlen > length - readlen)
                curlen = length - readlen;

            if (ent->cktype == CKT_CHAR_ARRAY) {
                frame_put_nlast(frm, ent->u.charr.pbyte + curpos, curlen);

            } else if (ent->cktype == CKT_BUFFER) {
                frame_put_nlast(frm, (uint8 *)ent->u.buf.pbyte + curpos, curlen);

            } else if (ent->cktype == CKT_BUFFER_PTR) {
                frame_put_nlast(frm, (uint8 *)ent->u.bufptr.pbyte + curpos, curlen);

            } else if (ent->cktype == CKT_FILE_NAME) {
                if (ent->u.filename.hfile == NULL) {
                    ent->u.filename.hfile = native_file_open(ent->u.filename.fname, NF_READ);

                    if (ent->u.filename.fsize != native_file_size(ent->u.filename.hfile))
                        file_attr(ent->u.filename.fname,
                                  &ent->u.filename.inode,
                                  &ent->u.filename.fsize, NULL,
                                  &ent->u.filename.mtime, NULL);
                }

                native_file_seek(ent->u.filename.hfile, ent->u.filename.offset + curpos);

                if (frame_rest(frm) < curlen) 
                    frame_grow(frm, curlen); 

                ret = native_file_read(ent->u.filename.hfile, frame_end(frm), curlen);
                if (ret >= 0) frame_len_add(frm, ret);

            } else if (ent->cktype == CKT_FILE_PTR) {
                ret = frame_file_read(frm, ent->u.fileptr.fp, ent->u.fileptr.offset + curpos, curlen);
                if (ret >= 0) curlen = ret;

            } else if (ent->cktype == CKT_FILE_DESC) {
                ret = frame_filefd_read(frm, ent->u.filefd.fd, ent->u.filefd.offset + curpos, curlen);
                if (ret >= 0) curlen = ret;

            } else if (ent->cktype == CKT_CALLBACK) {
                void   * pbyte = NULL;
                int64    bytelen = 0;
                int64    onelen = 0;

                while (ent->u.callback.fetchfunc && onelen < curlen) {
                    ret = (*ent->u.callback.fetchfunc)(ent->u.callback.fetchpara, 
                                                       ent->u.callback.offset + curpos + onelen,
                                                       curlen - onelen,
                                                       &pbyte, &bytelen);
                    if (ret >= 0) {
                        frame_put_nlast(frm, pbyte, bytelen);
                        onelen += bytelen;
                        continue;
                    }
                    break;
                }
                curlen = onelen;
            }

            readlen += curlen;
            readpos += curlen;
            if (readlen >= length) {
                ck->seekpos = readpos;
                return readlen;
            }
        }
        accentlen += ent->length;

        if (httpchunk) {
            if (ent->trailerlen > 0 && readpos < accentlen + ent->trailerlen) {
                curpos = readpos - accentlen;
                curlen = ent->trailerlen - curpos;
                if (curlen > length - readlen)
                    curlen = length - readlen;
 
                frame_put_nlast(frm, ent->trailer + curpos, curlen);
 
                readlen += curlen;
                readpos += curlen;
                if (readlen >= length) {
                    ck->seekpos = readpos;
                    return readlen;
                }
            }
            accentlen += ent->trailerlen;
        }
    }

    if (httpchunk && ck->chunkendsize > 0 && readpos + 5 >= ck->chunkendsize) {
        if (readpos < accentlen + 5) { //0\r\n\r\n
            curpos = readpos - accentlen;
            curlen = 5 - curpos;
            if (curlen > length - readlen)
                curlen = length - readlen;
 
            frame_put_nlast(frm, chunk_end_flag + curpos, curlen);
 
            readlen += curlen;
            readpos += curlen;
            if (readlen >= length) {
                ck->seekpos = readpos;
                return readlen;
            }
        }
        accentlen += 5;
    }

    ck->seekpos = readpos;
    return readlen;
}

int64 chunk_readto_file (void * vck, int fd, int64 offset, int64 length, int httpchunk)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int        i, num, ret;

    int64      accentlen = 0;

    int64      readpos = 0;
    int64      readlen = 0;
    int64      curpos = 0;
    int64      curlen = 0;

    if (!ck) return -1;
    if (fd < 0) return -2;

    if (httpchunk) {
        if (offset < ck->rmchunklen)
            offset = ck->rmchunklen;

        if (offset >= ck->chunksize) return -3;

        if (length < 0) length = ck->chunksize - offset;

        if (length > ck->chunksize - offset)
            length = ck->chunksize - offset; 

        accentlen = ck->rmchunklen;

    } else {
        if (offset < ck->rmentlen)
            offset = ck->rmentlen;

        if (offset >= ck->size) return -3;
 
        if (length < 0) length = ck->size - offset;

        if (length > ck->size - offset)
            length = ck->size - offset;
 
        accentlen = ck->rmentlen;
    }

    readpos = offset;
    readlen = 0;
    ck->seekpos = offset;

    num = arr_num(ck->entity_list);
    for (i = 0; i < num; i++) {

        ent = arr_value(ck->entity_list, i);
        if (!ent) continue;

        if (httpchunk) {
            if (ent->lenstrlen > 0 && readpos < accentlen + ent->lenstrlen) {
                curpos = readpos - accentlen;
                curlen = ent->lenstrlen - curpos;
                if (curlen > length - readlen)
                    curlen = length - readlen;

                filefd_write(fd, ent->lenstr + curpos, curlen);

                readlen += curlen;
                readpos += curlen;
                if (readlen >= length) {
                    ck->seekpos = readpos;
                    return readlen;
                }
            }
            accentlen += ent->lenstrlen;
        }

        if (ent->length > 0 && readpos < accentlen + ent->length) {
            curpos = readpos - accentlen;
            curlen = ent->length - curpos;
            if (curlen > length - readlen)
                curlen = length - readlen;

            if (ent->cktype == CKT_CHAR_ARRAY) {
                filefd_write(fd, ent->u.charr.pbyte + curpos, curlen);

            } else if (ent->cktype == CKT_BUFFER) {
                filefd_write(fd, (uint8 *)ent->u.buf.pbyte + curpos, curlen);

            } else if (ent->cktype == CKT_BUFFER_PTR) {
                filefd_write(fd, (uint8 *)ent->u.bufptr.pbyte + curpos, curlen);

            } else if (ent->cktype == CKT_FILE_NAME) {
                if (ent->u.filename.hfile == NULL) {
                    ent->u.filename.hfile = native_file_open(ent->u.filename.fname, NF_READ);

                    if (ent->u.filename.fsize != native_file_size(ent->u.filename.hfile))
                        file_attr(ent->u.filename.fname,
                                  &ent->u.filename.inode,
                                  &ent->u.filename.fsize, NULL,
                                  &ent->u.filename.mtime, NULL);
                }

                filefd_copy(native_file_fd(ent->u.filename.hfile),
                            ent->u.filename.offset + curpos,
                            curlen, fd, NULL);

            } else if (ent->cktype == CKT_FILE_PTR) {
                filefd_copy(fileno(ent->u.fileptr.fp),
                            ent->u.fileptr.offset + curpos,
                            curlen, fd, NULL);

            } else if (ent->cktype == CKT_FILE_DESC) {
                filefd_copy(ent->u.filefd.fd,
                            ent->u.filefd.offset + curpos,
                            curlen, fd, NULL);

            } else if (ent->cktype == CKT_CALLBACK) {
                void   * pbyte = NULL;
                int64    bytelen = 0;
                int64    onelen = 0;

                while (ent->u.callback.fetchfunc && onelen < curlen) {
                    ret = (*ent->u.callback.fetchfunc)(ent->u.callback.fetchpara, 
                                                       ent->u.callback.offset + curpos + onelen,
                                                       curlen - onelen,
                                                       &pbyte, &bytelen);
                    if (ret >= 0) {
                        filefd_write(fd, pbyte, bytelen);
                        onelen += bytelen;
                        continue;
                    }
                    break;
                }
                curlen = onelen;
            }

            readlen += curlen;
            readpos += curlen;
            if (readlen >= length) {
                ck->seekpos = readpos;
                return readlen;
            }
        }
        accentlen += ent->length;

        if (httpchunk) {
            if (ent->trailerlen > 0 && readpos < accentlen + ent->trailerlen) {
                curpos = readpos - accentlen;
                curlen = ent->trailerlen - curpos;
                if (curlen > length - readlen)
                    curlen = length - readlen;
 
                filefd_write(fd, ent->trailer + curpos, curlen);
 
                readlen += curlen;
                readpos += curlen;
                if (readlen >= length) {
                    ck->seekpos = readpos;
                    return readlen;
                }
            }
            accentlen += ent->trailerlen;
        }
    }

    if (httpchunk && ck->chunkendsize > 0 && readpos + 5 >= ck->chunkendsize) {
        if (readpos < accentlen + 5) { //0\r\n\r\n
            curpos = readpos - accentlen;
            curlen = 5 - curpos;
            if (curlen > length - readlen)
                curlen = length - readlen;
 
            filefd_write(fd, chunk_end_flag + curpos, curlen);
 
            readlen += curlen;
            readpos += curlen;
            if (readlen >= length) {
                ck->seekpos = readpos;
                return readlen;
            }
        }
        accentlen += 5;
    }

    ck->seekpos = readpos;
    return readlen;
}

int chunk_go_ahead (void * vck, void * msg, int64 offset, int64 step)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int        i, num;
 
    int64      accentlen = 0;
    int64      readpos = 0;

    int64      curpos = 0;
    int64      curlen = 0;
 
    if (!ck) return -1;
 
    if (offset < ck->rmentlen) return -2;
    if (offset >= ck->size) return -3;
 
    if (ck->procnotify)
        (*ck->procnotify)(msg, ck->procnotifypara, ck->procnotifycbval, offset, step);

    readpos = offset;
    accentlen = ck->rmentlen;
 
    num = arr_num(ck->entity_list);

    for (i = 0; i < num; i++) {

        ent = arr_value(ck->entity_list, i);
        if (!ent || ent->length <= 0) continue;
 
        if (readpos < accentlen + ent->length) {
            curpos = readpos - accentlen;
            curlen = ent->length - curpos;
 
            if (step > curlen) {
                //should be en error
            }

            if (ent->cktype == CKT_CALLBACK && ent->u.callback.movefunc) {
                (*ent->u.callback.movefunc)(ent->u.callback.movepara,
                                            ent->u.callback.offset + curpos, step);
            }

            break;
        }

        accentlen += ent->length;
    }
 
    return 0;
}

int chunk_add_buffer (void * vck, void * pbuf, int64 len)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;

    if (!ck) return -1;

    if (len < 0) return 0;

    ent = kzalloc(sizeof(*ent));
    if (!ent) return -100;

    if (len < sizeof(ent->u.charr.pbyte)) {
        ent->cktype = CKT_CHAR_ARRAY;
        ent->length = len;

        if (len > 0) memcpy(ent->u.charr.pbyte, pbuf, len);
        ((char *)ent->u.charr.pbyte)[len] = '\0';

    } else {
        ent->cktype = CKT_BUFFER;
        ent->length = len;

        ent->u.buf.pbyte = kalloc(len + 1);
        if (ent->u.buf.pbyte == NULL) {
            kfree(ent);
            return -200;
        }

        if (len > 0) memcpy(ent->u.buf.pbyte, pbuf, len);
        ((char *)ent->u.buf.pbyte)[len] = '\0';
    }

    arr_push(ck->entity_list, ent);

    ck->size += len;
    ck->bufnum++;

#if defined(_WIN32) || defined(_WIN64)
    sprintf(ent->lenstr, "%I64x\r\n", ent->length);
#else
    sprintf(ent->lenstr, "%llx\r\n", ent->length);
#endif
    ent->lenstrlen = strlen(ent->lenstr);
    strcpy(ent->trailer, "\r\n");
    ent->trailerlen = 2;
    ck->chunksize += ent->length + ent->lenstrlen + ent->trailerlen;

    return 0;
}

int64 chunk_prepend_strip_buffer (void * vck, void * pbuf, int64 len, char * escch, int chlen, uint8 isheader)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int        i, num = 0;
    int        insloc = 0;
 
    if (!ck) return -1;
 
    if (len < 0) return 0;
 
    /* find the location of non-header entity */
    if (!isheader) {
        num = arr_num(ck->entity_list);
        for (i = 0; i < num; i++) {
            ent = arr_value(ck->entity_list, i);
            if (!ent) continue;
            if (ent->header) continue;
            break;
        }
        insloc = i;
    }
 
    ent = kzalloc(sizeof(*ent));
    if (!ent) return -100;
 
    ent->u.buf.pbyte = kalloc(len+1);
    if (ent->u.buf.pbyte == NULL) {
        kfree(ent);
        return -200;
    }
 
    ent->cktype = CKT_BUFFER;
    ent->length = string_strip(pbuf, len, escch, chlen, ent->u.buf.pbyte, len);
    if (ent->length >= 0)
        ((char *)ent->u.buf.pbyte)[ent->length] = '\0';

    ent->header = isheader ? 1 : 0;

    arr_insert(ck->entity_list, ent, insloc);
    ck->bufnum++;

    ck->size += len;

    if (ent->header) {
        ent->lenstrlen = 0;
        ent->trailerlen = 0;
        ck->chunksize += ent->length;
    } else {
#if defined(_WIN32) || defined(_WIN64)
        sprintf(ent->lenstr, "%I64x\r\n", ent->length);
#else
        sprintf(ent->lenstr, "%llx\r\n", ent->length);
#endif
        ent->lenstrlen = strlen(ent->lenstr);
        strcpy(ent->trailer, "\r\n");
        ent->trailerlen = 2;
        ck->chunksize += ent->length + ent->lenstrlen + ent->trailerlen;
    }

    if (ck->endsize > 0) {
        ck->endsize = ck->size;
        ck->chunkendsize = ck->chunksize;
    }

    return ent->length;
}

int64 chunk_add_strip_buffer (void * vck, void * pbuf, int64 len, char * escch, int chlen)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
 
    if (!ck) return -1;
 
    if (len <= 0) return 0;
 
    ent = kzalloc(sizeof(*ent));
    if (!ent) return -100;
 
    ent->u.buf.pbyte = kalloc(len+1);
    if (ent->u.buf.pbyte == NULL) {
        kfree(ent);
        return -200;
    }

    ent->cktype = CKT_BUFFER;
    ent->length = string_strip(pbuf, len, escch, chlen, ent->u.buf.pbyte, len);
    if (ent->length >= 0)
        ((char *)ent->u.buf.pbyte)[ent->length] = '\0';

    arr_push(ck->entity_list, ent);
    ck->bufnum++;

    ck->size += ent->length;

#if defined(_WIN32) || defined(_WIN64)
    sprintf(ent->lenstr, "%I64x\r\n", ent->length);
#else
    sprintf(ent->lenstr, "%llx\r\n", ent->length);
#endif
    ent->lenstrlen = strlen(ent->lenstr);
    strcpy(ent->trailer, "\r\n");
    ent->trailerlen = 2;
    ck->chunksize += ent->length + ent->lenstrlen + ent->trailerlen;

    return ent->length;
}

int64 chunk_append_strip_buffer (void * vck, void * pbuf, int64 len, char * escch, int chlen)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
 
    if (!ck) return -1;
 
    if (len <= 0) return 0;
 
    ent = kzalloc(sizeof(*ent));
    if (!ent) return -100;
 
    ent->u.buf.pbyte = kalloc(len+1);
    if (ent->u.buf.pbyte == NULL) {
        kfree(ent);
        return -200;
    }
 
    ent->cktype = CKT_BUFFER;
    ent->length = string_strip(pbuf, len, escch, chlen, ent->u.buf.pbyte, len);
    if (ent->length >= 0)
        ((char *)ent->u.buf.pbyte)[ent->length] = '\0';

#if defined(_WIN32) || defined(_WIN64)
    sprintf(ent->lenstr, "%I64x\r\n", ent->length);
#else
    sprintf(ent->lenstr, "%llx\r\n", ent->length);
#endif
    ent->lenstrlen = strlen(ent->lenstr);
    strcpy(ent->trailer, "\r\n");
    ent->trailerlen = 2;

    if (ck->endsize > 0) {
        arr_push(ck->entity_list, ent);

        ck->bufnum++;
        ck->size += len;
        ck->chunksize += ent->length + ent->lenstrlen + ent->trailerlen;

        ck->endsize = ck->size;
        ck->chunkendsize = ck->chunksize;

    } else {
        arr_push(ck->tail_entity_list, ent);
    }

    return ent->length;
}

int chunk_remove_bufptr (void * vck, void * pbuf)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int        i, num;
    int        rmnum = 0;

    if (!ck) return -1;

    num = arr_num(ck->entity_list);

    for (i = 0; i < num; i++) {

        ent = arr_value(ck->entity_list, i);
        if (!ent) continue;

        if (ent->cktype != CKT_BUFFER_PTR)
            continue;

        if (pbuf == ent->u.bufptr.pbyte) {
            arr_delete(ck->entity_list, i);
            i--;

            ck->size -= ent->length;
            ck->bufnum--;
            ck->chunksize -= ent->length + ent->lenstrlen + ent->trailerlen;

            kfree(ent);

            rmnum++;
        }
    }

    return rmnum;
}

int chunk_prepend_bufptr (void * vck, void * pbuf, int64 len, void * porig, void * freefunc, uint8 isheader)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int        i, num = 0;
    int        insloc = 0;

    if (!ck) return -1;

    if (len <= 0) return 0;

    /* find the location of non-header entity */
    if (!isheader) {
        num = arr_num(ck->entity_list);
        for (i = 0; i < num; i++) {
            ent = arr_value(ck->entity_list, i);
            if (!ent) continue;
            if (ent->header) continue;
            break;
        }
        insloc = i;
    }

    ent = kzalloc(sizeof(*ent));
    if (!ent) return -100;

    ent->cktype = CKT_BUFFER_PTR;
    ent->header = isheader ? 1 : 0;
    ent->length = len;
    ent->u.bufptr.pbyte = pbuf;
    ent->u.bufptr.porig = porig;
    ent->u.bufptr.freefunc = freefunc;

    arr_insert(ck->entity_list, ent, insloc);
    ck->bufnum++;

    ck->size += len;

    if (ent->header) {
        ent->lenstrlen = 0;
        ent->trailerlen = 0;
        ck->chunksize += ent->length;
    } else {
#if defined(_WIN32) || defined(_WIN64)
        sprintf(ent->lenstr, "%I64x\r\n", ent->length);
#else
        sprintf(ent->lenstr, "%llx\r\n", ent->length);
#endif
        ent->lenstrlen = strlen(ent->lenstr);
        strcpy(ent->trailer, "\r\n");
        ent->trailerlen = 2;
        ck->chunksize += ent->length + ent->lenstrlen + ent->trailerlen;
    }

    if (ck->endsize > 0) {
        ck->endsize = ck->size;
        ck->chunkendsize = ck->chunksize;
    }

    return 0;
}

int chunk_add_bufptr (void * vck, void * pbuf, int64 len, void * porig, void * freefunc)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;

    if (!ck) return -1;

    if (len <= 0) return 0;

    ent = kzalloc(sizeof(*ent));
    if (!ent) return -100;

    ent->cktype = CKT_BUFFER_PTR;
    ent->length = len;
    ent->u.bufptr.pbyte = pbuf;
    ent->u.bufptr.porig = porig;
    ent->u.bufptr.freefunc = freefunc;

#if defined(_WIN32) || defined(_WIN64)
    sprintf(ent->lenstr, "%I64x\r\n", ent->length);
#else
    sprintf(ent->lenstr, "%llx\r\n", ent->length);
#endif
    ent->lenstrlen = strlen(ent->lenstr);
    strcpy(ent->trailer, "\r\n");
    ent->trailerlen = 2;

    arr_push(ck->entity_list, ent);

    ck->bufnum++;
    ck->size += len;
    ck->chunksize += ent->length + ent->lenstrlen + ent->trailerlen;

    return 0;
}

int chunk_append_bufptr (void * vck, void * pbuf, int64 len, void * porig, void * freefunc)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;

    if (!ck) return -1;

    if (len <= 0) return 0;

    ent = kzalloc(sizeof(*ent));
    if (!ent) return -100;

    ent->cktype = CKT_BUFFER_PTR;
    ent->length = len;
    ent->u.bufptr.pbyte = pbuf;
    ent->u.bufptr.porig = porig;
    ent->u.bufptr.freefunc = freefunc;

#if defined(_WIN32) || defined(_WIN64)
    sprintf(ent->lenstr, "%I64x\r\n", ent->length);
#else
    sprintf(ent->lenstr, "%llx\r\n", ent->length);
#endif
    ent->lenstrlen = strlen(ent->lenstr);
    strcpy(ent->trailer, "\r\n");
    ent->trailerlen = 2;

    if (ck->endsize > 0) {
        arr_push(ck->entity_list, ent);

        ck->bufnum++;
        ck->size += len;
        ck->chunksize += ent->length + ent->lenstrlen + ent->trailerlen;

        ck->endsize = ck->size;
        ck->chunkendsize = ck->chunksize;

    } else {
        arr_push(ck->tail_entity_list, ent);
    }

    return 0;
}

int chunk_bufptr_porig_find (void * vck, void * porig)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int        i, num;
    int        entnum = 0;

    if (!ck) return 0;

    num = arr_num(ck->entity_list);

    for (i = 0, entnum = 0; i < num; i++) {
        ent = arr_value(ck->entity_list, i);
        if (!ent) continue;

        if (ent->cktype == CKT_BUFFER_PTR) {
            if (ent->u.bufptr.porig == porig)
                entnum++;
        }
    }

    return entnum;
}

int chunk_add_file (void * vck, char * fname, int64 offset, int64 length, int merge)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int64      fsize = 0;
    long       inode = 0;
    time_t     mtime = 0;

    if (!ck) return -1;

    if (offset < 0) offset = 0;

    if (file_attr(fname, &inode, &fsize, NULL, &mtime, NULL) < 0)
        return -100;

    if (length < 0) length = fsize;

    if (offset + length > fsize)
        length = fsize - offset;
    if (length <= 0) return 0;

    ent = arr_last(ck->entity_list);
    if (merge && ent && ent->cktype == CKT_FILE_NAME && 
        strcmp(ent->u.filename.fname, fname) == 0 &&
        ent->u.filename.offset + ent->length >= offset &&
        ent->u.filename.offset + ent->length < offset + length)
    {
        ck->chunksize -= ent->length + ent->lenstrlen + ent->trailerlen;

        length -= (ent->u.filename.offset + ent->length - offset);
        if (length <= 0) return 0;

        ent->length += length;

    } else {
        ent = kzalloc(sizeof(*ent));
        if (!ent) return -100;

        ent->cktype = CKT_FILE_NAME;
        ent->length = length;

        ent->u.filename.pbyte = NULL;
#if defined(_WIN32) || defined(_WIN64)
        ent->u.filename.hmap = NULL;
#endif
        ent->u.filename.pmap = NULL;
        ent->u.filename.maplen = 0;

        ent->u.filename.offset = offset;
        ent->u.filename.hfile = NULL;
        ent->u.filename.fname = str_dup(fname, -1);

        arr_push(ck->entity_list, ent);
        ck->filenum++;
    }

    ent->u.filename.fsize = fsize;
    ent->u.filename.inode = inode;
    ent->u.filename.mtime = mtime;

    ck->size += length;

#if defined(_WIN32) || defined(_WIN64)
    sprintf(ent->lenstr, "%I64x\r\n", ent->length);
#else
    sprintf(ent->lenstr, "%llx\r\n", ent->length);
#endif
    ent->lenstrlen = strlen(ent->lenstr);

    strcpy(ent->trailer, "\r\n"); 
    ent->trailerlen = 2;

    ck->chunksize += ent->length + ent->lenstrlen + ent->trailerlen;

    return 0;
}

int64 chunk_prepend_file (void * vck, char * fname, int64 packsize)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int64      fsize = 0;
    long       inode = 0;
    time_t     mtime = 0;
    int        i, num = 0;
    int        insloc = 0;

    int64      offset = 0;
    int64      length = 0;

    if (!ck) return -1;

    if (file_attr(fname, &inode, &fsize, NULL, &mtime, NULL) < 0)
        return -100;

    if (fsize <= 0) return -101;

    /* find the location of non-header entity */
    num = arr_num(ck->entity_list);
    for (i = 0; i < num; i++) {
        ent = arr_value(ck->entity_list, i);
        if (!ent) continue;
        if (ent->header) continue;
        break;
    }
    insloc = i;

    if (packsize <= 0) num = 1;
    else {
        num = (fsize + packsize - 1) / packsize;
    }

    for (i = 0; i < num; i++) {
        offset = packsize * i;
        length = fsize - offset;
        if (packsize > 0 && length > packsize)
            length = packsize;

        ent = kzalloc(sizeof(*ent));
        if (!ent) return -200;

        ent->cktype = CKT_FILE_NAME;
        ent->length = length;

        ent->u.filename.pbyte = NULL;
#if defined(_WIN32) || defined(_WIN64)
        ent->u.filename.hmap = NULL;
#endif
        ent->u.filename.pmap = NULL;
        ent->u.filename.maplen = 0;

        ent->u.filename.offset = offset;
        ent->u.filename.hfile = NULL;
        ent->u.filename.fname = str_dup(fname, -1);

        ent->u.filename.fsize = fsize;
        ent->u.filename.inode = inode;
        ent->u.filename.mtime = mtime;

#if defined(_WIN32) || defined(_WIN64)
        sprintf(ent->lenstr, "%I64x\r\n", ent->length);
#else
        sprintf(ent->lenstr, "%llx\r\n", ent->length);
#endif
        ent->lenstrlen = strlen(ent->lenstr);

        strcpy(ent->trailer, "\r\n");
        ent->trailerlen = 2;

        arr_insert(ck->entity_list, ent, insloc++);

        ck->filenum++;
        ck->size += length;
        ck->chunksize += ent->length + ent->lenstrlen + ent->trailerlen;

        if (ck->endsize > 0) {
            ck->endsize = ck->size;
            ck->chunkendsize = ck->chunksize;
        }
    }

    return fsize;
}

int64 chunk_append_file (void * vck, char * fname, int64 packsize)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int64      fsize = 0;
    long       inode = 0;
    time_t     mtime = 0;
    int        i, num = 0;

    int64      offset = 0;
    int64      length = 0;

    if (!ck) return -1;

    if (file_attr(fname, &inode, &fsize, NULL, &mtime, NULL) < 0)
        return -100;

    if (fsize <= 0) return -101;

    if (packsize <= 0) num = 1;
    else {
        num = (fsize + packsize - 1) / packsize;
    }

    for (i = 0; i < num; i++) {
        offset = packsize * i;
        length = fsize - offset;
        if (packsize > 0 && length > packsize)
            length = packsize;

        ent = kzalloc(sizeof(*ent));
        if (!ent) return -200;

        ent->cktype = CKT_FILE_NAME;
        ent->length = length;

        ent->u.filename.pbyte = NULL;
#if defined(_WIN32) || defined(_WIN64)
        ent->u.filename.hmap = NULL;
#endif
        ent->u.filename.pmap = NULL;
        ent->u.filename.maplen = 0;

        ent->u.filename.offset = offset;
        ent->u.filename.hfile = NULL;
        ent->u.filename.fname = str_dup(fname, -1);

        ent->u.filename.fsize = fsize;
        ent->u.filename.inode = inode;
        ent->u.filename.mtime = mtime;

#if defined(_WIN32) || defined(_WIN64)
        sprintf(ent->lenstr, "%I64x\r\n", ent->length);
#else
        sprintf(ent->lenstr, "%llx\r\n", ent->length);
#endif
        ent->lenstrlen = strlen(ent->lenstr);

        strcpy(ent->trailer, "\r\n");
        ent->trailerlen = 2;

        if (ck->endsize > 0) {
            arr_push(ck->entity_list, ent);

            ck->filenum++;
            ck->size += length;
            ck->chunksize += ent->length + ent->lenstrlen + ent->trailerlen;

            ck->endsize = ck->size;
            ck->chunkendsize = ck->chunksize;

        } else {
            arr_push(ck->tail_entity_list, ent);
        }
    }

    return fsize;
}

int chunk_add_filefp (void * vck, FILE * fp, int64 offset, int64 length)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int64      fsize = 0;
#ifdef UNIX
    struct stat   st;
#endif

    if (!ck) return -1;
    if (!fp) return -2;

    if (offset < 0) offset = 0;
 
#ifdef UNIX
    if (fstat(fileno(fp), &st) < 0) return -100;

    fsize = st.st_size;
    if (length < 0) length = fsize;
    if (offset + length > fsize)
        length = fsize - offset;
#endif
    if (length <= 0) return 0;
 
    ent = kzalloc(sizeof(*ent));
    if (!ent) return -100;
 
    ent->cktype = CKT_FILE_PTR;
    ent->length = length;
 
    ent->u.fileptr.pbyte = NULL;
#if defined(_WIN32) || defined(_WIN64)
    ent->u.fileptr.hmap = NULL;
#endif
    ent->u.fileptr.pmap = NULL;
    ent->u.fileptr.maplen = 0;

    ent->u.fileptr.fp = fp;
    ent->u.fileptr.offset = offset;
    ent->u.fileptr.fsize = fsize;
#ifdef UNIX
    ent->u.fileptr.inode = st.st_ino;
    ent->u.fileptr.mtime = st.st_mtime;
#endif
 
    arr_push(ck->entity_list, ent);

    ck->size += ent->length;
    ck->filenum++;

#if defined(_WIN32) || defined(_WIN64)
    sprintf(ent->lenstr, "%I64x\r\n", ent->length);
#else
    sprintf(ent->lenstr, "%llx\r\n", ent->length);
#endif
    ent->lenstrlen = strlen(ent->lenstr);
    strcpy(ent->trailer, "\r\n");
    ent->trailerlen = 2;
    ck->chunksize += ent->length + ent->lenstrlen + ent->trailerlen;

    return 0;
}

int chunk_add_filefd (void * vck, int fd, int64 offset, int64 length)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int64      fsize = 0;
#ifdef UNIX
    struct stat   st;
#endif
 
    if (!ck) return -1;
    if (fd < 0) return -2;
 
    if (offset < 0) offset = 0;
 
#ifdef UNIX
    if (fstat(fd, &st) < 0) return -100;

    fsize = st.st_size;

    if (length < 0) length = fsize;
    if (offset + length > fsize)
        length = fsize - offset;
#endif
    if (length <= 0) return 0;
 
    ent = kzalloc(sizeof(*ent));
    if (!ent) return -100;
 
    ent->cktype = CKT_FILE_DESC;
    ent->length = length;
 
    ent->u.filefd.pbyte = NULL;
#if defined(_WIN32) || defined(_WIN64)
    ent->u.filefd.hmap = NULL;
#endif
    ent->u.filefd.pmap = NULL;
    ent->u.filefd.maplen = 0;

    ent->u.filefd.fd = fd;
    ent->u.filefd.offset = offset;
    ent->u.filefd.fsize = fsize;
#ifdef UNIX
    ent->u.filefd.inode = st.st_ino;
    ent->u.filefd.mtime = st.st_mtime;
#endif
 
    arr_push(ck->entity_list, ent);

    ck->size += ent->length;
    ck->filenum++;

#if defined(_WIN32) || defined(_WIN64)
    sprintf(ent->lenstr, "%I64x\r\n", ent->length);
#else
    sprintf(ent->lenstr, "%llx\r\n", ent->length);
#endif
    ent->lenstrlen = strlen(ent->lenstr);
    strcpy(ent->trailer, "\r\n");
    ent->trailerlen = 2;
    ck->chunksize += ent->length + ent->lenstrlen + ent->trailerlen;

    return 0;
}
 
int chunk_add_cbdata (void * vck, void * fetchfunc, void * fetchobj, int64 offset, int64 length,
                      void * movefunc, void * moveobj, void * cleanfunc, void * cleanobj)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;

    if (!ck) return -1;
    if (!fetchfunc) return -2;

    if (offset < 0) offset = 0;
    if (length <= 0) return 0;
 
    ent = kzalloc(sizeof(*ent));
    if (!ent) return -100;
 
    ent->cktype = CKT_CALLBACK;
    ent->length = length;
 
    ent->u.callback.fetchfunc = fetchfunc;
    ent->u.callback.fetchpara = fetchobj;
    ent->u.callback.movefunc = movefunc;
    ent->u.callback.movepara = moveobj;
    ent->u.callback.endfunc = cleanfunc;
    ent->u.callback.endpara = cleanobj;
    ent->u.callback.offset = offset;
 
    arr_push(ck->entity_list, ent);
    ck->bufnum++;

    ck->size += length;

#if defined(_WIN32) || defined(_WIN64)
    sprintf(ent->lenstr, "%I64x\r\n", ent->length);
#else
    sprintf(ent->lenstr, "%llx\r\n", ent->length);
#endif
    ent->lenstrlen = strlen(ent->lenstr);
    strcpy(ent->trailer, "\r\n");
    ent->trailerlen = 2;
    ck->chunksize += ent->length + ent->lenstrlen + ent->trailerlen;

    return 0;
}


int chunk_add_process_notify (void * vck, void * notifyfunc, void * notifypara, uint64 notifycbval)
{
    chunk_t  * ck = (chunk_t *)vck;

    if (!ck) return -1;

    ck->procnotify = notifyfunc;
    ck->procnotifypara = notifypara;
    ck->procnotifycbval = notifycbval;

    return 0;
}

int chunk_remove (void * vck, int64 pos, int httpchunk)
{
    chunk_t * ck = (chunk_t *)vck;
    ckent_t * ent = NULL;
    int       i, num, rmnum;
    int64     acclen = 0;

    if (!ck) return -1;

    if (httpchunk) {
        if (pos < ck->rmchunklen)
            return 0;
        acclen = ck->rmchunklen;

    } else {
        if (pos < ck->rmentlen)
            return 0;
        acclen = ck->rmentlen;
    }

    rmnum = 0;
    num = arr_num(ck->entity_list);

    for (i = 0; i < num; i++) {
        ent = arr_value(ck->entity_list, 0);
        if (!ent) {
            arr_delete(ck->entity_list, 0);
            continue;
        }

        if (httpchunk) {
            if (pos >= acclen && pos < acclen + ent->lenstrlen + ent->length + ent->trailerlen)
                break;

            acclen += ent->length + ent->lenstrlen + ent->trailerlen;

        } else {
            if (pos >= acclen && pos < acclen + ent->length)
                break;

            acclen += ent->length;
        }

        ck->rmchunklen += ent->length + ent->lenstrlen + ent->trailerlen;
        ck->rmentlen += ent->length;

        switch (ent->cktype) {
        case CKT_FILE_NAME:
        case CKT_FILE_PTR:
        case CKT_FILE_DESC:
            ck->filenum--;
            break;
        case CKT_CHAR_ARRAY:
        case CKT_BUFFER:
        case CKT_BUFFER_PTR:
        case CKT_CALLBACK:
            ck->bufnum--;
            break;
        }

        arr_delete(ck->entity_list, 0);
        i--; num--;
        rmnum++;

        if (ent->u.bufptr.porig && ent->u.bufptr.freefunc &&
            chunk_bufptr_porig_find(ck, ent->u.bufptr.porig) <= 0)
        {
            (*ent->u.bufptr.freefunc)(ent->u.bufptr.porig);
        }

        chunk_entity_free(ent);
    }

    return rmnum;
}


int chunk_buf_and_file (void * vck)
{
    chunk_t  * ck = (chunk_t *)vck;

    if (!ck) return 0;

    if (ck->filenum <= 0) return 0;

    return ck->bufnum > 0 ? 1 : 0;
}

int chunk_vec_get (void * vck, int64 offset, chunk_vec_t * pvec, int httpchunk)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int        i, num;
 
    int64      accentlen = 0;

    int64      readpos = 0;
    int64      curpos = 0;
    int64      curlen = 0;
 
    void     * pbyte = NULL;
    int64      bytelen = 0;

    if (!ck) return -1;
 
    if (!pvec) return -2;
    
    if (httpchunk) {
        if (offset < ck->rmchunklen)
            return -3;
 
        if (offset >= ck->chunksize) {
            if (ck->chunkendsize > 0) return -4;
            else return 0;
        }
 
        accentlen = ck->rmchunklen;

    } else {
        if (offset < ck->rmentlen)
            return -3;

        if (offset >= ck->size) {
            if (ck->endsize > 0) return -4;
            else return 0;
        }
 
        accentlen = ck->rmentlen;
    }

    pvec->offset = offset;
    pvec->size = 0;
    pvec->iovcnt = 0;
    pvec->filefd = -1;
    pvec->fpos = 0;
    pvec->filesize = 0;
     
    readpos = offset;
    num = arr_num(ck->entity_list);
 
    for (i = 0; i < num; i++) {
        if (pvec->iovcnt >= sizeof(pvec->iovs)/sizeof(pvec->iovs[0]) - 3)
            break;

        ent = arr_value(ck->entity_list, i);
        if (!ent) continue;

        if (httpchunk) {
            if (ent->lenstrlen > 0 && readpos < accentlen + ent->lenstrlen) {
                if (pvec->vectype != 0 && pvec->vectype != 1)
                    return (int)pvec->size;
    
                curpos = readpos - accentlen;
                curlen = ent->lenstrlen - curpos;
    
                pvec->iovs[pvec->iovcnt].iov_base = ent->lenstr + curpos;
                pvec->iovs[pvec->iovcnt].iov_len = curlen;
                pvec->iovcnt++;
    
                pvec->size += curlen;
                pvec->vectype = 1; //mem buffer
    
                readpos += curlen;
            }
            accentlen += ent->lenstrlen;
        }

        if (ent->length > 0 && readpos < accentlen + ent->length) {
            curpos = readpos - accentlen;
            curlen = ent->length - curpos;
 
            if (ent->cktype == CKT_CHAR_ARRAY) {
                if (pvec->vectype != 0 && pvec->vectype != 1)
                    return (int)pvec->size;
 
                pvec->iovs[pvec->iovcnt].iov_base = ent->u.charr.pbyte + curpos;
                pvec->iovs[pvec->iovcnt].iov_len = curlen;
                pvec->iovcnt++;
 
                pvec->size += curlen;
 
                pvec->vectype = 1; //mem buffer

            } else if (ent->cktype == CKT_BUFFER) {
                if (pvec->vectype != 0 && pvec->vectype != 1)
                    return (int)pvec->size;

                pvec->iovs[pvec->iovcnt].iov_base = (uint8 *)ent->u.buf.pbyte + curpos;
                pvec->iovs[pvec->iovcnt].iov_len = curlen;
                pvec->iovcnt++;

                pvec->size += curlen;

                pvec->vectype = 1; //mem buffer
 
            } else if (ent->cktype == CKT_BUFFER_PTR) {
                if (pvec->vectype != 0 && pvec->vectype != 1)
                    return (int)pvec->size;

                pvec->iovs[pvec->iovcnt].iov_base = (uint8 *)ent->u.bufptr.pbyte + curpos;
                pvec->iovs[pvec->iovcnt].iov_len = curlen;
                pvec->iovcnt++;

                pvec->size += curlen;

                pvec->vectype = 1; //mem buffer
 
            } else if (ent->cktype == CKT_FILE_NAME) {
                if (pvec->vectype != 0 && pvec->vectype != 2)
                    return (int)pvec->size;

                if (ent->u.filename.hfile == NULL) {
                    ent->u.filename.hfile = native_file_open(ent->u.filename.fname, NF_READ);

                    if (ent->u.filename.fsize != native_file_size(ent->u.filename.hfile)) {
#ifdef UNIX
                        file_attr(ent->u.filename.fname,
                                  &ent->u.filename.inode,
                                  &ent->u.filename.fsize, NULL,
                                  &ent->u.filename.mtime, NULL);
#endif
#if defined(_WIN32) || defined(_WIN64)
                        native_file_attr(ent->u.filename.hfile,
                                         &ent->u.filename.fsize,
                                         &ent->u.filename.mtime,
                                         &ent->u.filename.inode, NULL);
#endif
                    }
                }

                pvec->filefd = native_file_fd(ent->u.filename.hfile);
#if defined(_WIN32) || defined(_WIN64)
                pvec->filehandle = native_file_handle(ent->u.filename.hfile);
#endif
                pvec->hfile = ent->u.filename.hfile;

                pvec->fpos = ent->u.filename.offset + curpos;
                pvec->filesize = ent->u.filename.fsize;

                pvec->size += curlen;

                pvec->vectype = 2; //file

                return (int)pvec->size;
 
            } else if (ent->cktype == CKT_FILE_PTR) {
                if (pvec->vectype != 0 && pvec->vectype != 2)
                    return (int)pvec->size;

                pvec->filefd = fileno(ent->u.fileptr.fp);
#if defined(_WIN32) || defined(_WIN64)
                pvec->filehandle = fd_to_file_handle(pvec->filefd);
#endif
                pvec->hfile = ent->u.fileptr.fp;

                pvec->fpos = ent->u.fileptr.offset + curpos;
                pvec->filesize = ent->u.fileptr.fsize;

                pvec->size += curlen;

                pvec->vectype = 2; //file

                return (int)pvec->size;

            } else if (ent->cktype == CKT_FILE_DESC) {
                if (pvec->vectype != 0 && pvec->vectype != 2)
                    return (int)pvec->size;

                pvec->filefd = ent->u.filefd.fd;
#if defined(_WIN32) || defined(_WIN64)
                pvec->filehandle = fd_to_file_handle(pvec->filefd);
#endif

                pvec->fpos = ent->u.filefd.offset + curpos;
                pvec->filesize = ent->u.filefd.fsize;
 
                pvec->size += curlen; 
                 
                pvec->vectype = 2; //file
 
                return (int)pvec->size;

            } else if (ent->cktype == CKT_CALLBACK) {
                if (ent->u.callback.fetchfunc) {
                    if (pvec->vectype != 0 && pvec->vectype != 1)
                        return (int)pvec->size;

                    pbyte = NULL;
                    bytelen = 0;
                    (*ent->u.callback.fetchfunc)(ent->u.callback.fetchpara,
                                                   ent->u.callback.offset + curpos,
                                                   curlen, &pbyte, &bytelen);

                    pvec->iovs[pvec->iovcnt].iov_base = pbyte;
                    pvec->iovs[pvec->iovcnt].iov_len = bytelen;
                    pvec->iovcnt++;
     
                    pvec->size += bytelen;
 
                    pvec->vectype = 1; //mem buffer
                }
            }
    
            readpos += curlen;
        }
        accentlen += ent->length;

        if (httpchunk) {
            if (ent->trailerlen > 0 && readpos < accentlen + ent->trailerlen) {
                if (pvec->vectype != 0 && pvec->vectype != 1)
                    return (int)pvec->size;
 
                curpos = readpos - accentlen;
                curlen = ent->trailerlen - curpos;
 
                pvec->iovs[pvec->iovcnt].iov_base = ent->trailer + curpos;
                pvec->iovs[pvec->iovcnt].iov_len = curlen;
                pvec->iovcnt++;
 
                pvec->size += curlen;
                pvec->vectype = 1; //mem buffer
 
                readpos += curlen;
            }
            accentlen += ent->trailerlen;
        }
    } //end while 

    if (httpchunk && ck->chunkendsize > 0 && readpos + 5 >= ck->chunkendsize) {
        if (readpos < accentlen + 5) { //0\r\n\r\n
            if (pvec->vectype != 0 && pvec->vectype != 1)
                return (int)pvec->size;

            curpos = readpos - accentlen;
            curlen = 5 - curpos;

            pvec->iovs[pvec->iovcnt].iov_base = chunk_end_flag + curpos;
            pvec->iovs[pvec->iovcnt].iov_len = curlen;
            pvec->iovcnt++;

            pvec->size += curlen;
            pvec->vectype = 1; //mem buffer

            readpos += curlen;
        }
        accentlen += 5;
    }

    return (int)pvec->size;
}

int chunk_writev (void * vck, int fd, int64 offset, int64 * actnum, int httpchunk)
{
    chunk_t     * ck = (chunk_t *)vck;
    chunk_vec_t   iovec;
    int           ret = 0;
    int64         pos = 0;
    int64         num = 0;

    if (actnum) *actnum = 0;

    if (!ck) return -1;
 
    if (httpchunk) {
        if (offset < ck->rmchunklen)
            return -3;
 
        if (offset >= ck->chunksize) {
            if (ck->chunkendsize > 0) return -4;
            else return 0;
        }
 
    } else {
        if (offset < ck->rmentlen)
            offset = ck->rmentlen;
 
        if (offset >= ck->size) {
            if (ck->endsize > 0) return -4;
            else return 0;
        }
    }

    pos = offset;

    for ( ; chunk_get_end(ck, pos, httpchunk) == 0; ) {
        memset(&iovec, 0, sizeof(iovec));
        ret = chunk_vec_get(ck, pos, &iovec, httpchunk);
 
        if (ret < 0 || (iovec.vectype != 1 && iovec.vectype != 2 && iovec.size > 0)) {
            return ret;
        }
 
        if (iovec.size == 0) {
            /* no available data to send, waiting for more data... */
            return 0;
        }
 
        if (iovec.vectype == 2) { //sendfile
            ret = filefd_copy(iovec.filefd, iovec.fpos, iovec.size, fd, &num);
            if (ret < 0) {
                return -100;
            }
 
        } else if (iovec.vectype == 1) { //mem buffer, writev
            ret = filefd_writev(fd, iovec.iovs, iovec.iovcnt, &num);
            if (ret < 0) {
                return -101;
            }
        }
 
        pos += num;
        if (actnum) *actnum += num;
    }

    return 0;
}


int chunk_at (void * vck, int64 pos, int * ind)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int        i, num, ret;
 
    int64      accentlen = 0;
 
    int64      readpos = 0;
    int64      readlen = 0;
    int64      curpos = 0;
    int64      curlen = 0;
 
    if (!ck) return -1;

    if (pos < ck->rmentlen) return -2;
    if (pos >= ck->size) return -3;

    accentlen = ck->rmentlen;

    readpos = pos;
    readlen = 0;
    ck->seekpos = pos;
 
    num = arr_num(ck->entity_list);
    for (i = 0; i < num; i++) {
 
        ent = arr_value(ck->entity_list, i);
        if (!ent) continue;
 
        if (ent->length > 0 && readpos < accentlen + ent->length) {
            curpos = readpos - accentlen;
            curlen = ent->length - curpos;
 
            if (ent->cktype == CKT_CHAR_ARRAY) {
                if (ind) *ind = i;
                return ((uint8 *)ent->u.charr.pbyte)[curpos];
 
            } else if (ent->cktype == CKT_BUFFER) {
                if (ind) *ind = i;
                return ((uint8 *)ent->u.buf.pbyte)[curpos];
 
            } else if (ent->cktype == CKT_BUFFER_PTR) {
                if (ind) *ind = i;
                return ((uint8 *)ent->u.bufptr.pbyte)[curpos];
 
            } else if (ent->cktype == CKT_FILE_NAME) {
                if (ent->u.filename.hfile == NULL) {
                    ent->u.filename.hfile = native_file_open(ent->u.filename.fname, NF_READ);

                    if (ent->u.filename.fsize != native_file_size(ent->u.filename.hfile))
                        file_attr(ent->u.filename.fname,
                                  &ent->u.filename.inode,
                                  &ent->u.filename.fsize, NULL,
                                  &ent->u.filename.mtime, NULL);
                }
 
                curpos += ent->u.filename.offset;
                if (curpos < ent->u.filename.mapoff || 
                    curpos >= ent->u.filename.mapoff + ent->u.filename.maplen)
                {
#ifdef UNIX
                    if (ent->u.filename.pmap != NULL) {
                        file_munmap(ent->u.filename.pmap, ent->u.filename.maplen);
                        ent->u.filename.pmap = NULL;
                        ent->u.filename.pbyte = NULL;
                    }
                    ent->u.filename.pbyte = file_mmap(NULL, native_file_fd(ent->u.filename.hfile), 
                                                  curpos, 2048*1024,
                                                  PROT_READ, MAP_SHARED,
                                                  &ent->u.filename.pmap,
                                                  &ent->u.filename.maplen,
                                                  &ent->u.filename.mapoff);
#endif
#if defined(_WIN32) || defined(_WIN64)
                    if (ent->u.filename.pmap != NULL) {
                        file_munmap(ent->u.filename.hmap, ent->u.filename.pmap);
                        ent->u.filename.hmap = NULL;
                        ent->u.filename.pmap = NULL;
                        ent->u.filename.pbyte = NULL;
                    }
                    ent->u.filename.pbyte = file_mmap(NULL, native_file_handle(ent->u.filename.hfile),
                                                  curpos, 2048*1024,
                                                  NULL,
                                                  &ent->u.filename.hmap,
                                                  &ent->u.filename.pmap,
                                                  &ent->u.filename.maplen,
                                                  &ent->u.filename.mapoff);
#endif
                    if (ent->u.filename.pbyte == NULL)
                        return -100;
                }
                if (ind) *ind = i;
                return ((uint8 *)ent->u.filename.pmap)[curpos - ent->u.filename.mapoff];

            } else if (ent->cktype == CKT_FILE_PTR) {
                curpos += ent->u.fileptr.offset;
                if (curpos < ent->u.fileptr.mapoff ||
                    curpos >= ent->u.fileptr.mapoff + ent->u.fileptr.maplen)
                {
#ifdef UNIX
                    if (ent->u.fileptr.pmap != NULL) {
                        file_munmap(ent->u.fileptr.pmap, ent->u.fileptr.maplen);
                        ent->u.fileptr.pmap = NULL;
                        ent->u.fileptr.pbyte = NULL;
                    }
                    ent->u.fileptr.pbyte = file_mmap(NULL, fileno(ent->u.fileptr.fp),
                                                  ent->u.fileptr.offset + curpos,
                                                  2048*1024,
                                                  PROT_READ, MAP_SHARED,
                                                  &ent->u.fileptr.pmap,
                                                  &ent->u.fileptr.maplen,
                                                  &ent->u.fileptr.mapoff);
#endif
#if defined(_WIN32) || defined(_WIN64)
                    if (ent->u.fileptr.pmap != NULL) {
                        file_munmap(ent->u.fileptr.hmap, ent->u.fileptr.pmap);
                        ent->u.fileptr.hmap = NULL;
                        ent->u.fileptr.pmap = NULL;
                        ent->u.fileptr.pbyte = NULL;
                    }
                    ent->u.fileptr.pbyte = file_mmap(NULL, fd_to_file_handle(fileno(ent->u.fileptr.fp)),
                                                  curpos, 2048*1024,
                                                  NULL,
                                                  &ent->u.fileptr.hmap,
                                                  &ent->u.fileptr.pmap,
                                                  &ent->u.fileptr.maplen,
                                                  &ent->u.fileptr.mapoff);
#endif
                    if (ent->u.fileptr.pbyte == NULL)
                        return -100;
                }
                if (ind) *ind = i;
                return ((uint8 *)ent->u.fileptr.pmap)[curpos - ent->u.fileptr.mapoff];

            } else if (ent->cktype == CKT_FILE_DESC) {
                curpos += ent->u.filefd.offset;
                if (curpos < ent->u.filefd.mapoff ||
                    curpos >= ent->u.filefd.mapoff + ent->u.filefd.maplen)
                {
#ifdef UNIX
                    if (ent->u.filefd.pmap != NULL) {
                        file_munmap(ent->u.filefd.pmap, ent->u.filefd.maplen);
                        ent->u.filefd.pmap = NULL;
                        ent->u.filefd.pbyte = NULL;
                    }
                    ent->u.filefd.pbyte = file_mmap(NULL, ent->u.filefd.fd,
                                                  ent->u.filefd.offset + curpos,
                                                  2048*1024,
                                                  PROT_READ, MAP_SHARED,
                                                  &ent->u.filefd.pmap,
                                                  &ent->u.filefd.maplen,
                                                  &ent->u.filefd.mapoff);
#endif
#if defined(_WIN32) || defined(_WIN64)
                    if (ent->u.filefd.pmap != NULL) {
                        file_munmap(ent->u.filefd.hmap, ent->u.filefd.pmap);
                        ent->u.filefd.hmap = NULL;
                        ent->u.filefd.pmap = NULL;
                        ent->u.filefd.pbyte = NULL;
                    }
                    ent->u.filefd.pbyte = file_mmap(NULL, fd_to_file_handle(ent->u.filefd.fd),
                                                  curpos, 2048*1024,
                                                  NULL,
                                                  &ent->u.filefd.hmap,
                                                  &ent->u.filefd.pmap,
                                                  &ent->u.filefd.maplen,
                                                  &ent->u.filefd.mapoff);
#endif
                    if (ent->u.filefd.pbyte == NULL)
                        return -100;
                }
                if (ind) *ind = i;
                return ((uint8 *)ent->u.filefd.pmap)[curpos - ent->u.filefd.mapoff];
 
            } else if (ent->cktype == CKT_CALLBACK) {
                void   * pbyte = NULL;
                int64    bytelen = 0;
 
                ret = (*ent->u.callback.fetchfunc)(ent->u.callback.fetchpara, 
                                                   ent->u.callback.offset + curpos,
                                                   curlen, &pbyte, &bytelen);
                if (ret >= 0) {
                    if (ind) *ind = i;
                    return ((uint8 *)pbyte)[0];
                } else return -101;
            }
 
            readlen += curlen;
            readpos += curlen;
        }
        accentlen += ent->length;
    }
 
    return -200;
}

void * chunk_ptr (void * vck, int64 pos, int * ind, void ** ppbuf, int64 * plen)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int        i, num, ret;
 
    int64      accentlen = 0;
 
    int64      readpos = 0;
    int64      readlen = 0;
    int64      curpos = 0;
    int64      curlen = 0;
 
    if (!ck) return NULL;
 
    if (pos < ck->rmentlen) return NULL;
    if (pos >= ck->size) return NULL;
 
    accentlen = ck->rmentlen;
 
    readpos = pos;
    readlen = 0;
    ck->seekpos = pos;
 
    num = arr_num(ck->entity_list);
    for (i = 0; i < num; i++) {
 
        ent = arr_value(ck->entity_list, i);
        if (!ent) continue;
 
        if (ent->length > 0 && readpos < accentlen + ent->length) {
            curpos = readpos - accentlen;
            curlen = ent->length - curpos;
 
            if (ent->cktype == CKT_CHAR_ARRAY) {
                if (ind) *ind = i;
                if (ppbuf) *ppbuf = ent->u.charr.pbyte + curpos;
                if (plen) *plen = curlen;
                return ent->u.charr.pbyte + curpos;
 
            } else if (ent->cktype == CKT_BUFFER) {
                if (ind) *ind = i;
                if (ppbuf) *ppbuf = (uint8 *)ent->u.buf.pbyte + curpos;
                if (plen) *plen = curlen;
                return (uint8 *)ent->u.buf.pbyte + curpos;
 
            } else if (ent->cktype == CKT_BUFFER_PTR) {
                if (ind) *ind = i;
                if (ppbuf) *ppbuf = (uint8 *)ent->u.bufptr.pbyte + curpos;
                if (plen) *plen = curlen;
                return (uint8 *)ent->u.bufptr.pbyte + curpos;
 
            } else if (ent->cktype == CKT_FILE_NAME) {
                if (ent->u.filename.hfile == NULL) {
                    ent->u.filename.hfile = native_file_open(ent->u.filename.fname, NF_READ);

                    if (ent->u.filename.fsize != native_file_size(ent->u.filename.hfile))
                        file_attr(ent->u.filename.fname,
                                  &ent->u.filename.inode,
                                  &ent->u.filename.fsize, NULL,
                                  &ent->u.filename.mtime, NULL);
                }
 
                curpos += ent->u.filename.offset;
                if (curpos < ent->u.filename.mapoff ||
                    curpos >= ent->u.filename.mapoff + ent->u.filename.maplen)
                {
#ifdef UNIX
                    if (ent->u.filename.pmap != NULL) {
                        file_munmap(ent->u.filename.pmap, ent->u.filename.maplen);
                        ent->u.filename.pmap = NULL;
                        ent->u.filename.pbyte = NULL;
                    }
                    ent->u.filename.pbyte = file_mmap(NULL, native_file_fd(ent->u.filename.hfile),
                                                  curpos, 2048*1024,
                                                  PROT_READ, MAP_SHARED,
                                                  &ent->u.filename.pmap,
                                                  &ent->u.filename.maplen,
                                                  &ent->u.filename.mapoff);
#endif
#if defined(_WIN32) || defined(_WIN64)
                    if (ent->u.filename.pmap != NULL) {
                        file_munmap(ent->u.filename.hmap, ent->u.filename.pmap);
                        ent->u.filename.hmap = NULL;
                        ent->u.filename.pmap = NULL;
                        ent->u.filename.pbyte = NULL;
                    }
                    ent->u.filename.pbyte = file_mmap(NULL, native_file_handle(ent->u.filename.hfile),
                                                  curpos, 2048*1024,
                                                  NULL,
                                                  &ent->u.filename.hmap,
                                                  &ent->u.filename.pmap,
                                                  &ent->u.filename.maplen,
                                                  &ent->u.filename.mapoff);
#endif

                    if (ent->u.filename.pbyte == NULL)
                        return NULL;
                }

                if (curlen > ent->u.filename.maplen - (curpos - ent->u.filename.mapoff))
                    curlen = ent->u.filename.maplen - (curpos - ent->u.filename.mapoff);

                if (ind) *ind = i;
                if (ppbuf) *ppbuf = (uint8 *)ent->u.filename.pmap + curpos - ent->u.filename.mapoff;
                if (plen) *plen = curlen;
                return (uint8 *)ent->u.filename.pmap + curpos - ent->u.filename.mapoff;
 
            } else if (ent->cktype == CKT_FILE_PTR) {
                curpos += ent->u.fileptr.offset;
                if (curpos < ent->u.fileptr.mapoff ||
                    curpos >= ent->u.fileptr.mapoff + ent->u.fileptr.maplen)
                {
#ifdef UNIX
                    if (ent->u.fileptr.pmap != NULL) {
                        file_munmap(ent->u.fileptr.pmap, ent->u.fileptr.maplen);
                        ent->u.fileptr.pmap = NULL;
                        ent->u.fileptr.pbyte = NULL;
                    }
                    ent->u.fileptr.pbyte = file_mmap(NULL, fileno(ent->u.fileptr.fp),
                                                  curpos, 2048*1024,
                                                  PROT_READ, MAP_SHARED,
                                                  &ent->u.fileptr.pmap,
                                                  &ent->u.fileptr.maplen,
                                                  &ent->u.fileptr.mapoff);
#endif
#if defined(_WIN32) || defined(_WIN64)
                    if (ent->u.fileptr.pmap != NULL) {
                        file_munmap(ent->u.fileptr.hmap, ent->u.fileptr.pmap);
                        ent->u.fileptr.hmap = NULL;
                        ent->u.fileptr.pmap = NULL;
                        ent->u.fileptr.pbyte = NULL;
                    }
                    ent->u.fileptr.pbyte = file_mmap(NULL, fd_to_file_handle(fileno(ent->u.fileptr.fp)),
                                                  curpos, 2048*1024,
                                                  NULL,
                                                  &ent->u.fileptr.hmap,
                                                  &ent->u.fileptr.pmap,
                                                  &ent->u.fileptr.maplen,
                                                  &ent->u.fileptr.mapoff);
#endif
                    if (ent->u.fileptr.pbyte == NULL)
                        return NULL;
                }

                if (curlen > ent->u.fileptr.maplen - (curpos - ent->u.fileptr.mapoff))
                    curlen = ent->u.fileptr.maplen - (curpos - ent->u.fileptr.mapoff);

                if (ind) *ind = i;
                if (ppbuf) *ppbuf = (uint8 *)ent->u.fileptr.pmap + curpos - ent->u.fileptr.mapoff;
                if (plen) *plen = curlen;
                return (uint8 *)ent->u.fileptr.pmap + curpos - ent->u.fileptr.mapoff;
 
            } else if (ent->cktype == CKT_FILE_DESC) {
                curpos += ent->u.filefd.offset;
                if (curpos < ent->u.filefd.mapoff ||
                    curpos >= ent->u.filefd.mapoff + ent->u.filefd.maplen)
                {
#ifdef UNIX
                    if (ent->u.filefd.pmap != NULL) {
                        file_munmap(ent->u.filefd.pmap, ent->u.filefd.maplen);
                        ent->u.filefd.pmap = NULL;
                        ent->u.filefd.pbyte = NULL;
                    }
                    ent->u.filefd.pbyte = file_mmap(NULL, ent->u.filefd.fd,
                                                  curpos, 2048*1024,
                                                  PROT_READ, MAP_SHARED,
                                                  &ent->u.filefd.pmap,
                                                  &ent->u.filefd.maplen,
                                                  &ent->u.filefd.mapoff);
#endif
#if defined(_WIN32) || defined(_WIN64)
                    if (ent->u.filefd.pmap != NULL) {
                        file_munmap(ent->u.filefd.hmap, ent->u.filefd.pmap);
                        ent->u.filefd.hmap = NULL;
                        ent->u.filefd.pmap = NULL;
                        ent->u.filefd.pbyte = NULL;
                    }
                    ent->u.filefd.pbyte = file_mmap(NULL, fd_to_file_handle(ent->u.filefd.fd),
                                                  curpos, 2048*1024,
                                                  NULL,
                                                  &ent->u.filefd.hmap,
                                                  &ent->u.filefd.pmap,
                                                  &ent->u.filefd.maplen,
                                                  &ent->u.filefd.mapoff);
#endif
                    if (ent->u.filefd.pbyte == NULL)
                        return NULL;
                }

                if (curlen > ent->u.filefd.maplen - (curpos - ent->u.filefd.mapoff))
                    curlen = ent->u.filefd.maplen - (curpos - ent->u.filefd.mapoff);

                if (ind) *ind = i;
                if (ppbuf) *ppbuf = (uint8 *)ent->u.filefd.pmap + curpos - ent->u.filefd.mapoff;
                if (plen) *plen = curlen;
                return (uint8 *)ent->u.filefd.pmap + curpos - ent->u.filefd.mapoff;
 
            } else if (ent->cktype == CKT_CALLBACK) {
                void   * pbyte = NULL;
                int64    bytelen = 0;
 
                ret = (*ent->u.callback.fetchfunc)(ent->u.callback.fetchpara,
                                                   ent->u.callback.offset + curpos,
                                                   curlen, &pbyte, &bytelen);
                if (ret >= 0) {
                    if (ind) *ind = i;
                    if (ppbuf) *ppbuf = pbyte;
                    if (plen) *plen = bytelen;
                    return pbyte;
                } else return NULL;
            }
 
            readlen += curlen;
            readpos += curlen;
        }
        accentlen += ent->length;
    }
 
    return NULL;
}

int chunk_char (void * vck, int64 pos, ckpos_vec_t * pvec, int * pind)
{
    chunk_t  * ck = (chunk_t *)vck;
 
    if (!ck) return -1;
    if (!pvec) return -2;
 
    if (!pvec->pbyte || pvec->bytelen <= 0 || pos < pvec->offset || pos >= pvec->offset + pvec->bytelen) {
        if (chunk_ptr(ck, pos, &pvec->entind, (void **)&pvec->pbyte, &pvec->bytelen) == NULL)
            return  -100;
 
        pvec->offset = pos;
    }
 
    if (pvec->pbyte && pvec->bytelen > 0 && pos >= pvec->offset && pos < pvec->offset + pvec->bytelen) {
        if (pind) *pind = pvec->entind;
        return pvec->pbyte[pos - pvec->offset];
    }
 
    return -200;
}
 

int64 chunk_skip_to (void * vck, int64 pos, int64 skiplimit, void * vpat, int patlen)
{
    chunk_t  * ck = (chunk_t *)vck;
    uint8    * pat = (uint8 *)vpat;
    int        fch = 0, j;
    int64      i = 0, fsize;

    ckpos_vec_t posvec = {0};
 
    if (!ck) return -1;
    if (!pat || patlen <= 0) return pos;
    if (pos >= ck->size) return ck->size;
 
    fsize = ck->size - pos;
    if (skiplimit >= 0 && skiplimit < fsize)
        fsize = skiplimit;
 
    for (i = 0; i < fsize; i++) {
        fch = chunk_char(ck, pos + i, &posvec, NULL);
        if (fch < 0) return pos + i;

        for (j = 0; j < patlen; j++) {
            if (pat[j] == fch) return pos + i;
        }
    }
 
    return pos + i;
}

int64  chunk_rskip_to(void * vck, int64 pos, int64 skiplimit, void * vpat, int patlen)
{
    chunk_t  * ck = (chunk_t *)vck;
    uint8    * pat = (uint8 *)vpat;
    int        fch = 0, j;
    int64      i = 0;
 
    if (!ck) return -1;
    if (!pat || patlen <= 0) return pos;
 
    if (pos < ck->rmentlen) return ck->rmentlen;
    if (pos >= ck->size) pos = ck->size - 1;
 
    for (i = 0; i <= pos - ck->rmentlen + 1; i++) {
        if (skiplimit >= 0 && i >= skiplimit)
            break;
 
        fch = chunk_at(ck, pos - i, NULL);
        if (fch < 0) break;
 
        for (j = 0; j < patlen; j++) {
            if (pat[j] == fch) return pos - i;
        }
    }
 
    return pos - i;
}
 
int64 chunk_skip_over (void * vck, int64 pos, int64 skiplimit, void * vpat, int patlen)
{
    chunk_t  * ck = (chunk_t *)vck;
    uint8    * pat = (uint8 *)vpat;
    int        fch = 0, j;
    int64      i = 0, fsize;
 
    ckpos_vec_t posvec = {0};
 
    if (!ck) return -1;
    if (!pat || patlen <= 0) return pos;
    if (pos >= ck->size) return ck->size;
 
    fsize = ck->size - pos;
    if (skiplimit >= 0 && skiplimit < fsize)
        fsize = skiplimit;
 
    for (i = 0; i < fsize; i++) {
        fch = chunk_char(ck, pos + i, &posvec, NULL);
        if (fch < 0) return pos + i;

        for (j = 0; j < patlen; j++) {
            if (pat[j] == fch) break;
        }

        if (j >= patlen) return pos + i;
    }
 
    return pos + i;
}

int64 chunk_rskip_over(void * vck, int64 pos, int64 skiplimit, void * vpat, int patlen)
{
    chunk_t  * ck = (chunk_t *)vck;
    uint8    * pat = (uint8 *)vpat;
    int        fch = 0, j;
    int64      i = 0;
 
    if (!ck) return -1;
    if (!pat || patlen <= 0) return pos;
 
    if (pos <= ck->rmentlen) return ck->rmentlen;
    if (pos >= ck->size) pos = ck->size - 1;
 
    for (i = 0; i <= pos - ck->rmentlen + 1; i++) {
        if (skiplimit >= 0 && i >= skiplimit)
            break;
 
        fch = chunk_at(ck, pos - i, NULL);
        if (fch < 0) break;
 
        for (j = 0; j < patlen; j++) {
            if (pat[j] == fch) break;
        }

        if (j >= patlen) return pos - i;
    }
 
    return pos - i;
}
 
static int64 chunk_qstrlen (void * vck, int64 pos, int64 skiplimit)
{
    chunk_t  * ck = (chunk_t *)vck;
    int        fch = 0;
    int        quote = 0;
    int64      i, fsize;

    ckpos_vec_t posvec = {0};
 
    if (!ck) return 0;

    fsize = ck->size;
    if (pos >= fsize) return 0;

    quote = chunk_char(ck, pos, &posvec, NULL);
    if (quote != '"' && quote != '\'') return 0;

    for (i = 1; i < skiplimit && i < fsize - pos; i++) {
        fch = chunk_char(ck, pos + i, &posvec, NULL);
        if (fch < 0) return i;

        if (fch == '\\') i++;
        else if (fch == quote) return i + 1;
    }

    return 1;
}

int64 chunk_skip_quote_to(void * vck, int64 pos, int64 skiplimit, void * vpat, int patlen)
{
    chunk_t  * ck = (chunk_t *)vck;
    int        j, fch = 0;
    uint8    * pat = (uint8 *)vpat;
    int64      fsize = 0, i, qlen;

    ckpos_vec_t posvec = {0};
 
    if (!ck) return -1;
    if (!pat || patlen <= 0) return pos;
 
    fsize = ck->size - pos;
    if (skiplimit >= 0 && skiplimit < fsize)
        fsize = skiplimit;
 
    for (i = 0; i < fsize; ) {
        fch = chunk_char(ck, pos + i, &posvec, NULL);
        if (fch < 0) return pos + i;

        if (fch == '\\' && i + 1 < fsize) {
            i += 2;
            continue;
        }

        for (j = 0; j < patlen; j++) {
            if (pat[j] == fch) return pos + i;
        }
 
        if (fch == '"' || fch == '\'') {
            qlen = chunk_qstrlen(ck, pos + i, fsize - i);
            i += qlen;
            continue;
        }
        i++;
    }

    return pos + i;
}

int64 chunk_skip_esc_to  (void * vck, int64 pos, int64 skiplimit, void * vpat, int patlen)
{
    chunk_t  * ck = (chunk_t *)vck;
    uint8    * pat = (uint8 *)vpat;
    int        fch = 0, j;
    int64      i = 0, fsize;
 
    ckpos_vec_t posvec = {0};
 
    if (!ck) return -1;
    if (!pat || patlen <= 0) return pos;
    if (pos >= ck->size) return ck->size;
 
    fsize = ck->size - pos;
    if (skiplimit >= 0 && skiplimit < fsize)
        fsize = skiplimit;
 
    for (i = 0; i < fsize; i++) {
        fch = chunk_char(ck, pos + i, &posvec, NULL);
        if (fch < 0) return pos + i;

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

int64 sun_find_chunk (void * vck, int64 pos, void * pattern, int patlen, void * pvec, int * pind)
{
    chunk_t      * ck = (chunk_t *)vck;
    uint8        * pat = (uint8 *)pattern;
    pat_sunvec_t * patvec = (pat_sunvec_t *)pvec;
    uint8          alloc = 0;
    int            i = 0;
    uint8          matchfound = 0;
    ckpos_vec_t    posvec = {0};
 
    if (!ck) return -1;

    if (pos < ck->rmentlen || pos >= ck->size)
        return -2;

    if (!pat) return -3;
    if (patlen < 0) patlen = str_len(pat);
    if (patlen <= 0)
        return -3;
 
    if (patlen > chunk_rest_size(ck, 0))
        return -4;
 
    if (!patvec) {
        patvec = pat_sunvec_alloc(pat, patlen, 0);
        alloc = 1;
    }
 
    if (!patvec) return -10;
 
    for ( ; pos <= ck->size - patlen; ) {
 
        for (i = 0; i < patlen && pat[i] == chunk_char(ck, i + pos, &posvec, pind); i++);
 
        if (i >= patlen) {
            matchfound = 1;
            break;
 
        } else {
            pos += patlen - patvec->chloc[ chunk_char(ck, pos + patlen, &posvec, pind) ];
        }
    }
 
    if (alloc) pat_sunvec_free(patvec);
 
    if (matchfound)
        return pos;
 
    return -100;
}

int64 bm_find_chunk (void * vck, int64 pos, void * pattern, int patlen, void * pvec, int * pind)
{
    chunk_t     * ck = (chunk_t *)vck;
    uint8       * pat = (uint8 *)pattern;
    pat_bmvec_t * patvec = (pat_bmvec_t *)pvec;
    uint8         alloc = 0;
    int           i = 0;
    uint8         matchfound = 0;
    ckpos_vec_t   posvec = {0};
 
    if (!ck) return -1;
 
    if (pos < ck->rmentlen || pos >= ck->size)
        return -2;
 
    if (!pat) return -3;
    if (patlen < 0) patlen = str_len(pat);
    if (patlen <= 0) 
        return -3;
 
    if (patlen > chunk_rest_size(ck, 0))
        return -4;
 
    if (!patvec) {
        patvec = pat_bmvec_alloc(pat, patlen, 0);
        alloc = 1;
    }
 
    if (!patvec) return -10;
 
    for ( ; pos < ck->size - patlen; ) {
        for (i = patlen - 1;
             i >= 0 && pat[i] == chunk_char(ck, pos + i, &posvec, pind);
             i--);
 
        if (i < 0) {
            matchfound = 1;
            break;
        } else {
            pos += max( patvec->goodsuff[i],
                        patvec->badch[ chunk_char(ck, pos + i, &posvec, pind) ] - patlen + 1 + i );
        }
    }
 
    if (alloc) pat_bmvec_free(patvec);
 
    if (matchfound)
        return pos;
 
    return -100;
}

int64 kmp_find_chunk (void * vck, int64 pos, void * pattern, int patlen, void * pvec, int * pind)
{
    chunk_t      * ck = (chunk_t *)vck;
    uint8        * pat = (uint8 *)pattern;
    pat_kmpvec_t * patvec = (pat_kmpvec_t *)pvec;
    uint8          alloc = 0;
    int            patpos = 0;
    ckpos_vec_t    posvec = {0};
 
    if (!ck) return -1;
 
    if (pos < ck->rmentlen || pos >= ck->size)
        return -2;
 
    if (!pat) return -3;
    if (patlen < 0) patlen = str_len(pat);
    if (patlen <= 0) 
        return -3;
 
    if (patlen > chunk_rest_size(ck, 0))
        return -4;
 
    if (!patvec) {
        patvec = pat_kmpvec_alloc(pat, patlen, 0, 0);
        alloc = 1;
    }
 
    if (!patvec) return -10;
 
    for (patpos = 0; pos < ck->size && patpos < patlen; ) {
 
        if (chunk_char(ck, pos, &posvec, pind) == pat[patpos]) {
            pos++;
            patpos++;
 
        } else if (patpos == 0) {
            pos++;
 
        } else {
            patpos = patvec->failvec[patpos - 1] + 1;
        }
    }
 
    if (alloc) pat_kmpvec_free(patvec);
 
    if (patpos < patlen) return -100;
 
    return pos - patlen;
}

void chunk_print (void * vck, FILE * fp)
{
    chunk_t  * ck = (chunk_t *)vck;
    ckent_t  * ent = NULL;
    int        i, num;
    char       buf[2048];

    if (!ck) return;

    if (!fp) fp = stdout;

    num = arr_num(ck->entity_list);

    sprintf(buf, "----------------chunk=%p-----------------\n", ck);
    sprintf(buf+strlen(buf), "size=%lld rmentlen=%lld endsize=%lld chunksize=%lld"
                             " rmchunklen=%lld chunkendsize=%lld\n",
            ck->size, ck->rmentlen, ck->endsize, ck->chunksize, ck->rmchunklen, ck->chunkendsize);
    sprintf(buf+strlen(buf), "filenum=%d bufnum=%d entitynum=%d\n", ck->filenum, ck->bufnum, num);
    fprintf(fp, buf); buf[0] = '\0';

    num = arr_num(ck->entity_list);
    for (i = 0; i < num; i++) {
 
        ent = arr_value(ck->entity_list, i);
        if (!ent) continue;

        if (ent->cktype == CKT_CHAR_ARRAY) {
            sprintf(buf+strlen(buf), "  CHAR_ARRAY len=%lld hdr=%d\n", ent->length, ent->header);
        } else if (ent->cktype == CKT_BUFFER) {
            sprintf(buf+strlen(buf), "  BUFFER     len=%lld hdr=%d\n", ent->length, ent->header);

        } else if (ent->cktype == CKT_BUFFER_PTR) {
            sprintf(buf+strlen(buf), "  BUFFER_PTR len=%lld hdr=%d\n", ent->length, ent->header);

        } else if (ent->cktype == CKT_FILE_NAME) {
            sprintf(buf+strlen(buf), "  FILE_NAME  len=%lld hdr=%d\n", ent->length, ent->header);
            sprintf(buf+strlen(buf), "    offset=%lld fsize=%lld inode=%lu mtime=%lu mapoff=%lld maplen=%lld\n",
                    ent->u.filename.offset, ent->u.filename.fsize, ent->u.filename.inode,
                    ent->u.filename.mtime, ent->u.filename.mapoff, ent->u.filename.maplen);
            sprintf(buf+strlen(buf), "    fname=%s\n", ent->u.filename.fname);

        } else if (ent->cktype == CKT_FILE_PTR) {
            sprintf(buf+strlen(buf), "  FILE_PTR   len=%lld hdr=%d\n", ent->length, ent->header);
            sprintf(buf+strlen(buf), "    offset=%lld fsize=%lld inode=%lu mtime=%lu mapoff=%lld maplen=%lld\n",
                    ent->u.fileptr.offset, ent->u.fileptr.fsize, ent->u.fileptr.inode,
                    ent->u.fileptr.mtime, ent->u.fileptr.mapoff, ent->u.fileptr.maplen);

        } else if (ent->cktype == CKT_FILE_DESC) {
            sprintf(buf+strlen(buf), "  FILE_DESC  len=%lld hdr=%d\n", ent->length, ent->header);
            sprintf(buf+strlen(buf), "    offset=%lld fsize=%lld inode=%lu mtime=%lu mapoff=%lld maplen=%lld\n",
                    ent->u.filefd.offset, ent->u.filefd.fsize, ent->u.filefd.inode,
                    ent->u.filefd.mtime, ent->u.filefd.mapoff, ent->u.filefd.maplen);

        } else if (ent->cktype == CKT_CALLBACK) {
            sprintf(buf+strlen(buf), "  CALLBACK   len=%lld hdr=%d\n", ent->length, ent->header);

        } else {
            sprintf(buf+strlen(buf), "  UNKNOWN    len=%lld hdr=%d\n", ent->length, ent->header);
        }
        fprintf(fp, buf); buf[0] = '\0';
    }
    fprintf(fp, "------------------------end chunk---------------------\n");
    fflush(fp);
}

