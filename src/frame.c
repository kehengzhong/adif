/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include "btype.h"
#include "memory.h"
#include "frame.h"
#include "strutil.h"
#include "patmat.h"
#include "fileop.h"
#include "tsock.h"

#include <stdarg.h>
#include <assert.h>
#ifdef UNIX
#include <sys/time.h>
#include <poll.h>
#include <ctype.h>
#include <netdb.h>

extern void * memrchr (__const void *__s, int __c, size_t __n);
#endif

#define  DEFAULT_SIZE  128

#if defined(_WIN32) || defined(_WIN64)
#include <io.h>

void * memrchr (const void * s, int c, size_t n)
{
    while(n && (((uint8 *)s)[n - 1] != (uint8)c)) {
        n--;
    }

    return(n ? (uint8 *)s + n - 1 : NULL);
}
#endif

frame_p frame_new (int size)
{
    frame_p frm = NULL;
 
    frm = kzalloc(sizeof(*frm));
    frm->next = NULL;
    if (size <= 0) {
        frm->len = 0;
        frm->size = 0;
        frm->start = 0;
        frm->data = NULL;
    } else {
        frm->start = 0;
        frm->len = 0;
        frm->size = size;
        frm->data = kzalloc(frm->size + 1);
    }
    return frm;
}

void frame_free (frame_p frm)
{
    frame_p  iter = NULL;

    if (!frm) return;

    do {
        iter = frm->next;

        if (frm->data)
            kfree(frm->data);
        kfree(frm);

        frm = iter;
    } while (frm != NULL);
}

void frame_delete (frame_p * pfrm)
{
    if (!pfrm) return;

    frame_free(*pfrm);

    *pfrm = NULL;
}

int frame_size (frame_p frm)
{
    if (!frm) return 0;

    return frm->size;
}

char * frame_string (frame_p frm)
{
    if (!frm || !frm->data) return NULL;

    if (frm->start + frm->len > frm->size)
        return NULL;
 
    frm->data[frm->start + frm->len] = '\0';
 
    return (char *)(frm->data + frm->start);
}

void frame_strip (frame_p frm)
{
    int iter = 0;
 
    if (!frm || frm == 0) return;
 
    for (iter = 0; iter < frm->len; iter++) {
        if (!isspace(frm->data[frm->start + iter]))
            break;
    }

    if (iter > 0) {
        frm->start += iter;
        frm->len -= iter;
    }
 
    if (frm->len > 0) {
        iter = 0;
        while (iter < frm->len &&
               isspace(frm->data[frm->len - 1 - iter]))
            iter++;
        if (iter > 0) {
            frm->len -= iter;
        }
    }
 
    if (frm->len == 0)
        frame_empty(frm);
}

void frame_empty (frame_p frm)
{
    if (!frm) return;

    frm->start = 0;
    frm->len = 0;
    frm->next = NULL;
}

frame_p frame_dup (frame_p frm)
{
    frame_p  dst = NULL;

    if (!frm) return NULL;

    dst = frame_new(frm->len);
    if (!dst) return NULL;

    memcpy(dst->data + dst->start, frm->data + frm->start, frm->len);
    dst->len = frm->len;

    return dst;
}

void frame_grow (frame_p frm, int size)
{
    int       dif = 0;

    if (!frm || size <= 0) return;

    if (size < frm->start &&
        frm->len <= 4 * frm->start &&
        frm->start >= DEFAULT_SIZE) 
    {
        /* just move the data area, no extra allocation */
        if (frm->len > 0)
            memmove(frm->data + frm->start - size, 
                    frm->data + frm->start, frm->len);
        frm->start -= size;
        return;
    }

    dif = (frm->size + size) % DEFAULT_SIZE;
    dif = DEFAULT_SIZE - dif;
    size += dif;

    if (frm->data == NULL) {
        frm->data = kzalloc(frm->size + size + 1);
        frm->start = 0;
        frm->len = 0;
    } else {
        frm->data = krealloc(frm->data, frm->size + size + 1);
        if (!frm->data)
            frm->data = kzalloc(frm->size + size + 1);
    }

    frm->size += size;
}

void frame_grow_to (frame_p frm, int size)
{
    if (!frm || size <= 0) return;

    if (size <= frm->size) return;

    frame_grow(frm, size - frm->size);
}

void frame_grow_head (frame_p frm, int size)
{
    int  dif = 0, rest = 0;

    if (!frm || size <= 0) return;

    if (frm->start > size) return;
    
    rest = frm->size - frm->start - frm->len;
    if (rest > size && 
        frm->len <= rest &&
        rest >= DEFAULT_SIZE)
    {
        /* just move the data area, no extra allocation */
        if (frm->len > 0)
            memmove(frm->data + frm->start + size,
                    frm->data + frm->start,
                    frm->len);
        frm->start += size;
        return;
    }

    dif = (frm->size + size) % DEFAULT_SIZE;
    dif = DEFAULT_SIZE - dif;
    size += dif;
 
    if (frm->data == NULL) {
        frm->data = kzalloc(frm->size + size + 1);
        frm->len = 0;
    } else {
        frm->data = krealloc(frm->data, frm->size + size + 1);
        if (!frm->data)
            frm->data = kzalloc(frm->size + size + 1);
 
        memmove (frm->data + size, frm->data, frm->size);
    }
    frm->size += size;
    frm->start += size;
}

void frame_print (frame_p frm, FILE * fp, int start, int count, int margin)
{
#define CHARS_ON_LINE 16
#define MARGIN_MAX    20
 
    uint8  hexbyte[CHARS_ON_LINE * 5];
    int    ch;
    uint8  marginChar [MARGIN_MAX + 1];
    int    len;
    int    lines, i, j, hexInd, ascInd, iter;
 
    if (!frm || !fp) return;
 
    if ((len = frm->len) == 0  || start >= len)
        return;
 
    if (start < 0) start = 0;
    if (start + count > len) count = len - start;
 
    lines = count / CHARS_ON_LINE;
    if (count % CHARS_ON_LINE) lines++;
 
    memset (marginChar, ' ', MARGIN_MAX + 1);
    if (margin < 0) margin = 0;
    if (margin > MARGIN_MAX) margin = MARGIN_MAX;
    marginChar[margin] = '\0';
 
    for (i = 0; i < lines; i++) {
        hexInd = 0;
        ascInd = 4 + CHARS_ON_LINE * 3;
        memset(hexbyte, ' ', CHARS_ON_LINE * 5);
 
        for (j = 0; j < CHARS_ON_LINE; j++) {
            if ( (iter = j + i * CHARS_ON_LINE) >= count)
                break;
            ch = (uint8)frm->data[frm->start + iter + start];
 
            hexbyte[hexInd++] = toHex(( (ch >> 4) & 15), 1);
            hexbyte[hexInd++] = toHex((ch & 15), 1);
            hexInd++;
 
            hexbyte[ascInd++] = ( ch >= 32 && ch <= 126) ? ch : '.';
        }
        hexbyte[CHARS_ON_LINE * 4 + 4] = '\n';
        hexbyte[CHARS_ON_LINE * 4 + 5] = '\0';
 
#if defined(_WIN32) || defined(_WIN64)
        if (fp == stderr || fp == stdout) {
            char trline[4096];
            sprintf(trline, "%s0x%04X   %s", marginChar, i, hexbyte);
            OutputDebugString(trline);
 
        } else
#endif
            fprintf(fp, "%s0x%04X   %s", marginChar, i, hexbyte);
    }
}

int frame_get (frame_p frm, int pos)
{
    int c;

    if (!frm || frm->len == 0)
        return -1;

    if (pos < 0 || pos >= frm->len)
        return -1;

    if (pos == 0) 
        return frame_get_first(frm);

    if (pos == frm->len - 1) 
        return frame_get_last(frm);

    c = frm->data[frm->start + pos];

    memmove(frm->data + frm->start + pos,
             frm->data + frm->start + pos + 1,
             frm->len - pos - 1);

    frm->len -= 1;
    if (frm->len == 0)
        frame_empty(frm);

    return c;
}

int frame_getn (frame_p frm, int pos, void * bytes, int n)
{
    if (!frm || frm->len == 0) 
        return -1;

    if (!bytes || pos < 0 || n <= 0)
        return -1;

    if (n > frm->len - pos)
        n = frm->len - pos;

    if (pos == 0) 
        return frame_get_nfirst(frm, bytes, n);

    if (pos + n >= frm->len)
        return frame_get_nlast(frm, bytes, n);
    
    memcpy(bytes, &frm->data[frm->start + pos], n);

    memmove (frm->data + frm->start + pos,
             frm->data + frm->start + pos + n,
             frm->len - pos - n);

    frm->len -= n;
    if (frm->len == 0)
        frame_empty(frm);

    return n;
}

