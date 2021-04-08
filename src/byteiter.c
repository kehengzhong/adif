/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include "btype.h"
#include <stdarg.h>
#include "memory.h"
#include "patmat.h"
#include "byteiter.h"

int iter_init (ByteIter * iter)
{
    if (!iter) return -1;

    iter->text = NULL;
    iter->textlen = 0;
    iter->cur = 0;

    return 0;
}

ByteIter * iter_alloc ()
{
    ByteIter * iter = NULL;

    iter = (ByteIter *) kzalloc(sizeof(*iter));
    if (!iter) return NULL;

    return iter;
}

int iter_free (ByteIter * iter) 
{
    if (!iter) return -1;

    kfree(iter);
    return 0;
}

int iter_set_buffer (ByteIter * iter, uint8 * text, int len)
{
    if (!iter) return -1;

    iter->text = text;
    iter->textlen = len;
    iter->cur = 0;

    return 0;
}


int iter_get_uint64BE (ByteIter * iter, uint64 * pval)
{
    int      i = 0;
    uint64   val = 0;
    uint8  * pbyte = NULL;
 
    if (!iter) return -1;
    if (iter_rest(iter) < 8) return -100;
 
    pbyte = iter_cur(iter);
    for (val=0, i=0; i<8; i++) {
        val <<= 8; val |= pbyte[i];
    }
    if (pval) *pval = val;
     
    iter_forward(iter, 8);
    return 0;
}

int iter_get_uint64LE (ByteIter * iter, uint64 * pval)
{
    int      i = 0;
    uint64   val = 0;
    uint8  * pbyte = NULL;

    if (!iter) return -1;
    if (iter_rest(iter) < 8) return -100;

    pbyte = iter_cur(iter);
    for (val=0, i=7; i>=0; i--) {
        val <<= 8; val |= pbyte[i];
    }
    if (pval) *pval = val;

    iter_forward(iter, 8);
    return 0;
}

int iter_get_uint32BE (ByteIter * iter, uint32 * pval)
{ 
    int      i = 0;
    uint32   val = 0;
    uint8  * pbyte = NULL;
     
    if (!iter) return -1;
    if (iter_rest(iter) < 4) return -100;
     
    pbyte = iter_cur(iter);
    for (val=0, i=0; i<4; i++) {
        val <<= 8; val |= pbyte[i];
    }    
    if (pval) *pval = val;
      
    iter_forward(iter, 4);
    return 0;  
}    
 
int iter_get_uint32LE (ByteIter * iter, uint32 * pval)
{
    int      i = 0;
    uint32   val = 0;
    uint8  * pbyte = NULL;
 
    if (!iter) return -1;
    if (iter_rest(iter) < 4) return -100;
 
    pbyte = iter_cur(iter);
    for (val=0, i=3; i>=0; i--) {
        val <<= 8; val |= pbyte[i];
    }
    if (pval) *pval = val;
 
    iter_forward(iter, 4);
    return 0;
}
 
int iter_get_uint16BE (ByteIter * iter, uint16 * pval)
{
    int      i = 0;
    uint16   val = 0;
    uint8  * pbyte = NULL;
 
    if (!iter) return -1;
    if (iter_rest(iter) < 2) return -100;
 
    pbyte = iter_cur(iter);
    for (val=0, i=0; i<2; i++) {
        val <<= 8; val |= pbyte[i];
    }
    if (pval) *pval = val;
 
    iter_forward(iter, 2);
    return 0;
}
 
int iter_get_uint16LE (ByteIter * iter, uint16 * pval)
{
    int      i = 0;
    uint16   val = 0;
    uint8  * pbyte = NULL;
 
    if (!iter) return -1;
    if (iter_rest(iter) < 2) return -100;
 
    pbyte = iter_cur(iter);
    for (val=0, i=1; i>=0; i--) {
        val <<= 8; val |= pbyte[i];
    }
    if (pval) *pval = val;
 
    iter_forward(iter, 2);
    return 0;
}

int iter_get_uint8 (ByteIter * iter, uint8 * pval)
{
    if (!iter) return -1;
    if (iter_rest(iter) < 1) return -100;
 
    if (pval) *pval = *iter_cur(iter);
    iter_forward(iter, 1);

    return 0;
}

int iter_get_bytes (ByteIter * iter, uint8 * pbuf, int buflen) 
{
    uint8  * pbyte = NULL;
    int      len = 0;

    if (!iter) return -1;

    pbyte = iter_cur(iter);
    len = iter_rest(iter);
    if (len > buflen) len = buflen;
 
    if (pbuf && len > 0)
        memcpy(pbuf, pbyte, len);

    iter_forward(iter, len);
    return len;
}


int iter_set_bytes (ByteIter * iter, uint8 * pbuf, int buflen)
{
    uint8  * pbyte = NULL;
    int      len = 0;

    if (!iter) return -1;

    pbyte = iter_cur(iter);
    len = iter_rest(iter);
    if (len > buflen) len = buflen;
     
    if (pbuf && len > 0) {
        memcpy(pbyte, pbuf, len);
    } else if (pbuf == NULL && len > 0) {
        memset(pbyte, 0, len);
    }
 
    iter_forward(iter, len);  
    return len;
}

