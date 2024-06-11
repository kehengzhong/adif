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

#ifndef _CHUNK_H_
#define _CHUNK_H_ 
    
#include "tsock.h"

#ifdef __cplusplus
extern "C" {
#endif 

typedef int FetchData (void * para, int64 offset, int64 lenght, void ** ppbyte, int64 * pbytelen);
typedef int GoAhead   (void * para, int64 offset, int64 step);
typedef int FetchEnd  (void * para, int status);
typedef void CKEFree  (void * obj);

typedef int ProcessNotify (void * msg, void * para, uint64 cbval, int64 offset, int64 step);

#define CKT_UNKNOWN     0
#define CKT_CHAR_ARRAY  1
#define CKT_BUFFER      2
#define CKT_BUFFER_PTR  3
#define CKT_MPOOL_BUF   4
#define CKT_FILE_NAME   5
#define CKT_FILE_PTR    6
#define CKT_FILE_DESC   7
#define CKT_CALLBACK    8


typedef struct chunk_entity {
    uint8           cktype : 6;
    uint8           header : 1;
    int64           length;

    char            lenstr[20];
    int             lenstrlen;
    char            trailer[3];
    int             trailerlen;

    union {
        struct {
            char       pbyte[48];
        } charr;

        struct {
            void     * pbyte;
        } buf;

        struct {
            void     * pbyte;
            void     * porig;
            CKEFree  * freefunc;
        } bufptr;

        struct {
            void     * pbyte;
            void     * mpool;
        } mpbuf;

        struct {
#if defined(_WIN32) || defined(_WIN64)
            HANDLE     hmap;
#endif
            void     * pbyte;
            void     * pmap;
            int64      maplen;
            int64      mapoff;
            int64      offset;
            int64      fsize;
            long       inode;
            time_t     mtime;
            void     * hfile;
            char     * fname;
        } filename;

        struct {
#if defined(_WIN32) || defined(_WIN64)
            HANDLE     hmap;
#endif
            void     * pbyte;
            void     * pmap;
            int64      maplen;
            int64      mapoff;
            int64      offset;
            int64      fsize;
            long       inode;
            time_t     mtime;
            FILE     * fp;
        } fileptr;

        struct {
#if defined(_WIN32) || defined(_WIN64)
            HANDLE     hmap;
#endif
            void     * pbyte;
            void     * pmap;
            int64      maplen;
            int64      mapoff;
            int64      offset;
            int64      fsize;
            long       inode;
            time_t     mtime;
            int        fd;
        } filefd;

        struct {
            FetchData * fetchfunc;
            void      * fetchpara;
            GoAhead   * movefunc;
            void      * movepara;
            FetchEnd  * endfunc;
            void      * endpara;
            int64       offset;
        } callback;
    } u;
} ckent_t, *ckent_p;

void   chunk_entity_free (void * ck, void * pent);

int    size_hex_len (int64 size);


typedef struct chunk_ {
    arr_t         * entity_list;

    /* HTTP Chunk format as following:
     * 24E5CRLF            #chunk-sizeCRLF  (first chunk begin)
     * 24E5(byte)CRLF      #(chunk-size octet)CRLF
     * 38A1CRLF            #chunk-sizeCRLF  (another chunk begin)
     * 38A1(byte)CRLF      #(chunk-size octet)CRLF
     * ......              #one more chunks may be followed
     * 0CRLF               #end of all chunks
     * X-bid-for: abcCRLF  #0-more HTTP Headers as entity header
     * .....               #one more HTTP headers with trailing CRLF
     * CRLF
     */
    uint8           httpchunk   : 4;
    uint8           alloctype   : 4; //0-default kalloc/kfree 1-os-specific malloc/free 2-kmempool alloc/free 3-kmemblk alloc/free

    void          * mpool;  //kemalloc pool

    int64           rmchunklen;      //total length of removed entities
    int64           chunksize;
    int64           chunkendsize;

    int64           rmentlen;   //total length of removed entities
    int64           size;
    int64           endsize;

    int             filenum;
    int             bufnum;

    int64           seekpos;

    uint8         * loadbuf;
    int             lblen;
    int             lbsize;

    ProcessNotify * procnotify;
    void          * procnotifypara;
    uint64          procnotifycbval;

} chunk_t, *chunk_p;

void * chunk_alloc (int buflen, int alloctype, void * mpool);
void * chunk_new   (int buflen);
void   chunk_free  (void * vck);

void   chunk_zero  (void * vck);

int64  chunk_size      (void * vck, int httpchunk);
int64  chunk_rest_size (void * vck, int httpchunk);
int64  chunk_startpos  (void * vck, int httpchunk);

int64  chunk_seekpos (void * vck);
int64  chunk_seek    (void * vck, int64 offset);

int    chunk_is_file (void * vck, int64 * fsize, time_t * mtime, long * inode, char ** fname);

int    chunk_num (void * vck);

int    chunk_has_file (void * vck);
int    chunk_has_buf  (void * vck);

int    chunk_get_end (void * vck, int64 pos, int httpchunk);
int    chunk_set_end (void * vck);

void   chunk_set_httpchunk (void * vck, int httpchunk);

int    chunk_attr  (void * vck, int index, int * cktype, int64 * plen);

int64  chunk_read         (void * vck, void * pbuf, int64 offset, int64 length, int httpchunk);
int64  chunk_read_ptr     (void * vck, int64 pos, int64 len, void ** ppb, int64 * pblen, int httpchunk);
int64  chunk_write_frame  (void * vck, frame_p frm, int64 offset, int64 length, int httpchunk);
int64  chunk_write_framep (void * vck, frame_p * pfrm, int64 offset, int64 length, int httpchunk);
int64  chunk_write_file   (void * vck, int fd, int64 offset, int64 length, int httpchunk);

int    chunk_go_ahead (void * vck, void * msg, int64 offset, int64 step);

int    chunk_add_buffer       (void * vck, void * pbuf, int64 len);
int64  chunk_prepend_strip_buffer (void * vck, void * pbuf, int64 len, char * escch, int chlen, uint8 isheader);
int64  chunk_add_strip_buffer     (void * vck, void * pbuf, int64 len, char * escch, int chlen);
int64  chunk_append_strip_buffer  (void * vck, void * pbuf, int64 len, char * escch, int chlen);

int    chunk_prepend_bufptr (void * vck, void * pbuf, int64 len, void * porig, void * freefunc, uint8 isheader);
int    chunk_add_bufptr     (void * vck, void * pbuf, int64 len, void * porig, void * freefunc);
int    chunk_append_bufptr  (void * vck, void * pbuf, int64 len, void * porig, void * freefunc);

int    chunk_remove_bufptr  (void * vck, void * pbuf);
int    chunk_bufptr_porig_find (void * vck, void * porig);

int    chunk_add_file     (void * vck, char * fname, int64 offset, int64 length, int merge);
int64  chunk_prepend_file (void * vck, char * fname, int64 packsize);
int64  chunk_append_file  (void * vck, char * fname, int64 packsize);

int    chunk_add_filefp   (void * vck, FILE * fp, int64 offset, int64 length);
int    chunk_add_filefd   (void * vck, int fd, int64 offset, int64 length);

int    chunk_remove_file (void * vck);

int    chunk_add_cbdata (void * vck, void * fetchfunc, void * fetchobj, int64 offset, int64 length,
                         void * movefunc, void * movepara, void * cleanfunc, void * cleanobj);

int    chunk_add_process_notify (void * vck, void * movefunc, void * movepara, uint64 movecbval);

int    chunk_remove (void * vck, int64 pos, int httpchunk);

int    chunk_copy (void * vsrc, void * vdst, void * porig, void * freefunc);

typedef struct {
    uint8         vectype;  //0-unknown  1-mem buffer  2-file

    int64         offset;   //offset from the begining of the chunk
    int64         size;     //available size

    struct iovec  iovs[192];
    int           iovcnt;

    int           filefd;
#if defined(_WIN32) || defined(_WIN64)
    HANDLE        filehandle;
#endif
    void        * hfile;   //native_file handle or FILE * pointer

    int64         fpos;      //file current offset
    int64         filesize;  //actual file size
} chunk_vec_t;

int    chunk_buf_and_file (void * vck);
int    chunk_vec_get (void * vck, int64 pos, chunk_vec_t * pvec, int httpchunk);

int    chunk_writev (void * vck, int fd, int64 offset, int64 * actnum, int httpchunk);

/* access the contents of the chunk by specifying an offset or using pattern matching */

typedef struct ckpos_vec {
    uint8    * pbyte;
    int64      bytelen;
    int64      offset;
    int        entind;
} ckpos_vec_t;
 
int    chunk_char (void * vck, int64 pos, ckpos_vec_t * pvec, int * pind);

int    chunk_at  (void * vck, int64 pos, int * ind);
void * chunk_ptr (void * vck, int64 pos, int * ind, void ** ppbuf, int64 * plen);

int64  chunk_skip_to (void * vck, int64 pos, int64 skiplimit, void * vpat, int patlen);
int64  chunk_rskip_to(void * vck, int64 pos, int64 skiplimit, void * vpat, int patlen);

int64  chunk_skip_over (void * vck, int64 pos, int64 skiplimit, void * vpat, int patlen);
int64  chunk_rskip_over(void * vck, int64 pos, int64 skiplimit, void * vpat, int patlen);

int64  chunk_skip_quote_to(void * vck, int64 pos, int64 skiplimit, void * vpat, int patlen);
int64  chunk_skip_esc_to  (void * vck, int64 pos, int64 skiplimit, void * vpat, int patlen);

int64  chunk_sun_find_bytes (void * vck, int64 pos, void * pattern, int patlen, void * pvec, int * pind);
int64  chunk_bm_find_bytes (void * vck, int64 pos, void * pattern, int patlen, void * pvec, int * pind);
int64  chunk_kmp_find_bytes (void * vck, int64 pos, void * pattern, int patlen, void * pvec, int * pind);

void   chunk_print (void * vck, FILE * fp);


#ifdef __cplusplus
}
#endif 
 
#endif