int frame_get_first (frame_p frm)
{
    int c;

    if (!frm) return -1;

    if (frm->len == 0)
        return -1;

    c = frm->data[frm->start];
    frm->start += 1;
    frm->len -= 1;

    if (frm->len == 0)
        frame_empty(frm);

    return c;
}

int frame_get_nfirst (frame_p frm, void * bytes, int n)
{
    if (!frm || !bytes || n <= 0) 
        return -1;

    if (frm->len == 0)
        return -1;

    if (n > frm->len) n = frm->len;

    memcpy(bytes, &frm->data[frm->start], n);
    frm->start += n;
    frm->len -= n;

    if (frm->len == 0)
        frame_empty(frm);

    return n;
}

int frame_get_last (frame_p frm)
{
    int c;

    if (!frm) return -1;

    if (frm->len == 0)
        return -1;

    c = frm->data[frm->start + frm->len - 1];
    frm->len -= 1;

    if (frm->len == 0)
        frame_empty(frm);

    return c;
}

int frame_get_nlast (frame_p frm, void * bytes, int n)
{
    if (!frm || !bytes || n <= 0)
        return -1;

    if (frm->len == 0)
        return -1;

    if (n > frm->len) n = frm->len;

    memcpy(bytes, &frm->data[frm->start + frm->len - n], n);
    frm->len -= n;

    if (frm->len == 0)
        frame_empty(frm);

    return n;
}

int frame_read (frame_p frm, int pos)
{
    if (frm->len == 0 || pos < 0 || pos >= frm->len)
        return -1;

    return frm->data[frm->start + pos];
}

int frame_readn (frame_p frm, int pos, void * bytes, int n)
{
    if (frm->len == 0 || !bytes || n <= 0 || pos < 0 || pos >= frm->len)
        return -1;

    if (pos + n > frm->len) n = frm->len - pos;

    memcpy(bytes, &frm->data[frm->start + pos], n);
    
    return n;
}

void frame_put_first (frame_p frm, int byte)
{
    if (!frm) return;

    if (frm->data == NULL) {
        frm->size = DEFAULT_SIZE;
        frm->data = kzalloc(frm->size + 1);
        frm->start = frm->size/2;
        frm->len = 0;
    }

    if (frm->start == 0) {
        frame_grow_head(frm, DEFAULT_SIZE);
    }

    frm->start -= 1;
    frm->data[frm->start] = byte;
    frm->len += 1;
}

void frame_put_nfirst (frame_p frm, void * bytes, int n)
{
    if (!frm || !bytes || n <= 0) return;

    if (frm->data == NULL) {
        frm->size = n + DEFAULT_SIZE - (n % DEFAULT_SIZE);
        frm->data = kzalloc(frm->size + 1);
        frm->start = n;
        frm->len = 0;
    }

    if (frm->start < n) {
        frame_grow_head(frm, n);
    }

    memcpy(frm->data + frm->start - n, bytes, n);
    frm->start -= n;
    frm->len += n;
}

void frame_put_last (frame_p frm, int byte)
{
    if (!frm) return;

    if (frm->data == NULL) {
        frm->size = DEFAULT_SIZE;
        frm->data = kzalloc(frm->size + 1);
        frm->start = frm->size/2;
        frm->len = 0;
    }

    if (frame_rest(frm) == 0) {
        frame_grow(frm, 1);
    }

    frm->data[frm->start + frm->len] = byte;
    frm->len += 1;
}

void frame_put_nlast (frame_p frm, void * bytes, int n)
{
    if (!frm || !bytes || n <= 0) return;

    if (frm->data == NULL) {
        frm->size = n + DEFAULT_SIZE - (n % DEFAULT_SIZE);
        frm->data = kzalloc(frm->size + 1);
        frm->start = 0;
        frm->len = 0;
    }

    if (frame_rest(frm) < n) {
        frame_grow(frm, n - frame_rest(frm));
    }

    memcpy(frm->data + frm->start + frm->len, bytes, n);
    frm->len += n;
}

void frame_put (frame_p frm, int pos, int byte)
{
    if (!frm || pos < 0) return;

    if (pos == 0) {
        frame_put_first(frm, byte);
        return;
    }

    if (pos >= frm->len) {
        frame_put_last(frm, byte);
        return;
    }

    if (frame_rest(frm) < 1) {
         frame_grow(frm, 1);
    }

    memmove(frm->data + frm->start + pos + 1,
            frm->data + frm->start + pos,
            frm->len - pos);

    frm->data[frm->start + pos] = byte;
    frm->len += 1;
}

void frame_putn (frame_p frm, int pos, void * bytes, int n)
{
    if (!frm || !bytes || pos < 0 || n <= 0) return;

    if (pos == 0) {
        frame_put_nfirst(frm, bytes, n);
        return;
    }

    if (pos >= frm->len) {
        frame_put_nlast(frm, bytes, n);
        return;
    }

    if (frame_rest(frm) < n) {
         frame_grow(frm, n - frame_rest(frm));
    }

    memmove(frm->data + frm->start + pos + n, 
            frm->data + frm->start + pos,
            frm->len - pos);

    memcpy(frm->data + frm->start + pos, bytes, n);
    frm->len += n;
}

void frame_set (frame_p frm, int pos, int byte)
{
    if (!frm || frm->len <= 0 || pos < 0 || pos >= frm->len)
        return;

    frm->data[frm->start + pos] = byte;
}

void frame_setn (frame_p frm, int pos, void * bytes, int n)
{
    if (!frm || frm->len <= 0 || pos < 0 || pos >= frm->len)
        return;

    if (!bytes || n <= 0)
        return;

    if (pos + n > frm->len) n = frm->len - pos;

    memcpy(frm->data + frm->start + pos, bytes, n);
}

void frame_append (frame_p frm, char * str)
{
    if (!frm || !str) return;

    frame_put_nlast(frm, str, strlen(str));
}

void frame_prepend (frame_p frm, char * str)
{
    if (!frm || !str) return;

    frame_put_nfirst(frm, str, strlen(str));
}

void frame_insert (frame_p frm, int pos, char * str)
{
    if (!frm || !str) return;

    frame_putn(frm, pos, str, strlen(str));
}

void frame_appendf (frame_p frm, char * fmt, ...)
{
    va_list args;
    int     avail = 0;
    int     written = 0;

    for (;;) {
        avail = frame_rest(frm);
        va_start(args, fmt);
        written = vsnprintf(frame_end(frm), avail, fmt, args);
        va_end(args);

        if (avail <= written) {
            frame_grow(frm, written - avail + 1);
            continue;
        }

        frm->len += written;
        return;
    }
}

void frame_prependf (frame_p frm, char * fmt, ...)
{
    va_list args;
    char    tmp[4096];
    int     len = 0;
    char  * p = NULL;
    int     avail = 0;
 
    va_start(args, fmt);
    len = vsnprintf(tmp, sizeof(tmp)-1, fmt, args);
    va_end(args);

    if (len > sizeof(tmp) - 1) {
        avail = len + 1;
        p = kalloc(avail);

        va_start(args, fmt);
        len = vsnprintf(p, avail, fmt, args);
        va_end(args);

        frame_put_nfirst(frm, p, len);

        kfree(p);

    } else {
        frame_put_nfirst(frm, tmp, len);
    }
}

void frame_insertf (frame_p frm, int pos, char * fmt, ...)
{
    va_list args;
    char    tmp[4096];
    int     len = 0;
    char  * p = NULL;
    int     avail = 0;
 
    va_start(args, fmt);
    len = vsnprintf(tmp, sizeof(tmp)-1, fmt, args);
    va_end(args);
 
    if (len > sizeof(tmp) - 1) {
        avail = len + 1;
        p = kalloc(avail);
         
        va_start(args, fmt);
        len = vsnprintf(p, avail, fmt, args);
        va_end(args);
 
        frame_putn(frm, pos, p, len);
 
        kfree(p);
     
    } else { 
        frame_putn(frm, pos, tmp, len);
    }
}

void frame_del_first (frame_p frm, int n)
{
    if (!frm || n <= 0) return;

    if (n >= frm->len) {
        frame_empty(frm);
        return;
    }

    frm->start += n;
    frm->len -= n;
}

