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

#ifndef _FRAME_ST_H_
#define _FRAME_ST_H_

#include "btype.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FrameST_ {

    unsigned   allocnum  : 30;
    unsigned   alloctype : 2; //0-default kalloc/kfree 1-os-specific malloc/free 2-kmempool alloc/free 3-kmemblk alloc/free

    int        size;
    int        start;
    int        len;

    void     * mpool;
    uint8    * data;

    struct FrameST_ * next;

} frame_t, *frame_p;

#define frame_alloc(size, atype, mpool)  frame_alloc_dbg((size), (atype), (mpool), __FILE__, __LINE__)
frame_p frame_alloc_dbg (int size, int alloctype, void * mpool, char * file, int line);

//frame_p frame_new    (int size);
#define frame_new(size)  frame_new_dbg((size), __FILE__, __LINE__)
frame_p frame_new_dbg    (int size, char * file, int line);

void    frame_free   (frame_t * frm);
void    frame_free_inner (frame_p frm);
void    frame_delete (frame_p * pfrm);

int     frame_size (frame_p frm);
//#define frame_size(frm)  ((frm) ? (frm)->size : 0)

#define frame_start(frm) ((frm) ? (frm)->start : 0)
#define frame_rest(frm)  ((frm) ? (frm)->size - (frm)->start - (frm)->len : 0)

#define frameL(frm)      ((frm) ? (frm)->len : 0)
#define frame_len(frm)   ((frm) ? (frm)->len : 0)
#define frame_len_set(frm, slen)   ((frm) ? ((frm)->len = (slen)) : 0)
#define frame_len_add(frm, slen)   ((frm) ? ((frm)->len += (slen)) : 0)

#define frameP(frm)    ((frm) ? (void *)((frm)->data + (frm)->start) : NULL)
#define frame_bgn(frm) ((frm) ? (void *)((frm)->data + (frm)->start) : NULL)
#define frame_end(frm) ((frm) ? (void *)((frm)->data + (frm)->start + (frm)->len) : (void *)frm)

#define frameAt(frm, pos) ((frm) ? ((frm)->data[(frm)->start + pos] : -1)

#define frameS(frm)  frame_string(frm)
char  * frame_string (frame_p frm);
void    frame_strip  (frame_p frm);

void    frame_empty (frame_p frm);
frame_p frame_dup   (frame_p frm);

#define frame_grow(frm, size)  frame_grow_dbg((frm), (size), __FILE__, __LINE__)
void    frame_grow_dbg (frame_p frm, int addsize, char * file, int line);
#define frame_grow_to(frm, size)  frame_grow_to_dbg((frm), (size), __FILE__, __LINE__)
void    frame_grow_to_dbg (frame_p frm, int dstsize, char * file, int line);
#define frame_grow_head(frm, size)  frame_grow_head_dbg((frm), (size), __FILE__, __LINE__)
void    frame_grow_head_dbg (frame_p frm, int size, char * file, int line);

void    frame_print (frame_p, FILE * fp, int start, int num, int margin);


int     frame_get  (frame_p frm, int pos);
int     frame_getn (frame_p frm, int pos, void * pbyte, int n);

int     frame_get_first  (frame_p frm);
int     frame_get_nfirst (frame_p frm, void * pbyte, int n);

int     frame_get_last (frame_p frm);
int     frame_get_nlast (frame_p frm, void * pbyte, int n);

#define frame_peek(frm, pos) frame_read((frm), (pos))
#define frame_peekn(frm, pos, pbyte, n) frame_readn((frm), (pos), (pbyte), (n))
int     frame_read  (frame_p frm, int pos);
int     frame_readn (frame_p frm, int pos, void * pbyte, int n);

void    frame_put_first (frame_p frm, int byte);
void    frame_put_firstp (frame_p * pfrm, int byte);
void    frame_put_nfirst (frame_p frm, void * pbyte, int len);
void    frame_put_nfirstp (frame_p * pfrm, void * bytes, int n);

void    frame_put_last (frame_p frm, int byte);
void    frame_put_lastp (frame_p * pfrm, int byte);
#define frame_put_nlast(frm, pbyte, len)  frame_put_nlast_dbg((frm), (pbyte), (len), __FILE__, __LINE__)
void    frame_put_nlast_dbg (frame_p frm, void * pbyte, int len, char * file, int line);
void    frame_put_nlastp (frame_p * pfrm, void * bytes, int n);

