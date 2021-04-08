/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#ifndef _BYTE_ITER_H_
#define _BYTE_ITER_H_

typedef struct byte_iter_ {

    uint8    * text;
    int        textlen;
    int        cur;
} ByteIter;

#define iter_cur(iter)  ((iter)->text + (iter)->cur)
#define iter_end(iter)  ((iter)->text + (iter)->textlen)
#define iter_bgn(iter)  ((iter)->text)
#define iter_len(iter)  ((iter)->textlen)
#define iter_rest(iter)  (((iter)->textlen>(iter)->cur) ? (iter)->textlen-(iter)->cur : 0)
#define iter_offset(iter)  ((iter)->cur)
#define iter_istail(iter)  ((iter)->cur >= (iter)->textlen)

#define iter_forward(iter, n)  (((n)<iter_rest(iter))?((iter)->cur+=(n)):((iter)->cur=(iter)->textlen))
#define iter_back(iter, n)  (((n)<iter_offset(iter))?((iter)->cur-=(n)):((iter)->cur=0))
#define iter_seekto(iter, n)  (((n)<(iter)->textlen)?((iter)->cur=(n)):((iter)->cur=(iter)->textlen))


#ifdef __cplusplus
extern "C" {
#endif

/* initialize the ByteIter instance */
int iter_init (ByteIter * iter);

/* allocate an ByteIter instance */
ByteIter * iter_alloc ();

/* release the memory of an ByteIter instance */
int iter_free (ByteIter * iter);


/* set the ASF document text for the ByteIter */
int iter_set_buffer (ByteIter * iter, uint8 * text, int len);

int iter_get_uint64BE (ByteIter * iter, uint64 * pval);
int iter_get_uint64LE (ByteIter * iter, uint64 * pval);
int iter_get_uint32BE (ByteIter * iter, uint32 * pval);
int iter_get_uint32LE (ByteIter * iter, uint32 * pval);
int iter_get_uint16BE (ByteIter * iter, uint16 * pval);
int iter_get_uint16LE (ByteIter * iter, uint16 * pval);
int iter_get_uint8    (ByteIter * iter, uint8 * pval);
int iter_get_bytes    (ByteIter * iter, uint8 * pbuf, int buflen);

int iter_set_bytes    (ByteIter * iter, uint8 * pbuf, int len);
int iter_fmtstr       (ByteIter * iter, const char * fmt, ...);
int iter_set_uint8    (ByteIter * iter, uint8 byte);
int iter_set_uint16BE (ByteIter * iter, uint16 val);
int iter_set_uint16LE (ByteIter * iter, uint16 val);
int iter_set_uint32BE (ByteIter * iter, uint32 val);
int iter_set_uint32LE (ByteIter * iter, uint32 val);
int iter_set_uint64BE (ByteIter * iter, uint64 val);
int iter_set_uint64LE (ByteIter * iter, uint64 val);


/* skip to next, once any char of the given char array is encountered, stop skipping*/
int iter_skipTo (ByteIter * iter, uint8 * chs, int charnum);

/* skip to next, once any char of the given char array is not encountered, stop skipping*/
int iter_skipOver (ByteIter * iter, uint8 * chs, int charnum);

int iter_skipTo_bytes (ByteIter * iter, char * pat, int patlen);


#ifdef __cplusplus
}
#endif

#endif