void frame_del_last (frame_p frm, int n)
{
    if (!frm || n <= 0) return;

    if (n >= frm->len) {
        frame_empty(frm);
        return;
    }

    frm->len -= n;
}

void frame_del (frame_p frm, int pos, int n)
{
    if (!frm || frm->len <= 0 || n <= 0 || pos < 0)
        return;

    if (pos >= frm->len) return;

    if (pos == 0) {
        frame_del_first(frm, n);
        return;
    }

    if (pos + n >= frm->len) {
        frame_del_last(frm, frm->len - pos);
        return;
    }

    memmove(frm->data + frm->start + pos,
             frm->data + frm->start + pos + n,
             frm->len - pos - n);

    frm->len -= n;

    if (frm->len == 0)
        frame_empty(frm);
}

void frame_trunc (frame_p frm, int len)
{
    if (!frm || len <= 0) return;

    if (len >= frm->len) return;

    if (len <= 0) {
        frame_empty(frm);
        return;
    }
    
    frm->len = len;
}

int frame_attach (frame_p frm, void * pbuf, int len)
{
    if (!frm || !pbuf || len <= 0)
        return -1;

    if (frm->data)
        kfree(frm->data);

    frm->data = pbuf;
    frm->start = 0;
    frm->size = len;
    frm->len = len;

    return len;
}

int frame_replace (frame_p frm, int pos, int len, void * pbyte, int bytelen)
{
    if (!frm || frm->len <= 0)
        return -1;
 
    if (len == 0 && bytelen == 0)
        return 0;
 
    if (pos < 0) pos = 0;
    if (pos > frm->len) pos = frm->len;
 
    if (pos + len > frm->len)
        len = frm->len - pos;
 
    if (len == 0) {
        frame_putn(frm, pos, pbyte, bytelen);
        return 0;
    } else if (bytelen == 0) {
        frame_del(frm, pos, len);
        return 0;
    }
 
    if (len < bytelen) {
        if (frame_rest(frm) < bytelen - len) {
            frame_grow(frm, bytelen - len - frame_rest(frm));
        }
 
        if (pos + len < frm->len)
            memmove(frm->data + frm->start + pos + bytelen,
                    frm->data + frm->start + pos + len,
                    frm->len - pos - len);
 
        memcpy(frm->data + frm->start + pos, pbyte, bytelen);
        frm->len += bytelen - len;
 
    } else if (len == bytelen) {
        memcpy(frm->data + frm->start + pos, pbyte, bytelen);
 
    } else { /* len > bytelen */
        memcpy(frm->data + frm->start + pos, pbyte, bytelen);
 
        if (pos + len < frm->len)
            memmove(frm->data + frm->start + pos + bytelen,
                    frm->data + frm->start + pos + len,
                    frm->len - pos - len);
        frm->len -= len - bytelen;
    }
 
    return 0;
}
 
int frame_search (frame_p frm, int pos, int len, void * pattern, int patlen, int backward)
{
    uint8 * pat = (uint8 *)pattern;
    uint8 * p = NULL;

    if (!frm || frm->len <= 0)
        return -1;

    if (pos < 0 || pos >= frm->len)
        return -1;

    if (len < 0) len = frm->len - pos;
    if (len > frm->len - pos) len = frm->len - pos;

    if (!pat || patlen <= 0)
        return -2;

    if (pos + patlen > len) 
        return -3;

    if (backward) {
        if (patlen == 1)
            p = memrchr(frm->data + frm->start + pos, pat[0], len);
        else
            p = kmp_rfind_bytes(frm->data + frm->start + pos, len, pat, patlen, NULL);
    } else {
        if (patlen == 1)
            p = memchr(frm->data + frm->start + pos, pat[0], len);
        else
            p = kmp_find_bytes(frm->data + frm->start + pos, len, pat, patlen, NULL);
    }

    if (!p) return -100;

    return p - (frm->data + frm->start);
}

int frame_search_replace (frame_p frm, int pos, int len, void * pattern, int patlen,
                              void * pbyte, int bytelen, int backward)
{
    uint8 * pat = (uint8 *)pattern;
    int  findpos = 0;
    int  schrep = 0;

    if (!frm || frm->len <= 0 || pos < 0 || pos >= frm->len)
        return -1;

    if (len < 0) len = frm->len - pos;
    if (len > frm->len - pos) len = frm->len - pos;

    if (!pat || patlen <= 0 || bytelen < 0)
        return -2;

    if (pos + patlen > len) 
        return -2;

    if (backward == 0) {
        for (findpos = pos; findpos + patlen <= len; ) {
            findpos = frame_search(frm, findpos, len, pat, patlen, backward);
            if (findpos < 0) return -100;

            frame_replace(frm, findpos, patlen, pbyte, bytelen);
            schrep++;

            findpos += bytelen;
            len -= (findpos - pos) + patlen;
        }
    } else {
        for (; pos + patlen <= len; ) {
            findpos = frame_search(frm, pos, len, pat, patlen, backward);
            if (findpos < 0) return -100;

            frame_replace(frm, findpos, patlen, pbyte, bytelen);
            schrep++;

            len = findpos - pos;
        }
    }

    return schrep;
}

int frame_search_string (frame_p frm, int pos, int len, void * pattern, int backward)
{
    uint8  * pat = (uint8 *)pattern;
    uint8  * p = NULL;
    int      patlen = 0;

    if (!frm || frm->len <= 0)
        return -1;

    if (pos < 0 || pos >= frm->len)
        return -1;

    if (len < 0) len = frm->len - pos;
    if (len > frm->len - pos) len = frm->len - pos;

    if (!pat || (patlen = str_len(pat)) <= 0)
        return -2;

    if (pos + patlen > len) 
        return -3;

    if (backward) {
        p = kmp_rfind_string(frm->data + frm->start + pos, len, pat, patlen, NULL);
    } else {
        p = kmp_find_string(frm->data + frm->start + pos, len, pat, patlen, NULL);
    }

    if (!p) return -100;

    return p - (frm->data + frm->start);

}
 
int frame_search_string_replace (frame_p frm, int pos, int len, void * pattern,
                                  void * pbyte, int bytelen, int backward)
{
    uint8  * pat = (uint8 *)pattern;
    int  findpos = 0;
    int  schrep = 0;
    int  patlen = 0;
 
    if (!frm || frm->len <= 0 || pos < 0 || pos >= frm->len)
        return -1;
 
    if (len < 0) len = frm->len - pos;
    if (len > frm->len - pos) len = frm->len - pos;
 
    if (!pat || (patlen = str_len(pat)) <= 0 || bytelen < 0)
        return -2;
 
    if (pos + patlen > len)
        return -2;
 
    if (backward == 0) {
        for (findpos = pos; findpos + patlen <= len; ) {
            findpos = frame_search_string(frm, findpos, len, pat, backward);
            if (findpos < 0) return -100;
 
            frame_replace(frm, findpos, patlen, pbyte, bytelen);
            schrep++;
 
            findpos += bytelen;
            len -= (findpos - pos) + patlen;
        }
    } else {
        for (; pos + patlen <= len; ) {
            findpos = frame_search_string(frm, pos, len, pat, backward);
            if (findpos < 0) return -100;
 
            frame_replace(frm, findpos, patlen, pbyte, bytelen);
            schrep++;
 
            len = findpos - pos;
        }
    }
 
    return schrep;
}

int frame_file_load (frame_p frm, char * fname)
{
    struct stat fs;
    int         fsize = 0;
    long        len = 0;
    FILE      * fp = NULL;

    if (!frm || !fname) return -1;

    if (file_stat(fname, &fs) < 0) 
        return -2;

    if (fs.st_size > 64*1024*1024)
        return -100;

    fsize = (int)fs.st_size;

    if (frame_rest(frm) < fsize)
        frame_grow(frm, fsize);

    fp = fopen(fname, "rb+");
    if (fp) {
        len = file_read(fp, frm->data + frm->start, fsize);
        if (len > 0) frm->len += len;
        fclose(fp);
    }

    return (int)len;
}

int frame_file_dump (frame_p frm, char * fname)
{
    FILE      * fp = NULL;
    long        len = -1;

    if (!frm || frm->len <= 0 || !fname) 
        return -1;

    fp = fopen(fname, "wb");
    if (fp) {
        len = file_write(fp, frm->data + frm->start, frm->len);
        fclose(fp);
    }

    return (int)len;
}