void    frame_put (frame_p frm, int pos, int byte);
void    frame_putn (frame_p frm, int pos, void * pbyte, int len);

void    frame_set  (frame_p frm, int pos, int byte);
void    frame_setn (frame_p frm, int pos, void * pbyte, int n);


void    frame_append  (frame_p frm, char * str);
void    frame_appendp (frame_p * pfrm, char * str);
void    frame_prepend (frame_p frm, char * str);
void    frame_prependp (frame_p * pfrm, char * str);
void    frame_insert  (frame_p frm, int pos, char * str);

void    frame_appendf  (frame_p frm, char *fmt, ...);
void    frame_appendfp (frame_p * pfrm, char * fmt, ...);
void    frame_prependf (frame_p frm, char *fmt, ...);
void    frame_insertf  (frame_p frm, int pos, char *fmt, ...);


void    frame_del       (frame_p frm, int pos, int len);
void    frame_del_first (frame_p frm, int len);
void    frame_del_last  (frame_p frm, int len);

void    frame_trunc (frame_p frm, int len);

int     frame_attach (frame_p frm, void * pbuf, int len);


int     frame_replace (frame_p frm, int pos, int len, void * pbyte, int bytelen);
int     frame_search  (frame_p frm, int pos, int len, void * pat, int patlen, int backward);
int     frame_search_replace (frame_p frm, int pos, int len, void * pat, int patlen,
                               void * pbyte, int bytelen, int backward);
int     frame_search_string  (frame_p frm, int pos, int len, void * str, int backward);
int     frame_search_string_replace (frame_p frm, int pos, int len, void * pat,
                                     void * pbyte, int bytelen, int backward);


int     frame_file_load (frame_p frm, char * fname);
int     frame_file_dump (frame_p frm, char * fname);

int     frame_file_read (frame_p frm, FILE * fp, long fpos, long flen);
int     frame_file_write (frame_p frm, FILE * fp, long fpos);

int     frame_filefd_read (frame_p frm, int fd, long fpos, long flen);
int     frame_filefd_write (frame_p frm, int fd, long fpos);

int     frame_tcp_recv (frame_p frm, SOCKET fd, int waitms, int * actnum);
int     frame_tcp_send (frame_p frm, SOCKET fd, int waitms, int * actnum);

/* receive data from socket fd based on non-blocking and zero-copy whenever possible */
#define frame_tcp_nbzc_recv(frm, fd, num, err) frame_tcp_nbzc_recv_dbg(frm, fd, num, err, __FILE__, __LINE__)
int frame_tcp_nbzc_recv_dbg (frame_p frm, SOCKET fd, int * actnum, int * perr, char * file, int line);

int     frame_tcp_nb_recv (frame_p frm, SOCKET fd, int * actnum, int * perr);
int     frame_tcp_nb_send (frame_p frm, SOCKET fd, int * actnum);


int     frame_bin_to_base64 (frame_p srcfrm, frame_p dstfrm);
int     frame_base64_to_bin (frame_p srcfrm, frame_p dstfrm);

int     frame_bin_to_ascii (frame_p srcfrm, frame_p dstfrm);
int     frame_ascii_to_bin (frame_p srcfrm, frame_p dstfrm);

/* special char such as \r \n \t ' \" \\  should be escaped among different system communication */
int     frame_slash_add (void * p, int len, void * escch, int chlen, frame_p frm);
int     frame_slash_strip (void * p, int len, void * escch, int chlen, frame_p frm);

/* escaped char including: \r \n \t \b \f \" \\  */
int     frame_json_escape (void * psrc, int len, frame_p dstfrm);
int     frame_json_unescape (void * psrc, size_t size, frame_p dstfrm);

int     frame_uri_encode (frame_p frm, void * psrc, int size, uint32 * escvec);
int     frame_uri_decode (frame_p frm, void * psrc, int size);

int     frame_html_escape (void * psrc, int len, frame_p dstfrm);

int     frame_bit_set (frame_p frm, int bitpos, int val);
int     frame_bit_get (frame_p frm, int bitpos);

int     frame_bit_shift_left (frame_p frm, int bits);
int     frame_bit_shift_right (frame_p frm, int bits);

void    frame_append_nbytes (frame_p frm, uint8 byte, int n);

frame_p frame_realloc (frame_p frm, int size);

int     frame_add_time (frame_t * frame, time_t * curtm);

#ifdef __cplusplus
}
#endif

#endif