int iter_fmtstr (ByteIter * iter, const char * fmt, ...)
{
    uint8  * pbyte = NULL;
    int      len = 0;
    va_list  args;
    char     tmp[4096];

    if (!iter) return -1;

    pbyte = iter_cur(iter);
    len = iter_rest(iter);
     
    memset(tmp, 0, sizeof(tmp));
    va_start(args, fmt);
    vsprintf(tmp, fmt, args);
    va_end(args);

    if (len > (int)strlen(tmp)) len = strlen(tmp);

    memcpy(pbyte, tmp, len);
    return len;
}


int iter_set_uint8 (ByteIter * iter, uint8 byte)
{
    if (!iter) return -1;
    if (iter_rest(iter) < 1) return -100;
 
    *iter_cur(iter) = byte; 
    iter_forward(iter, 1);
 
    return 0;
}

int iter_set_uint16BE (ByteIter * iter, uint16 val)
{
    int      i = 0;
    uint8    byte = 0;
    uint8  * pbyte = NULL;  
         
    if (!iter) return -1;
    if (iter_rest(iter) < 2) return -100;
 
    pbyte = iter_cur(iter);
    for (i=0; i<2; i++) {
        byte = (uint8)((val >> ((1-i)*8)) & 0xFF);
        pbyte[i] = byte;
    }
 
    iter_forward(iter, 2);
    return 0;
}

int iter_set_uint16LE (ByteIter * iter, uint16 val)
{
    int      i = 0;
    uint8    byte = 0;
    uint8  * pbyte = NULL;
 
    if (!iter) return -1;
    if (iter_rest(iter) < 2) return -100;
 
    pbyte = iter_cur(iter);
    for (i=0; i<2; i++) { 
        byte = (uint8)((val >> (i*8)) & 0xFF);
        pbyte[i] = byte;
    }
 
    iter_forward(iter, 2);
    return 0;
}

int iter_set_uint32BE (ByteIter * iter, uint32 val)
{
    int      i = 0;
    uint8    byte = 0;
    uint8  * pbyte = NULL;
 
    if (!iter) return -1;
    if (iter_rest(iter) < 4) return -100;
 
    pbyte = iter_cur(iter);
    for (i=0; i<4; i++) { 
        byte = (uint8)((val >> ((3-i)*8)) & 0xFF);
        pbyte[i] = byte;
    }
 
    iter_forward(iter, 4);
    return 0;
}

int iter_set_uint32LE (ByteIter * iter, uint32 val)
{
    int      i = 0;
    uint8    byte = 0;
    uint8  * pbyte = NULL;
 
    if (!iter) return -1;
    if (iter_rest(iter) < 4) return -100;
 
    pbyte = iter_cur(iter);
    for (i=0; i<4; i++) {
        byte = (uint8)((val >> i*8) & 0xFF);
        pbyte[i] = byte;
    }
 
    iter_forward(iter, 4);
    return 0;
}

int iter_set_uint64BE (ByteIter * iter, uint64 val)
{
    int      i = 0;
    uint8    byte = 0;
    uint8  * pbyte = NULL;
 
    if (!iter) return -1;
    if (iter_rest(iter) < 8) return -100;
 
    pbyte = iter_cur(iter);
    for (i=0; i<8; i++) {
        byte = (uint8)((val >> ((7-i)*8)) & 0xFF);
        pbyte[i] = byte;
    }
 
    iter_forward(iter, 8);
    return 0;
}

int iter_set_uint64LE (ByteIter * iter, uint64 val)
{
    int      i = 0;
    uint8    byte = 0;
    uint8  * pbyte = NULL;
  
    if (!iter) return -1;
    if (iter_rest(iter) < 8) return -100;
     
    pbyte = iter_cur(iter);
    for (i=0; i<8; i++) {
        byte = (uint8)((val >> i*8) & 0xFF);
        pbyte[i] = byte;
    }
 
    iter_forward(iter, 8);
    return 0;
}


/* skip to next, once any char of the given char array is encountered, stop skipping*/
int iter_skipTo (ByteIter * iter, uint8 * chs, int charnum) 
{ 
    int i, moves = 0;
     
    if (!iter) return -1;
 
    while (!iter_istail(iter)) {
        for (i=0; i<charnum; i++) {
            if (iter->text[iter->cur] == chs[i]) return moves;
        }
        iter->cur ++;
        moves++;
    }
 
    return moves;
}
 
/* skip to next, once any char of the given char array is not encountered, stop skipping*/
int iter_skipOver (ByteIter * iter, uint8 * chs, int charnum)
{
    int i, moves = 0;
 
    if (!iter) return -1;
 
    while (!iter_istail(iter)) {
        for (i=0; i<charnum; i++) {
            if (iter->text[iter->cur] != chs[i]) return moves;
        }
        iter->cur++;
        moves++;
    }
 
    return moves;
}
 
int iter_skipTo_bytes (ByteIter * iter, char * pat, int patlen)
{
    uint8  * pbgn = NULL;
    int      len = 0;
    uint8  * pbyte = NULL;

    if (!iter) return 0;

    pbgn = iter_cur(iter);
    len = iter_rest(iter);
    pbyte = (uint8 *)kmp_find_bytes((char *)pbgn, len, pat, patlen, NULL);
    if (pbyte && pbyte > iter_bgn(iter)) {
        len = pbyte - iter_bgn(iter);
        if (len < iter_len(iter))
            iter->cur = len;
    }

    return 0;
} 