int frame_file_read (frame_p frm, FILE * fp, long fpos, long flen)
{
    int    readlen = 0;
    long   len = 0;
 
    if (!frm || !fp) return -1;
 
    if (flen > 64*1024*1024)
        return -100;
 
    readlen = (int)flen;
 
    if (frame_rest(frm) < readlen)
        frame_grow(frm, readlen);
 
    if (fpos >= 0) {
        if (file_seek(fp, fpos, SEEK_SET) < 0)
            return -101;
    }

    len = file_read(fp, frm->data + frm->start + frm->len, readlen);
    if (len > 0) frm->len += len;
 
    return (int)len;
}

int frame_file_write (frame_p frm, FILE * fp, long fpos)
{
    long        len = -1;

    if (!frm || frm->len <= 0 || !fp) 
        return -1;

    if (fpos >= 0) {
        if (file_seek(fp, fpos, SEEK_SET) < 0)
            return -101;
    }

    len = file_write(fp, frm->data + frm->start, frm->len);

    return (int)len;
}

int frame_filefd_read (frame_p frm, int fd, long fpos, long flen)
{
    int    readlen = 0;
    long   len = 0;
         
    if (!frm || fd < 0) return -1; 
                 
    if (flen > 64*1024*1024)
        return -100;
  
    readlen = (int)flen; 
  
    if (frame_rest(frm) < readlen)
        frame_grow(frm, readlen);
         
    if (fpos >= 0) {
        if (lseek(fd, fpos, SEEK_SET) < 0)
            return -101;
    }
     
    len = filefd_read(fd, frm->data + frm->start + frm->len, readlen);
    if (len > 0) frm->len += len;
 
    return (int)len;
} 
 
int frame_filefd_write (frame_p frm, int fd, long fpos)
{
    long        len = -1;
     
    if (!frm || frm->len <= 0 || fd < 0)
        return -1;
 
    if (fpos >= 0) {
        if (lseek(fd, fpos, SEEK_SET) < 0)
            return -101;
    }
 
    len = filefd_write(fd, frm->data + frm->start, frm->len);
 
    return (int)len;
}

int frame_tcp_recv (frame_p frm, SOCKET fd, int waitms, int * actnum)
{
    uint8   buf[524288];
    int     size = sizeof(buf);
    int     ret = 0, readLen = 0;
    int     unread = 0;
    int     errcode;
    int     restms = 0;
    struct  timeval tick0, tick1;
#ifdef UNIX
#ifdef _SOLARIS_
    int     errcase = 0, eret = 0;
    int     errlen = sizeof(errcase);
#endif
#endif
 
    if (actnum) *actnum = 0;
 
    if (fd == INVALID_SOCKET)
        return -70;

    if (!frm) return 0;
 
    if (waitms > 0) {
        gettimeofday(&tick0, NULL);
        restms = waitms;
    } else {
        restms = 0;
    }
 
    for (readLen = 0; ; ) {
        if (waitms > 0) {
            gettimeofday(&tick1, NULL);
            restms -= tv_diff_us(&tick0, &tick1)/1000;
            if (restms <= 0) return 0;
            tick0 = tick1;
        }
 
        unread = sock_unread_data(fd);
#ifdef UNIX
        errno = 0;
#endif
        if (unread > 0) {
            if (frame_rest(frm) < unread)
                frame_grow(frm, unread - frame_rest(frm));

            while (unread > 0) {
                ret = recv(fd, frame_end(frm), unread, MSG_NOSIGNAL);
                if (ret > 0) {
                    readLen += ret;
                    frame_len_add(frm, ret);
                    unread -= ret;
                    continue;
                }
                break;
            }
            if (ret > 0 && unread == 0)
                break;

        } else {
            ret = recv(fd, buf, size, MSG_NOSIGNAL);
            if (ret > 0) {
                readLen += ret;
                if (frm) frame_put_nlast(frm, buf, ret);
                continue;
            }
        }
 
        if (ret == 0) {
            if (actnum) *actnum = readLen;
            return -20; /* connection closed by other end */
 
        } else if (ret == -1) {
#if defined(_WIN32) || defined(_WIN64)
            errcode = WSAGetLastError();
            if (errcode == WSAEINTR || errcode == WSAEWOULDBLOCK) {
                continue;
            }
#endif
#ifdef UNIX
            errcode = errno;
            if (errcode == EINTR || errcode == EAGAIN || errcode == EWOULDBLOCK) {
                continue;
            }
    #ifdef _SOLARIS_
            errlen = sizeof(errcase);
            errcase = 0;
            ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &errcase, &errlen);
            if (ret < 0 || errcase != 0) { ret = -31; goto error; }
            if(errcode == 0) continue;
    #endif
#endif
            ret = -30;
            goto error;
        }
    }
 
    if (actnum) *actnum = readLen;
    return readLen;
 
error:
    if (actnum) *actnum = readLen;
    return ret;
}

int frame_tcp_send (frame_p frm, SOCKET fd, int waitms, int * actnum)
{
    int     ret, sendLen = 0;
    int     errcode;
    long    restms = 0;
    struct  timeval t0, t1;
#ifdef UNIX
#ifdef _SOLARIS_
    int     errcase = 0, eret = 0;
    int     errlen = sizeof(errcase);
#endif
#endif
 
    if (actnum) *actnum = 0;
 
    if (fd == INVALID_SOCKET) return -70;

    if (!frm || frm->len <= 0)
        return 0;
 
    if (waitms > 0) {
        gettimeofday(&t0, NULL);
        restms = waitms;
    } else {
        restms = 0;
    }
 
    for (sendLen = 0; sendLen < frm->len; ) {
        if (waitms > 0) {
            gettimeofday(&t1, NULL);
            restms -= tv_diff_us(&t0, &t1)/1000;
            if (restms <= 0) {
                if (actnum) *actnum = sendLen;
                return 0;
            }
            t0 = t1;
        }

#ifdef UNIX
        errno = 0;
#endif
        ret = send (fd, (uint8 *)frameP(frm) + sendLen, frm->len - sendLen, MSG_NOSIGNAL);
        if (ret == -1) {
#if defined(_WIN32) || defined(_WIN64)
            errcode = WSAGetLastError();
            if (errcode == WSAEINTR || errcode == WSAEWOULDBLOCK) {
                continue;
            }
#endif
#ifdef UNIX
            errcode = errno;
            if (errcode == EINTR || errcode == EAGAIN || errcode == EWOULDBLOCK) {
                continue;
            }
    #ifdef _SOLARIS_
            errlen = sizeof(errcase);
            errcase = 0;
            ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &errcase, &errlen);
            if (ret < 0 || errcase != 0) { ret = -31; goto error; }
            if(errcode == 0) continue;
    #endif
#endif
            ret = -30;
            goto error;
        } else {
            sendLen += ret;
        }
    }
 
    if (actnum) *actnum = sendLen;
    return sendLen;

error:
    if (actnum) *actnum = sendLen;
    return ret;
}

/* receive data from socket fd based on non-blocking and zero-copy whenever possible */

int frame_tcp_nbzc_recv (frame_p frm, SOCKET fd, int * actnum, int * perr)
{
    uint8   buf[524288];
    int     size = sizeof(buf);
    int     ret = 0, readLen = 0;
    int     unread = 0;
    int     errtimes = 0;
    int     errcode;
#ifdef UNIX
#ifdef _SOLARIS_
    int     errcase = 0, eret = 0;
    int     errlen = sizeof(errcase);
#endif
#endif
 
    if (actnum) *actnum = 0;

    if (fd == INVALID_SOCKET)
        return -1;
 
    if (!frm) return -2;

    for (readLen = 0; ; ) {
        unread = sock_unread_data(fd);
#ifdef UNIX
        errno = 0;
#endif

        if (unread > 0) {
            if (frame_rest(frm) < unread)
                frame_grow(frm, unread - frame_rest(frm));

            while (unread > 0) {
                ret = recv(fd, frame_end(frm), unread, MSG_NOSIGNAL);
                if (ret > 0) {
                    readLen += ret;
                    frame_len_add(frm, ret);
                    unread -= ret;
                    continue;
                }
                break;
            }
            if (ret > 0 && unread == 0)
                break;

        } else {
            ret = recv(fd, buf, size, MSG_NOSIGNAL);
            if (ret > 0) {
                readLen += ret;
                frame_put_nlast(frm, buf, ret);
                continue;
            }
        }
 
#ifdef UNIX
        if (perr) *perr = errno;
#endif
#if defined(_WIN32) || defined(_WIN64)
        if (perr) *perr = errcode = WSAGetLastError();
#endif

        if (ret == 0) {
            if (actnum) *actnum = readLen;
            return -20;

        } else if (ret == SOCKET_ERROR) {
#if defined(_WIN32) || defined(_WIN64)
            errcode = WSAGetLastError();
            if (errcode == WSAEINTR) {
                continue;
            }
            if (errcode == WSAEWOULDBLOCK) {
                if (++errtimes >= 1) break;
                continue;
            }
#endif
#ifdef UNIX
            errcode = errno;
            if (errcode == EINTR) {
                continue;
            }
            if (errcode == EAGAIN || errcode == EWOULDBLOCK) {
                if (++errtimes >= 1) break;
                continue;
            }
    #ifdef _SOLARIS_
            errlen = sizeof(errcase);
            errcase = 0;
            eret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &errcase, &errlen);
            if (eret < 0 || errcase != 0) goto error;
            if (errcode == 0) break;
    #endif
#endif
            ret = -30;
            goto error;
        }
    }
 
    if (actnum) *actnum = readLen;
    return readLen;
 
error: /* connection exception */
    if (actnum) *actnum = readLen;
    return ret;
}
 
int frame_tcp_nb_recv (frame_p frm, SOCKET fd, int * actnum, int * perr)
{
    uint8   buf[524288];
    int     size = sizeof(buf);
    int     ret = 0, readLen = 0;
    int     errcode;
#ifdef UNIX
#ifdef _SOLARIS_
    int     errcase = 0, eret = 0;
    int     errlen = sizeof(errcase);
#endif
#endif
 
    if (actnum) *actnum = 0;
    if (perr) *perr = 0;

    if (fd == INVALID_SOCKET)
        return -1;
 
    for (readLen = 0; ; ) {
#ifdef UNIX
        errno = 0;
#endif
        ret = recv(fd, buf, size, MSG_NOSIGNAL);
        if (ret > 0) {
            readLen += ret;
            if (frm) frame_put_nlast(frm, buf, ret);
            continue;
        }
 
#ifdef UNIX
        if (perr) *perr = errno;
#endif
#if defined(_WIN32) || defined(_WIN64)
        if (perr) *perr = errcode = WSAGetLastError();
#endif

        if (ret == 0) {
            if (actnum) *actnum = readLen;
            return -20;

        } else if (ret == SOCKET_ERROR) {
#if defined(_WIN32) || defined(_WIN64)
            errcode = WSAGetLastError();
            if (errcode == WSAEINTR) {
                continue;
            }
            if (errcode == WSAEWOULDBLOCK) {
                break;
            }
#endif
#ifdef UNIX
            errcode = errno;
            if (errcode == EINTR) {
                continue;
            }
            if (errcode == EAGAIN || errcode == EWOULDBLOCK) {
                break;
            }
    #ifdef _SOLARIS_
            errlen = sizeof(errcase);
            errcase = 0;
            eret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &errcase, &errlen);
            if (eret < 0 || errcase != 0) goto error;
            if (errcode == 0) break;
    #endif
#endif
            ret = -30;
            goto error;
        }
    }
 
    if (actnum) *actnum = readLen;
    return readLen;
 
error: /* connection exception */
    if (actnum) *actnum = readLen;
    return ret;
}
 
int frame_tcp_nb_send (frame_p frm, SOCKET fd, int * actnum)
{
    int     sendLen = 0;
    int     ret, errcode;
    int     errtimes = 0;
#ifdef UNIX
#ifdef _SOLARIS_
    int     errcase = 0, eret = 0;
    int     errlen = sizeof(errcase);
#endif
#endif
 
    if (actnum) *actnum = 0;
 
    if (fd == INVALID_SOCKET)
        return -1;

    if (!frm || frm->len <= 0)
        return 0;

    for (sendLen = 0, errtimes = 0; sendLen < frm->len; ) {
#ifdef UNIX
        errno = 0;
#endif
        ret = send(fd, (uint8 *)frameP(frm) + sendLen, frm->len - sendLen, MSG_NOSIGNAL);
        if (ret == -1) {
#if defined(_WIN32) || defined(_WIN64)
            errcode = WSAGetLastError();
            if (errcode == WSAEINTR || errcode == WSAEWOULDBLOCK) {
                if (++errtimes >= 1) break;
                continue;
            }
#endif
#ifdef UNIX
            errcode = errno;
            if (errcode == EINTR || errcode == EAGAIN || errcode == EWOULDBLOCK) {
                if (++errtimes >= 1) break;
                continue;
            }
    #ifdef _SOLARIS_
            errlen = sizeof(errcase);
            errcase = 0;
            ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &errcase, &errlen);
            if (ret < 0 || errcase != 0) { ret = -31; goto error; }
            if(errcode == 0) {
                if (++errtimes >= 3) break;
                continue;
            }
    #endif
#endif
            ret = -30;
            goto error;
        } else {
            sendLen += ret;
        }
    }
 
    if (actnum) *actnum = sendLen;
    return sendLen;
error:
    if (actnum) *actnum = sendLen;
    return ret;
}

//#define BASE64CRLF 1
int frame_bin_to_base64 (frame_p frm, frame_p dst)
{
    static const uint8 base64[65] =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int      triplets, lines, orig_len;
    int      from, to;
    int      left_on_line;
    uint8  * pbin = NULL;
    uint8  * pdst = NULL;
 
    if (!frm) return -1;
    if (!dst) dst = frm;
 
    if (frm->len == 0) {
        /* Always terminate with CR LF */
#ifdef BASE64CRLF
        frame_put_nlast(dst, "\r\n", 2);
#endif
        return 0;
    }
 
    /* The lines must be 76 characters each (or less), and each
     * triplet will expand to 4 characters, so we can fit 19
     * triplets on one line.  We need a CR LF after each line,
     * which will add 2 octets per 19 triplets (rounded up). */
    triplets = (frm->len + 2) / 3;   /* round up */
    lines = (triplets + 18) / 19;
 
#ifdef BASE64CRLF
    frame_grow_to(dst, triplets * 4 + lines * 2);
#else
    frame_grow_to(dst, triplets * 4 + 1);
#endif
 
    orig_len = frm->len;
    pbin = frameP(frm);
    pdst = frameP(dst);
 
#ifdef BASE64CRLF
    dst->len = triplets * 4 + lines * 2;
#else
    dst->len = triplets * 4;
#endif
    pdst[dst->len] = '\0';
 
    /* This function works back-to-front, so that encoded data will
     * not overwrite source data.
     * from points to the start of the last triplet (which may be
     * an odd-sized one), and to points to the start of where the
     * last quad should go.  */
    from = (triplets - 1) * 3;
#ifdef BASE64CRLF
    to = (triplets - 1) * 4 + (lines - 1) * 2;
    /* First write the CR LF after the last quad */
    pdst[to + 5] = 10;   /* LF */
    pdst[to + 4] = 13;   /* CR */
#else
    to = (triplets - 1) * 4;
#endif
 
    left_on_line = triplets - ((lines - 1) * 19);
 
    /* base64 encoding is in 3-octet units.  To handle leftover
     * octets, conceptually we have to zero-pad up to the next
     * 6-bit unit, and pad with '=' characters for missing 6-bit units.
     * We do it by first completing the first triplet with zero-octets,
     * and after the loop replacing some of the result characters with '='
     * characters. There is enough room for this, because even with a 1 or 2
     * octet source string, space for four octets of output will be reserved. */
    switch (orig_len % 3) {
    case 0:
        break;
    case 1:
        pbin[orig_len] = 0;
        pbin[orig_len + 1] = 0;
        break;
    case 2:
        pbin[orig_len] = 0;
        break;
    }
 
    /* Now we only have perfect triplets. */
    while (from >= 0) {
        int  whole_triplet;
 
        /* Add a newline, if necessary */
        if (left_on_line == 0) {
#ifdef BASE64CRLF
            to -= 2;
            pdst[to + 5] = 10;  /* LF */
            pdst[to + 4] = 13;  /* CR */
#endif
            left_on_line = 19;
        }
 
        whole_triplet = (pbin[from] << 16) |
                        (pbin[from + 1] << 8) |
                        pbin[from + 2];
        pdst[to + 3] = base64[whole_triplet % 64];
        pdst[to + 2] = base64[(whole_triplet >> 6) % 64];
        pdst[to + 1] = base64[(whole_triplet >> 12) % 64];
        pdst[to] = base64[(whole_triplet >> 18) % 64];
 
        to -= 4;
        from -= 3;
        left_on_line--;
    }
 
    assert(left_on_line == 0);
    assert(from == -3);
    assert(to == -4);
 
    /* Insert padding characters in the last quad.  Remember that
     * there is a CR LF between the last quad and the end of the
     * string. */
    switch (orig_len % 3) {
    case 0:
        break;
    case 1:
#ifdef BASE64CRLF
        assert(pdst[dst->len - 3] == 'A');
        assert(pdst[dst->len - 4] == 'A');
        pdst[dst->len - 3] = '=';
        pdst[dst->len - 4] = '=';
#else
        assert(pdst[dst->len - 1] == 'A');
        assert(pdst[dst->len - 2] == 'A');
        pdst[dst->len - 1] = '=';
        pdst[dst->len - 2] = '=';
#endif
        break;
    case 2:
#ifdef BASE64CRLF
        assert(pdst[dst->len - 3] == 'A');
        pdst[dst->len - 3] = '=';
#else
        assert(pdst[dst->len - 1] == 'A');
        pdst[dst->len - 1] = '=';
#endif
        break;
    }
 
    return 0;
}
 
int frame_base64_to_bin (frame_p frm, frame_p dst)
{
    int      triplet, pos, len, to;
    int      quadpos = 0;
    uint8  * data = NULL;
    uint8  * pdst = NULL;
 
    if (!frm) return -1;
 
    if (frm->len == 0)
        return 0;
 
    if (!dst) dst = frm;
    
    len = (frm->len + 3)/4 * 3;
    if (dst->len < len) {
        frame_grow(dst, len - dst->len);
    }

    data = frameP(frm);
    pdst = frameP(dst);
 
    to = 0;
    triplet = 0;
    quadpos = 0;
    for (pos = 0; pos < frm->len; pos++) {
        int c = data[pos];
        int sixbits;
 
        if (c >= 'A' && c <= 'Z') {
            sixbits = c - 'A';
        } else if (c >= 'a' && c <= 'z') {
            sixbits = 26 + c - 'a';
        } else if (c >= '0' && c <= '9') {
            sixbits = 52 + c - '0';
        } else if (c == '+') {
            sixbits = 62;
        } else if (c == '/') {
            sixbits = 63;
        } else if (c == '=') {
            /* These can only occur at the end of encoded text.  RFC 2045
             * says we can assume it really is the end. */
            break;
        } else if (isspace(c)) {
            /* skip whitespace */
            continue;
        } else {
            continue;
        }
 
        triplet = (triplet << 6) | sixbits;
        quadpos++;
 
        if (quadpos == 4) {
            pdst[to++] = (triplet >> 16) & 0xff;
            pdst[to++] = (triplet >> 8) & 0xff;
            pdst[to++] = triplet & 0xff;
            quadpos = 0;
        }
    }
 
    /* Deal with leftover octets */
    switch (quadpos) {
    case 0:
        break;
    case 3:  /* triplet has 18 bits, we want the first 16 */
        pdst[to++] = (triplet >> 10) & 0xff;
        pdst[to++] = (triplet >> 2) & 0xff;
        break;
    case 2:  /* triplet has 12 bits, we want the first 8 */
        pdst[to++] = (triplet >> 4) & 0xff;
        break;
    case 1:
        break;
    }
 
    dst->len = to;
    pdst[to] = '\0';
 
    return 0;
}

int frame_bin_to_ascii (frame_p binfrm, frame_p ascfrm)
{
    int     i = 0;
    uint8   byte = 0;
    uint8 * pbin = NULL;
 
    if (!binfrm || binfrm->len < 1) return -1;
    if (!ascfrm) return -2;
 
    pbin = binfrm->data + binfrm->start;

    for (i = 0; i < binfrm->len; i++) {
        byte = ((pbin[i] >> 4) & 0x0F);
        if (byte < 10) frame_put_last(ascfrm, (byte + '0'));
        else if (byte < 16) frame_put_last(ascfrm, (byte -10 + 'A'));
        else frame_put_last(ascfrm, '.');
 
        byte = (pbin[i] & 0x0F);
        if (byte < 10) frame_put_last(ascfrm, (byte + '0'));
        else if (byte < 16) frame_put_last(ascfrm, (byte -10 + 'A'));
        else frame_put_last(ascfrm, '.');
    }
 
    return 0;
}
 
int frame_ascii_to_bin (frame_p ascfrm, frame_p binfrm)
{
    int     i;
    uint8   byte = 0;
    uint8 * pascii = NULL;
 
    if (!ascfrm || ascfrm->len < 1) return -1;
    if (!binfrm) return -2;
 
    pascii = ascfrm->data + ascfrm->start;

    for (i = 0; i < ascfrm->len; i++) {
        byte <<= 4;
        if (pascii[i] >= 'a' && pascii[i] <= 'f')
            byte |= pascii[i] - 'a' + 10;
        else if (pascii[i] >= 'A' && pascii[i] <= 'F')
            byte |= pascii[i] - 'A' + 10;
        else if (pascii[i] >= '0' && pascii[i] <= '9')
            byte |= pascii[i] - '0';
        else
            return -100;

        if ((i + 1) % 2 == 0) {
            frame_put_last(binfrm, byte);
            byte = 0;
        }
    }
 
    return 0;
}

int frame_slash_add (void * psrc, int len, void * pesc, int chlen, frame_p dstfrm)
{
    uint8 * p = (uint8 *)psrc;
    uint8 * escch = (uint8 *)pesc;
    int     i = 0, num = 0, totalnum = 0;
    uint8   dstbuf[4096];
    uint8   ch = 0;
 
    if (!p) return 0;
    if (len < 0) len = str_len(p);
    if (len <= 0) return 0;

    if (!dstfrm) return 0;
 
    if (escch && chlen < 0)
        chlen = str_len(escch);
    if (!escch) chlen = 0;
    if (!escch || chlen <= 0) {
        frame_put_nlast(dstfrm, p, len);
        return len;
    }
 
    for (i = 0, num = 0; i < len; i++) {
        ch = p[i];
        if (memchr(escch, ch, chlen) != NULL) {
            dstbuf[num++] = '\\';
 
            switch (ch) {
            case '\r':
                dstbuf[num++] = 'r';
                break;
            case '\n':
                dstbuf[num++] = 'n';
                break;
            case '\t':
                dstbuf[num++] = 't';
                break;
            case '\b':
                dstbuf[num++] = 'b';
                break;
            case '\f':
                dstbuf[num++] = 'f';
                break;
            default:
                dstbuf[num++] = ch;
                break;
            }
        } else {
            if (ch <= 0x1F) {
                dstbuf[num++] = '\\'; dstbuf[num++] = 'u';
                dstbuf[num++] = '0'; dstbuf[num++] = '0';
                dstbuf[num++] = '0' + ((ch >> 4) & 0xF);
                ch &= 0xF;
                dstbuf[num++] = (ch < 10) ? ('0' + ch) : ('A' + ch - 10);
            } else {
                dstbuf[num++] = ch;
            }
        }

        if (num >= sizeof(dstbuf) - 8) {
            frame_put_nlast(dstfrm, dstbuf, num);
            totalnum += num;
            num = 0;
        }
    }
 
    if (num > 0)
        frame_put_nlast(dstfrm, dstbuf, num);

    totalnum += num;
    return totalnum;
}
 
int frame_slash_strip (void * psrc, int len, void * pesc, int chlen, frame_p dstfrm)
{
    uint8 * p = (uint8 *)psrc;
    uint8 * escch = (uint8 *)pesc;
    int     i = 0, num = 0, totalnum = 0;
    uint8   ch = 0;
    uint8   dstbuf[4096];
    uint32  uval = 0;
    int     ret = 0;

 
    if (!p) return 0;
    if (len < 0) len = str_len(p);
    if (len <= 0) return 0;
 
    if (!dstfrm) return 0;

    if (escch && chlen < 0)
        chlen = str_len(escch);
    if (!escch) chlen = 0;
    if (!escch || chlen <= 0) {
        frame_put_nlast(dstfrm, p, len);
        return len;
    }
 
    for (i = 0, num = 0; i < len; ) {
        if (p[i] == '\\' && i+1<len) {
            switch (p[i+1]) {
            case '\\':
                ch = '\\';
                break;
            case '"':
                ch = '"';
                break;
            case '/':
                ch = '/';
                break;
            case '\'':
                ch = '\'';
                break;
            case 'r':
                ch = '\r';
                break;
            case 'n':
                ch = '\n';
                break;
            case 't':
                ch = '\t';
                break;
            case 'b':
                ch = '\b';
                break;
            case 'f':
                ch = '\f';
                break;

            case 'u':
            case 'x':
                if (len - i >= 3) {
                    ret = str_hextou(p + i + 2, 4, &uval);
                    if (ret <= 0) break;
 
                    if (uval <= 0xFF) dstbuf[num++] = uval & 0xFF;
                    else if (uval <= 0xFFFF) {
                        if (isBigEndian()) {
                            dstbuf[num++] = (uval >> 8) & 0xFF;
                            dstbuf[num++] = (uval & 0xFF);
                        } else {
                            dstbuf[num++] = (uval & 0xFF);
                            dstbuf[num++] = (uval >> 8) & 0xFF;
                        }
                    }
                    i += 2 + ret;
                    continue;
                }
                break;

            default:
                ch = p[i+1];
                break;
            }
 
            if (chlen > 0 && memchr(escch, ch, chlen) != NULL) {
                dstbuf[num++] = ch; i+=2;
            } else {
                dstbuf[num++] = p[i++];
                dstbuf[num++] = p[i++];
            }
        } else {
            dstbuf[num++] = p[i++];
        }
 
        if (num >= sizeof(dstbuf)-4) {
            frame_put_nlast(dstfrm, dstbuf, num);
            totalnum += num;
            num = 0;
        }
    }
 
    if (num > 0)
        frame_put_nlast(dstfrm, dstbuf, num);

    totalnum += num;

    return totalnum;
}

int frame_json_escape (void * psrc, int len, frame_p dstfrm)
{
    uint8   * src = (uint8 *)psrc;
    uint8     ch;
    int       i = 0, num = 0, totalnum = 0;
    uint8     dstbuf[4096];
 
    if (!src || len <= 0) return 0;

    if (dstfrm == NULL) {
        while (len > 0) {
            ch = *src++;
 
            if (ch == '\\' || ch == '"') {
                num++;
            } else if (ch <= 0x1f) {
                switch (ch) {
                case '\n':
                case '\r':
                case '\t':
                case '\b':
                case '\f':
                    num++;
                    break;
                default:
                    num += sizeof("\\u001F") - 2;
                }
            }
            num++;
            len--;
        }
 
        return num;
    }

    for (i = 0, num = 0; i < len; i++) {
        ch = src[i];

        if (ch > 0x1f) {
            if (ch == '\\' || ch == '"') {
                dstbuf[num++] = '\\';
            }
            dstbuf[num++] = ch;
        } else {
            dstbuf[num++] = '\\';
 
            switch (ch) {
            case '\n':
                dstbuf[num++] = 'n';
                break;
            case '\r':
                dstbuf[num++] = 'r';
                break;
            case '\t':
                dstbuf[num++] = 't';
                break;
            case '\b':
                dstbuf[num++] = 'b';
                break;
            case '\f':
                dstbuf[num++] = 'f';
                break;
            default:
                dstbuf[num++] = 'u'; dstbuf[num++] = '0'; dstbuf[num++] = '0';
                dstbuf[num++] = '0' + (ch >> 4);
                ch &= 0xf;
                dstbuf[num++] = (ch < 10) ? ('0' + ch) : ('A' + ch - 10);
                break;
            }
        }

        if (num >= sizeof(dstbuf) - 8) {
            frame_put_nlast(dstfrm, dstbuf, num);
            totalnum += num;
            num = 0;
        }
    }
 
    if (num > 0)
        frame_put_nlast(dstfrm, dstbuf, num);
 
    totalnum += num;
    return totalnum;
}

int frame_json_unescape (void * psrc, size_t size, frame_p dstfrm)
{
    uint8   * src = (uint8 *)psrc;
    uint8     ch;
    uint32    uval = 0;
    size_t    i = 0;
    int       ret, num = 0, totalnum = 0;
    uint8     dstbuf[4096];

    if (!src || size <= 0) return 0;
 
    if (dstfrm == NULL) {
        for (i = 0; i < size; ) {
            ch = src[i++];
 
            if (ch == '\\' && size - i >= 1) {
                ch = src[i++];
                switch (ch) {
                case 'n':
                case 'r':
                case 't':
                case 'b':
                case 'f':
                case '\\':
                case '"':
                    num++;
                    break;
                case 'u':
                    if (size - i >= 1) {
                        ret = str_hextou(src + i, 4, &uval);
                        if (ret <= 0) break;
 
                        i += ret;
                        if (uval <= 0xFF) num += 1;
                        else if (uval <= 0xFFFF) num += 2;
                    }
                    break;
                case 'x':   // \x0B  --> \v
                    ret = str_hextou(src + i, size - i, &uval);
                    if (ret <= 0) break;
 
                    i += ret;
                    if (uval <= 0xFF) num += 1;
                    else if (uval <= 0xFFFF) num += 2;
                    break;
                default:
                    if (ch >= '0' && ch <= '9') {  // \13 --> \v
                        i--;
                        ret = str_atou(src + i, size - i, &uval);
                        if (ret <= 0) break;
 
                        i += ret;
                        if (uval <= 0xFF) num += 1;
                        else if (uval <= 0xFFFF) num += 2;
                    }
                    break;
                }
            } else {
                num++;
            }
        }
 
        return num;
    }

    for (i = 0, num = 0; i < size; ) {
        ch = src[i++];
 
        if (ch == '\\' && size - i >= 1) {
            ch = src[i++];
            switch (ch) {
            case 'n':
                dstbuf[num++] = '\n';
                break;
            case 'r':
                dstbuf[num++] = '\r';
                break;
            case 't':
                dstbuf[num++] = '\t';
                break;
            case 'b':
                dstbuf[num++] = '\b';
                break;
            case 'f':
                dstbuf[num++] = '\f';
                break;
            case '\\':
                dstbuf[num++] = '\\';
                break;
            case '"':
                dstbuf[num++] = '"';
                break;
            case 'u':    // \u001F
                if (size - i >= 1) {
                    ret = str_hextou(src + i, 4, &uval);
                    if (ret <= 0) break;
 
                    i += ret;
                    if (uval <= 0xFF) dstbuf[num++] = uval & 0xFF;
                    else if (uval <= 0xFFFF) {
                        if (isBigEndian()) {
                            dstbuf[num++] = (uval >> 8) & 0xFF;
                            dstbuf[num++] = (uval & 0xFF);
                        } else {
                            dstbuf[num++] = (uval & 0xFF);
                            dstbuf[num++] = (uval >> 8) & 0xFF;
                        }
                    }
                }
                break;
            case 'x':   // \x0B  --> \v
                ret = str_hextou(src + i, size - i, &uval);
                if (ret <= 0) break;
 
                i += ret;
                if (uval <= 0xFF) dstbuf[num++] = uval & 0xFF;
                else if (uval <= 0xFFFF) {
                    if (isBigEndian()) {
                        dstbuf[num++] = (uval >> 8) & 0xFF;
                        dstbuf[num++] = (uval & 0xFF);
                    } else {
                        dstbuf[num++] = (uval & 0xFF);
                        dstbuf[num++] = (uval >> 8) & 0xFF;
                    }
                }
                break;
            default:
                if (ch >= '0' && ch <= '9') {  // \13 --> \v
                    i--;
                    ret = str_atou(src + i, size - i, &uval);
                    if (ret <= 0) break;
 
                    i += ret;
 
                    if (uval <= 0xFF) dstbuf[num++] = uval & 0xFF;
                    else if (uval <= 0xFFFF) {
                        if (isBigEndian()) {
                            dstbuf[num++] = (uval >> 8) & 0xFF;
                            dstbuf[num++] = (uval & 0xFF);
                        } else {
                            dstbuf[num++] = (uval & 0xFF);
                            dstbuf[num++] = (uval >> 8) & 0xFF;
                        }
                    }
                }
                break;
            }
        } else {
            dstbuf[num++] = ch;
        }

        if (num >= sizeof(dstbuf)-4) {
            frame_put_nlast(dstfrm, dstbuf, num);
            totalnum += num;
            num = 0;
        }
    }
 
    if (num > 0)
        frame_put_nlast(dstfrm, dstbuf, num);
 
    totalnum += num;
 
    return totalnum;
}

int frame_uri_encode (frame_p frm, void * psrc, int size, uint32 * escvec)
{
    uint8   * src = (uint8 *)psrc;
    uint8     ch = 0;
    int       n = 0;
    static uint8 hex[] = "0123456789ABCDEF";

    static uint32   uri[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x80000029, /* 1000 0000 0000 0000  0000 0000 0010 1001 */
                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x80000000, /* 1000 0000 0000 0000  0000 0000 0000 0000 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };

    if (!escvec) escvec = uri;

    while (size) {
 
        ch = *src++;
 
        if (escvec[ch >> 5] & (1U << (ch & 0x1f))) {
            n += 3;

            frame_put_last(frm, '%');
            frame_put_last(frm, hex[ch >> 4]);
            frame_put_last(frm, hex[ch & 0xf]);
 
        } else {
            n++;
            frame_put_last(frm, ch);
        }
 
        size--;
    }
 
    return n;
}

int frame_uri_decode (frame_p frm, void * psrc, int size)
{
    uint8  * s = (uint8 *)psrc;
    int      len = 0;
    uint8    ch, c, decoded;
    enum {
        sw_usual = 0,
        sw_quoted,
        sw_quoted_second
    } state;

    if (!frm) return -1;

    if (!s) return -2;
    if (size < 0) size = str_len(s);
    if (size <= 0) return 0;

    state = 0;
    decoded = 0;
 
    while (size--) {
 
        ch = *s++;
 
        switch (state) {
        case sw_usual:
            if (ch == '%') {
                state = sw_quoted;
                break;
            }

            len++;
            if (ch == '+') ch = ' ';
            frame_put_last(frm, ch);
            break;
 
        case sw_quoted:
            if (ch >= '0' && ch <= '9') {
                decoded = (uint8) (ch - '0');
                state = sw_quoted_second;
                break;
            }
 
            c = (uint8) (ch | 0x20);
            if (c >= 'a' && c <= 'f') {
                decoded = (uint8) (c - 'a' + 10);
                state = sw_quoted_second;
                break;
            }
 
            /* the invalid quoted character */
            state = sw_usual;
 
            len++;
            frame_put_last(frm, ch);
            break;
 
        case sw_quoted_second:
            state = sw_usual;
 
            if (ch >= '0' && ch <= '9') {
                ch = (uint8) ((decoded << 4) + (ch - '0'));
 
                len++;
                frame_put_last(frm, ch);
                break;
            }
 
            c = (uint8) (ch | 0x20);
            if (c >= 'a' && c <= 'f') {
                ch = (uint8) ((decoded << 4) + (c - 'a') + 10);
 
                len++;
                frame_put_last(frm, ch);
                break;
            }
 
            /* the invalid quoted character */
            break;
        }
    }
 
    return len;
}

int frame_html_escape (void * psrc, int len, frame_p dstfrm)
{
    uint8   * src = (uint8 *)psrc;
    uint8     ch;
    int       i = 0, num = 0, totalnum = 0;
    uint8     dstbuf[4096];
 
    if (!src) return 0;
    if (len < 0) len = str_len(src);
    if (len <= 0) return 0;

    if (dstfrm == NULL) {
        while (len > 0) {
            ch = *src++;
 
            switch (ch) {
            case '<':
                num += sizeof("&lt;") - 2;
                break;
 
            case '>':
                num += sizeof("&gt;") - 2;
                break;
 
            case '&':
                num += sizeof("&amp;") - 2;
                break;
 
            case '"':
                num += sizeof("&quot;") - 2;
                break;
            }

            num++;
            len--;
        }
 
        return num;
    }

    for (i = 0, num = 0; i < len; i++) {
        ch = src[i];
 
        switch (ch) {
        case '<':
            dstbuf[num++] = '&'; dstbuf[num++] = 'l';
            dstbuf[num++] = 't'; dstbuf[num++] = ';';
            break;

        case '>':
            dstbuf[num++] = '&'; dstbuf[num++] = 'g';
            dstbuf[num++] = 't'; dstbuf[num++] = ';';
            break;
 
        case '&':
            dstbuf[num++] = '&'; dstbuf[num++] = 'a';
            dstbuf[num++] = 'm'; dstbuf[num++] = 'p';
            dstbuf[num++] = ';';
            break;
 
        case '"':
            dstbuf[num++] = '&'; dstbuf[num++] = 'q';
            dstbuf[num++] = 'u'; dstbuf[num++] = 'o';
            dstbuf[num++] = 't'; dstbuf[num++] = ';';
            break;

        default:
            dstbuf[num++] = ch;
            break;
        }

        if (num >= sizeof(dstbuf) - 8) {
            frame_put_nlast(dstfrm, dstbuf, num);
            totalnum += num;
            num = 0;
        }
    }
 
    if (num > 0)
        frame_put_nlast(dstfrm, dstbuf, num);
 
    totalnum += num;
    return totalnum;
}

int frame_bit_set (frame_p frm, int bitpos, int value)
{
    int len, loc;

    if (!frm) return -1;
    if (bitpos < 0) return -1;

    len = (bitpos+1)/8 + ((bitpos+1) % 8 ? 1:0);
    loc = bitpos % 8;

    if (len > frm->len) return -1;

    if (value) {
        frm->data[frm->start + len - 1] |= (0x80 >> loc);
    } else {
        frm->data[frm->start + len - 1] &= ~(0x80 >> loc);
    }
    return 0;
}


int frame_bit_get (frame_p frm, int bitpos)
{
    int len, loc;

    if (!frm) return -1;
    if (bitpos < 0) return -1;

    len = (bitpos + 1)/8 + ((bitpos + 1) % 8 ? 1:0);
    loc = bitpos % 8;

    if (len > frm->len) return -1;

    if ((frm->data[frm->start + len - 1] & (0x80 >> loc)) != 0)
        return 1;

    return 0;
}


int frame_bit_shift_left (frame_p frm, int offset)
{
    uint8   byte = 0;
    uint8 * pbuf = NULL;
    int     i = 0;

    if (!frm || frm->len == 0)
         return -1;

    if (offset < 0) return -2;

    offset %= 8;
    if (offset == 0) return 0;

    pbuf = (uint8 *)frm->data + frm->start;

    byte = (pbuf[0] >> (8-offset)) & (uint8)((1 << offset) - 1);

    for (i = 0; i < frm->len - 1; i++) {
        pbuf[i] = (pbuf[i] << offset) | (pbuf[i+1] >> (8 - offset));
    }
    pbuf[frm->len - 1] <<= offset;

    if (byte > 0)
        frame_put_first(frm, byte);

    if (pbuf[frm->len - 1] == 0x00)
        frm->len -= 1;

    return 0;
}

int frame_bit_shift_right (frame_p frm, int offset)
{
    uint8   byte = 0;
    uint8 * pbuf = NULL;
    int     i = 0;

    if (!frm || frm->len == 0) return -1;
    if (offset < 0) return -2;

    offset %= 8;
    if (offset == 0) return 0;

    pbuf = frm->data + frm->start;

    byte = pbuf[frm->len - 1] << (8 - offset);

    for (i = frm->len - 1; i > 0; i--) {
        pbuf[i] = (pbuf[i] >> offset) | (pbuf[i-1] << (8 - offset));
    }
    pbuf[0] >>= offset;

    if (byte > 0) frame_put_last(frm, byte);

    if (pbuf[0] == 0) frame_del_first(frm, 1);

    return 0;
}

void frame_append_nbytes (frame_p frm, uint8 byte, int n)
{
    if (!frm || n <= 0) return;

    if (frame_rest(frm) < n) {
        frame_grow(frm, n - frame_rest(frm));
    }

    memset(frm->data + frm->start + frm->len, byte, n);

    frm->len += n;
}

frame_p frame_realloc (frame_p frm, int size)
{
    uint8  * pbyte = NULL;

    if (!frm) return frame_new(size);

    if (frm->size <= size) return frm;

    pbyte = frm->data;

    frm->data = kalloc(size + 1);

    if (frm->len > size)
        frm->len = size;

    if (frm->len > 0)
        memcpy(frm->data, pbyte + frm->start, frm->len);

    frm->start = 0;
    frm->size = size;

    kfree(pbyte);

    return frm;
}

int frame_add_time (frame_t * frame, time_t * curtm)
{
    time_t    curt;
    struct tm st;
    char      tmpbuf[64];

    if (!frame) return -1;

    if (curtm == NULL) time(&curt);
    else curt = *curtm;

    if (curt == 0) {
        sprintf(tmpbuf, "0000-00-00 00:00:00");
    } else {
        st = *localtime(&curt);
        sprintf(tmpbuf, "%04d-%02d-%02d %02d:%02d:%02d",
                st.tm_year+1900, st.tm_mon+1, st.tm_mday,
                st.tm_hour, st.tm_min, st.tm_sec);
    }

    frame_append(frame, tmpbuf);

    return 0;
}

