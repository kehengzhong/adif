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

#include <stdarg.h>
#include "btype.h"
#include "strutil.h"
#include "memory.h"
#include "patmat.h"
#include "btime.h"
#include "frame.h"

#ifdef UNIX
#include <iconv.h>
#include <ctype.h>
#include <sys/time.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#include <tchar.h>
#include <windows.h>
#include <WinNT.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
void ansi_2_unicode(wchar_t * wcrt, char * pstr)
{
    int     len = 0;
    int     unicodeLen =0;
    wchar_t pUnicode[4096];

    if (wcrt && pstr) {
        len = strlen(pstr);
        unicodeLen = MultiByteToWideChar(CP_ACP, 0, pstr, -1, NULL, 0);
        memset(pUnicode, 0, (unicodeLen + 1) * sizeof(wchar_t));
        MultiByteToWideChar( CP_ACP, 0, pstr, -1, (LPWSTR)pUnicode, unicodeLen);
        wcscpy(wcrt, pUnicode);
    }
}
#endif

void ckstr_free (void * str)
{
    kfree(str);
}

void * ckstr_new (void * pbyte, int bytelen)
{
     ckstr_t * ps = kzalloc(bytelen);
     ps->p = pbyte;  ps->len = bytelen;
     return ps;
}

int ckstr_cmp (void * a, void * b)
{
    ckstr_t * ca = (ckstr_t *)a;
    ckstr_t * cb = (ckstr_t *)b;
    int       len, ret = 0;

    if ((!ca || ca->len <= 0) && (!cb || cb->len <= 0))
        return 0;

    if (!ca || ca->len <= 0) return -1;
    if (!cb || cb->len <= 0) return 1;

    len = ca->len > cb->len ? cb->len : ca->len;

    ret = str_ncmp(ca->p, cb->p, len);
    if (ret == 0)
        return ca->len > cb->len ? 1 : (ca->len < cb->len ? -1 : 0);

    return ret;
}

int ckstr_casecmp (void * a, void * b)
{
    ckstr_t * ca = (ckstr_t *)a;
    ckstr_t * cb = (ckstr_t *)b;
    int       len, ret = 0;

    if ((!ca || ca->len <= 0) && (!cb || cb->len <= 0))
        return 0;

    if (!ca || ca->len <= 0) return -1;
    if (!cb || cb->len <= 0) return 1;

    len = ca->len > cb->len ? cb->len : ca->len;

    ret = str_ncasecmp(ca->p, cb->p, len);
    if (ret == 0)
        return ca->len > cb->len ? 1 : (ca->len < cb->len ? -1 : 0);

    return ret;
}

int toHex (int ch, int upercase) 
{
    int bch = 'a';

    if (upercase) bch = 'A';

    if (ch <= 9) return ch + '0';
    if (ch >= 10 && ch <= 15) return ch - 10 + bch; 
    return '.'; 
} 

size_t str_len (void * pstr)
{
    size_t   i = 0;
    uint8  * p = (uint8 *)pstr;

    if (!p) return 0;

    while (p[i] != '\0') i++;

    return i;
}

void * str_cpy (void * pdst, void * psrc)
{
    uint8  * dst = (uint8 *)pdst;
    uint8  * src = (uint8 *)psrc;

    if (!dst) return NULL;

    for ( ; *src; dst++, src++) *dst = *src; 
    *dst = '\0';

    return dst;
}

void * str_ncpy (void * pdst, void * psrc, size_t n) 
{
    uint8  * dst = (uint8 *)pdst;
    uint8  * src = (uint8 *)psrc;
 
    if (!src) return dst;

    if (n == 0 || !dst) return dst;
 
    while (n--) {
        *dst = *src;

        if (*dst == '\0') {
            return dst;
        }

        dst++;
        src++;
    }

    *dst = '\0';
 
    return dst;
}

int str_secpy (void * dst, int dstlen, void * src, int srclen)
{
    if (!dst || dstlen <= 0) return 0;
    if (!src) { *(uint8 *)dst = '\0'; return 0; }
 
    if (srclen < 0) srclen = str_len(src);
    if (srclen == 0) { *(uint8 *)dst = '\0'; return 0; }
 
    if (srclen > dstlen) srclen = dstlen; 
 
    return (uint8 *)str_ncpy(dst, src, srclen) - (uint8 *)dst;
} 

void * str_cat (void * s1, const void * s2)
{
    uint8 * tmp = (uint8 *)s1;
    uint8 * dst = (uint8 *)s1;
    uint8 * src = (uint8 *)s2;

    while (*dst) dst++;
    while ((*dst++ = *src++));

    return tmp;
}

void * str_ncat (void * s1, const void * s2, size_t n)
{
    uint8 * tmp = (uint8 *)s1;
    uint8 * dst = (uint8 *)s1;
    uint8 * src = (uint8 *)s2;

    while (*dst) dst++;

    while (n--) {
        if (!(*dst++ = *src++))
            break;
    }

    *dst = '\0';

    return tmp;
}

void * str_secat (void * dst, int dstlen, void * src, int srclen)
{
    if (!dst || dstlen <= 0) return dst;
    if (!src) return dst;
 
    if (srclen < 0) srclen = str_len(src);
    if (srclen == 0) return dst;
 
    if (srclen > dstlen) srclen = dstlen;
 
    return str_ncat(dst, src, srclen);
}


void * str_dup (void * p, int bytelen)
{
    uint8 * pbyte = (uint8 *)p;
    uint8 * pdup = NULL;

    if (!pbyte) return NULL;

    if (bytelen < 0)
        bytelen = str_len(pbyte);
    if (bytelen <= 0)
        return NULL;

    pdup = kalloc(bytelen + 1);
    if (!pdup)
        return NULL;

    memcpy(pdup, pbyte, bytelen);
    pdup[bytelen] = '\0';

    return pdup;
}

int str_cmp (void * ps1, void * ps2)
{
    uint8 * s1 = (uint8 *)ps1; 
    uint8 * s2 = (uint8 *)ps2; 
    int     c1, c2;
  
    for ( ;; ) {
        c1 = (int) *s1++;
        c2 = (int) *s2++; 
     
        if (c1 == c2) {
            if (c1) {
                continue; 
            }  
            return 0;
        } 
 
        return c1 - c2; 
    }
}

int str_ncmp (void * ps1, void * ps2, int n)
{
    uint8  * s1 = (uint8 *)ps1;
    uint8  * s2 = (uint8 *)ps2;

    while (n > 0 && *s1 && *s2) {
        if (*s1 != *s2) 
            return *s1 - *s2;
        s1++;
        s2++;
        n--;
    }

    if (n <= 0) return 0;

    if (*s1) return 1;

    return -1;
}

int str_casecmp (void * ps1, void * ps2)
{
    uint8 * s1 = (uint8 *)ps1;
    uint8 * s2 = (uint8 *)ps2;
    int     c1, c2;
 
    if (!s1 && !s2) return 0;
    if (!s1 && s2) return -1;
    if (s1 && !s2) return 1;

    for ( ;; ) {
        c1 = (int) *s1++;
        c2 = (int) *s2++;
 
        c1 = (c1 >= 'A' && c1 <= 'Z') ? (c1 | 0x20) : c1;
        c2 = (c2 >= 'A' && c2 <= 'Z') ? (c2 | 0x20) : c2;
 
        if (c1 == c2) {
            if (c1) {
                continue;
            }
            return 0;
        }
 
        return c1 - c2;
    }
}
 
int str_ncasecmp (void * ps1, void * ps2, size_t n)
{
    uint8 * s1 = (uint8 *)ps1; 
    uint8 * s2 = (uint8 *)ps2; 
    int     c1, c2;
  
    if (!s1 && !s2) return 0;
    if (!s1 && s2) return -1;
    if (s1 && !s2) return 1;

    while (n > 0) {
        c1 = (int) *s1++;
        c2 = (int) *s2++; 
     
        c1 = (c1 >= 'A' && c1 <= 'Z') ? (c1 | 0x20) : c1;
        c2 = (c2 >= 'A' && c2 <= 'Z') ? (c2 | 0x20) : c2;
 
        if (c1 == c2) {
            if (c1) {
                n--;
                continue; 
            }  
            return 0;
        } 
 
        return c1 - c2; 
    }

    return 0;
}

void * str_str2 (void * vstr, int len, void * vsub, int sublen)
{
    uint8 * pstr = (uint8 *)vstr;
    uint8 * psub = (uint8 *)vsub;
    int     i;

    if (!pstr) return NULL;
    if (len < 0) len = str_len(pstr);

    if (!psub) return NULL;
    if (sublen < 0) sublen = str_len(psub);

    if (len < sublen) return NULL;

    for (i = 0; i <= len - sublen; i++) {
        if (memcmp(pstr + i, psub, sublen) == 0)
            return pstr + i;
    }

    return NULL;
}

void * str_str (void * vstr, int len, void * vsub, int sublen)
{
    uint8 * pstr = (uint8 *)vstr;
    uint8 * psub = (uint8 *)vsub;

    int     BASE = 1000000;
    int     i, power = 1;
    int     code = 0;
    int     hash = 0;

    if (!pstr) return NULL;
    if (len < 0) len = str_len(pstr);

    if (!psub) return NULL;
    if (sublen < 0) sublen = str_len(psub);

    if (len < sublen) return NULL;

    for (i = 0; i < sublen; i++) {
        power = (power * 31) % BASE;
    }

    for (i = 0; i < sublen; i++) {
        code = (code * 31 + psub[i]) % BASE;
    }

    for (i = 0; i < len; i++) {
        hash = (hash * 31 + pstr[i]) % BASE;
        if (i < sublen - 1) continue;

        if (i >= sublen) {
            hash = hash - (pstr[i - sublen] * power) % BASE;
            if (hash < 0) hash += BASE;
        }

        if (hash == code) {
            if (memcmp(pstr + i - sublen + 1, psub, sublen) == 0)
                return pstr + i - sublen + 1;
        }
    }
    return NULL;
}


void * str_trim_head (void * pstr)
{
    uint8   * ptrim = (uint8 *)pstr;
    size_t    len=0, i=0;
    uint8   * p;

    if (!ptrim) return NULL;

    len = str_len(ptrim);
    if (len <= 0) return ptrim;

    p = &ptrim[0];
    for (i = 0; ISSPACE(*p); i++) {
        if (i >= len) { 
            ptrim[0] = '\0';
            return ptrim;
        }
        p++;
    }

    return p;
}

void * str_trim_tail (void * pstr)
{
    uint8   * ptrim = (uint8 *)pstr;
    size_t    len=0, i=0;
    uint8   * ptail;

    if (!ptrim) return NULL;

    len = str_len(ptrim);
    if (len <= 0) return ptrim;

    ptail = &ptrim[len-1];
    for (i = 0; ISSPACE(*ptail); i++) {
        if (i >= len) {
            ptrim[0] = '\0';
            break;
        }
        *ptail = '\0';
        ptail--;
    }

    return ptrim;
}


void * str_trim (void * pstr)
{
    uint8   * ptrim = (uint8 *)pstr;
    size_t    len=0, i=0;
    uint8   * p, * ptail;

    if (!ptrim) return NULL;

    len = str_len(ptrim);
    if (len <= 0) return ptrim;

    p = &ptrim[0];
    for (i = 0; ISSPACE(*p); i++) {
        if (i >= len) {
            ptrim[0] = '\0';
            return ptrim;
        }
        p++;
    }

    len = str_len(p);
    if (len <= 0) return p;

    ptail = &p[len-1];
    for (i = 0; ISSPACE(*ptail); i++) {
        if (i >= len) {
            p[0] = '\0';
            return p;
        }
        *ptail = '\0';
        ptail--;
    }

    return p;
}


int string_tolower (void * p, int bytelen)
{
    uint8 * pbyte = (uint8 *)p;
    int   i;

    if (!pbyte) return -1;
    if (bytelen < 0) bytelen = str_len(pbyte);
    if (bytelen < 1) return -2;

    for (i = 0; i < bytelen; i++) {
        if (pbyte[i] >= 'A' && pbyte[i] <= 'Z')
            pbyte[i] = pbyte[i] - 'A' + 'a';
    }
    return 0;
}

int string_toupper (void * p, int bytelen)
{
    uint8 * pbyte = (uint8 *)p;
    int   i;
 
    if (!pbyte) return -1;
    if (bytelen < 0) bytelen = str_len(pbyte);
    if (bytelen < 1) return -2;
 
    for (i = 0; i < bytelen; i++) {
        if (pbyte[i] >= 'a' && pbyte[i] <= 'z')
            pbyte[i] = pbyte[i] - 'a' + 'A';
    }
    return 0;
}


/* Multi-byte integers represent unsigned values and use variable length storage to 
   represent numbers. Each byte stores 7 bits of the integer. The high-order bit of
   each byte indicates whether the following byte is a part of the integer. If the 
   high-order bit is set, the lower seven bits are used and a next byte MUST be 
   consumed. If a byte has the high-order bit cleared (meaning that the value of the
   byte is less than 0x80) then that byte is the last byte of the integer. The least
   significant byte (LSB) of the integer appears first.

   The following table shows the number of bytes used to store a value in a certain range:
     _____________________________________________________
    | Range-from | Range-to   | Encoding-used             |
    | 0x00000000 | 0x0000007F | 1 byte                    |
    | 0x00000080 | 0x00003FFF | 2 bytes, LSB stored first |
    | 0x00004000 | 0x001FFFFF | 3 bytes, LSB stored first |
    | 0x00200000 | 0x0FFFFFFF | 4 bytes, LSB stored first |
    | 0x10000000 | 0x7FFFFFFF | 5 bytes, LSB stored first |
     -----------------------------------------------------
  For mb32 integers the resulting number MUST fit into a signed 32bit integer.
  For mb64 integers the resulting number MUST fit into a signed 64bit integer.  */

int str_mbi2uint (void * p, int len, uint32 * pmbival)
{
    uint8 * pbuf = (uint8 *)p;
    int     count;
    uint32  uval = 0;

    if (!pbuf || len < 1) return -1;

    for (count = 0; count < 5 && count < len; count++) {
        uval = (uval << 7) | (pbuf[count] & 0x7f);
        if (!(pbuf[count] & 0x80)) {
            if (pmbival) *pmbival = uval;
            return count + 1;
        }
    }

    if (pmbival) *pmbival = uval;

    return count;
}

int str_uint2mbi (uint32 value, void * p)
{   
    uint8 * pmbi = (uint8 *)p;
    uint8   octets[5];
    int     i, start;
    
    octets[4] = (value & 0x7f);
    value >>= 7;
    
    for (i = 3; value > 0 && i >= 0; i--) {
        octets[i] = (uint8)(0x80 | (value & 0x7f));
        value >>= 7;
    }   
    start = i + 1;

    if (pmbi)
        memcpy(pmbi, octets + start, 5 - start);

    return 5 - start;
}      

int str_atoi (void * p, int len, int * pval)
{
    uint8  * pbyte = (uint8 *)p;
    int      valsign = 1;
    int      i = 0, val = 0;

    if (!p || len <= 0) return 0;

    if (pbyte[i] == '-') { valsign = -1; i++; }
    else if (pbyte[i] == '+') { valsign = 1; i++; }

    for (val = 0; i < len; i++) {
        if (pbyte[i] >= '0' && pbyte[i] <= '9')
            val = val * 10 + (pbyte[i] - '0');
        else
            break;
    }

    val *= valsign;

    if (pval) *pval = val;
    return i;
}

int str_atol (void * p, int len, long * pval)
{
    uint8  * pbyte = (uint8 *)p;
    int      valsign = 1;
    int      i = 0;
    long     val = 0;

    if (!p || len <= 0) return 0;

    if (pbyte[i] == '-') { valsign = -1; i++; }
    else if (pbyte[i] == '+') { valsign = 1; i++; }

    for (val = 0; i < len; i++) {
        if (pbyte[i] >= '0' && pbyte[i] <= '9')
            val = val * 10 + (pbyte[i] - '0');
        else
            break;
    }

    val *= valsign;

    if (pval) *pval = val;
    return i;
}

int str_atoll (void * p, int len, int64 * pval)
{
    uint8  * pbyte = (uint8 *)p;
    int      valsign = 1;
    int      i = 0;
    int64    val = 0;

    if (!p || len <= 0) return 0;

    if (pbyte[i] == '-') { valsign = -1; i++; }
    else if (pbyte[i] == '+') { valsign = 1; i++; }

    for (val = 0; i < len; i++) {
        if (pbyte[i] >= '0' && pbyte[i] <= '9')
            val = val * 10 + (pbyte[i] - '0');
        else
            break;
    }

    val *= valsign;

    if (pval) *pval = val;
    return i;
}

int str_atou (void * p, int len, uint32 * pval)
{
    uint8  * pbyte = (uint8 *)p;
    int      i;
    uint32   val = 0;

    if (!p || len <= 0) return 0;

    for (val = 0, i = 0; i < len; i++) {
        if (pbyte[i] >= '0' && pbyte[i] <= '9')
            val = val * 10 + (pbyte[i] - '0');
        else
            break;
    }

    if (pval) *pval = val;
    return i;
}

int str_atoul (void * p, int len, ulong * pval)
{
    uint8  * pbyte = (uint8 *)p;
    int      i;
    ulong    val = 0;

    if (!p || len <= 0) return 0;

    for (val = 0, i = 0; i < len; i++) {
        if (pbyte[i] >= '0' && pbyte[i] <= '9')
            val = val * 10 + (pbyte[i] - '0');
        else
            break;
    }

    if (pval) *pval = val;
    return i;
}

int str_atoull (void * p, int len, uint64 * pval)
{
    uint8  * pbyte = (uint8 *)p;
    int      i;
    uint64   val = 0;

    if (!p || len <= 0) return 0;

    for (val = 0, i = 0; i < len; i++) {
        if (pbyte[i] >= '0' && pbyte[i] <= '9')
            val = val * 10 + (pbyte[i] - '0');
        else
            break;
    }

    if (pval) *pval = val;
    return i;
}

int str_hextoi (void * p, int len, int * pval)
{
    uint8  * pbyte = (uint8 *)p;
    int      valsign = 1;
    int      i = 0, val = 0;
    uint8    ch;

    if (!p || len <= 0) return 0;

    if (pbyte[i] == '-') { valsign = -1; i++; }
    else if (pbyte[i] == '+') { valsign = 1; i++; }

    for (val = 0; i < len; i++) {
        ch = pbyte[i];

        if (ch >= '0' && ch <= '9')
            val = val * 16 + (ch - '0');
        else if (ch >= 'a' && ch <= 'f')
            val = val * 16 + (ch - 'a');
        else if (ch >= 'A' && ch <= 'F')
            val = val * 16 + (ch - 'A');
        else break; 
    }

    val *= valsign;

    if (pval) *pval = val;
    return i;
}

int str_hextou (void * p, int len, uint32 * pval)
{
    uint8  * pbyte = (uint8 *)p;
    int      i;
    uint32   val = 0;
    uint8    ch;

    if (!p || len <= 0) return 0;

    for (val = 0, i = 0; i < len; i++) {
        ch = pbyte[i];

        if (ch >= '0' && ch <= '9')
            val = val * 16 + (ch - '0');
        else if (ch >= 'a' && ch <= 'f')
            val = val * 16 + (ch - 'a');
        else if (ch >= 'A' && ch <= 'F')
            val = val * 16 + (ch - 'A');
        else break; 
    }

    if (pval) *pval = val;
    return i;
}

int str_to_int (void * p, int bytelen, int base, void ** pterm)
{
    uint8 * pbyte = (uint8 *)p;
    uint8 * pbgn = NULL;
    uint8 * pend = NULL;
    int     valsign = 1;
    int     val = 0;

    if (!pbyte || bytelen <= 0) return 0;
    if (base <= 1) base = 10;

    pend = pbyte + bytelen;
    pbgn = skipOver(pbyte, pend-pbyte, " \t", 2);
    if (pbgn >= pend) {
        if (pterm) *pterm = pbgn;  
        return 0;
    }

    if (*pbgn == '-') { valsign = -1; pbgn++; }
    else if (*pbgn == '+') { valsign = 1; pbgn++; }
    
    for (val=0; pbgn<pend && isxdigit(*pbgn); pbgn++) {
        val *= base; 
        if (*pbgn >= '0' && *pbgn <= '9') val += *pbgn - '0';
        else if (*pbgn >= 'a' && *pbgn <= 'f') val += 10 + *pbgn - 'a';
        else if (*pbgn >= 'A' && *pbgn <= 'F') val += 10 + *pbgn - 'A';
    }

    if (pterm) *pterm = pbgn;  
    return val*valsign;
}

/* 2004-12-08 17:45:56+08
   Mon, 04 Jul 2050 07:07:07 GMT
   Mon, 24 Feb 2003 03:53:54 GMT       <=== subfmt=0 fmt=0 RFC1123 updated from RFC822
   Saturday, 30-Jun-2007 02:11:10 GMT  <=== subfmt=0 fmt=0 RFC1036 updated from RFC850
   Jan 20 2010 11:10:27                <=== subfmt=1 fmt=0 */

time_t str_gmt2time (void * p, int timelen, time_t * ptm)
{
    char      * ptime = (char *)p;
    time_t      tick = 0;
    int         tzone = 0;
    struct tm   ts;
    char      * pbgn = NULL;
    char      * pend = NULL;
    int         val = 0;
    char        sign = '+';
    int         i;
    static char * monthname[12] = {
                       "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    static char * weekname[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    uint8       fmt = 0;
    uint8       subfmt = 0;

    if (!ptime) return 0;
    if (timelen < 0) timelen = str_len(ptime);
    if (timelen <= 0) return 0;

    time(&tick);
    ts = *localtime(&tick);
    ts.tm_hour = ts.tm_min = ts.tm_sec = 0;

    pbgn = ptime;
    pend = ptime + timelen;

    pbgn = skipOver(pbgn, pend-pbgn, " \t", 2);
    if (pbgn >= pend) return 0;

    if (!isdigit(*pbgn)) {
        for (i = 0; i < 7; i++) {
            if (strncasecmp(pbgn, weekname[i], 3) == 0) {
                ts.tm_wday = i; break;
            }
        }

        if (i < 7) { //matched Weekday
            pbgn = skipTo(pbgn, pend-pbgn, ", \t", 3);
            if (pbgn >= pend) return 0;

            pbgn = skipOver(pbgn, pend-pbgn, ", \t", 3);
            if (pbgn >= pend) return 0;

            for (val = 0, i = 0; pbgn && pbgn < pend && isdigit(*pbgn) && i < 2; pbgn++, i++) {
                val *= 10; val += *pbgn - '0';
            }
            ts.tm_mday = val;

            pbgn = skipOver(pbgn, pend-pbgn, " \t-", 3);
            if (pbgn >= pend) return 0;

            subfmt = 0;
        } else {
            subfmt = 1;
        }

        for (i = 0; i < 12; i++) {
            if (strncasecmp(pbgn, monthname[i], 3) == 0) {
                ts.tm_mon = i; break;
            }
        }
        if (i >= 12) return 0;
        pbgn += 3;

        if (subfmt == 1) {
            pbgn = skipOver(pbgn, pend-pbgn, " \t-", 3);
            if (pbgn >= pend) return 0;

            for (val=0,i=0; pbgn && pbgn < pend && isdigit(*pbgn) && i<4; pbgn++, i++) {
                val *= 10; val += *pbgn - '0';
            }
            ts.tm_mday = val;
        }

        pbgn = skipOver(pbgn, pend-pbgn, " \t-", 3);
        if (pbgn >= pend) return 0;

        for (val=0,i=0; pbgn && pbgn < pend && isdigit(*pbgn) && i<4; pbgn++, i++) {
            val *= 10; val += *pbgn - '0';
        }
        if (val > 1900) {
            if (val < 1970) return 0;
            val -= 1900;
        } else if (val < 80 && val >= 0) {
            val = val + 2000 - 1900;
        } else if (i == 4 && val < 1900)
            return 0;

        ts.tm_year = val;

        fmt = 0;
    } else {
        /* year */
        while (pbgn < pend && !isdigit(*pbgn)) pbgn++;
        for (val = 0, i = 0; pbgn && pbgn < pend && isdigit(*pbgn) && i < 4; pbgn++, i++) {
            val *= 10; val += *pbgn - '0';
        }
        if (val > 1900) {
            if (val < 1970) return 0;
            val -= 1900;
        } else if (i == 2 && val < 80 && val >= 0) {
            val = val + 2000 - 1900;
        } else if (i == 4 && val < 1900)
            return 0;

        ts.tm_year = val;

       /* month */
        while (pbgn < pend && !isdigit(*pbgn)) pbgn++;

        for (val = 0, i = 0; pbgn && pbgn < pend && isdigit(*pbgn) && i < 2; pbgn++, i++) {
            val *= 10; val += *pbgn - '0';
        }
        val -= 1; if (val < 0) val = 0;
        ts.tm_mon = val;

        /* mday */
        while (pbgn < pend && !isdigit(*pbgn)) pbgn++;
        for (val=0,i=0; pbgn && pbgn < pend && isdigit(*pbgn) && i<2; pbgn++, i++) {
            val *= 10; val += *pbgn - '0';
        }
        ts.tm_mday = val;

        fmt = 1;
    }

    /* hour */
    while (pbgn < pend && !isdigit(*pbgn)) pbgn++;

    if (pbgn >= pend) {
        tick = mktime(&ts);
        if (ptm) *ptm = tick;
        return tick;
    }

    for (val=0,i=0; pbgn && pbgn < pend && isdigit(*pbgn) && i<2; pbgn++, i++) {
        val *= 10; val += *pbgn - '0';
    }
    ts.tm_hour = val;

    /* minute */
    while (pbgn < pend && !isdigit(*pbgn)) pbgn++;
    if (pbgn >= pend) {
        tick = mktime(&ts);
        if (ptm) *ptm = tick;
        return tick;
    }

    for (val=0,i=0; pbgn && pbgn < pend && isdigit(*pbgn) && i<2; pbgn++, i++) {
        val *= 10; val += *pbgn - '0';
    }
    ts.tm_min = val;

    /* second */
    while (pbgn < pend && !isdigit(*pbgn)) pbgn++;
    if (pbgn >= pend) {
        tick = mktime(&ts);
        if (ptm) *ptm = tick;
        return tick;
    }

    for (val = 0, i = 0; pbgn && pbgn < pend && isdigit(*pbgn) && i < 2; pbgn++, i++) {
        val *= 10; val += *pbgn - '0';
    }
    ts.tm_sec = val;

    if (fmt == 0) {
        pbgn = skipOver(pbgn, pend-pbgn, " \t", 2);
        if (pbgn >= pend) {
            tick = mktime(&ts);
            if (ptm) *ptm = tick;
            return tick;
        }

        if (strncasecmp(pbgn, "GMT", 3) != 0) {
            tick = mktime(&ts);
            if (ptm) *ptm = tick;
            return tick;
        }
        tzone = current_timezone();

    } else if (fmt == 1) {
        /* time zone */
        while (pbgn < pend && !isdigit(*pbgn) && *pbgn !='-' && *pbgn != '+') pbgn++;
        if (pbgn >= pend) {
            tick = mktime(&ts);
            if (ptm) *ptm = tick;
            return tick;
        }

        if (*pbgn == '+' || *pbgn == '-') { sign = *pbgn; pbgn++; }
        else sign = '+';
        for (val = 0, i = 0; pbgn && pbgn < pend && isdigit(*pbgn) && i < 2; pbgn++, i++) {
            val *= 10; val += *pbgn - '0';
        }
        if (sign == '-') val = -val;

        tzone = current_timezone();
        tzone -= val;
    }

    tick = mktime(&ts);
    tick += tzone * 3600;

    if (ptm) *ptm = tick;
    return tick;
}

/* time format definition 
   RFC1123-RFC822:  Sun, 06 Nov 1994 08:49:37 GMT
   RFC1036-RFC850:  Saturday, 30-Jun-2007 02:11:10 GMT  */

int str_time2gmt (time_t * ptm, void * gmtbuf, int len, int fmt)
{
    char        * timbuf = (char *)gmtbuf;
    time_t        tick;
    struct tm     gmtval;
    static char * monthname[12] = {
                       "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    static char * weekname[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static char * weekname2[7] = {"Sunday", "Monday", "Tuesday", "Wednesday",
                                  "Thursday", "Friday", "Saturday"};

 
    if (!timbuf || len < 36) return -1;
 
    memset(timbuf, 0, 36);
 
    if (!ptm) {
        tick = time(0);
        ptm = &tick;
    }
    gmtval = *gmtime(ptm);
 
    if (fmt == 0) {
        sprintf(timbuf, "%s, %02d %s %4d %02d:%02d:%02d GMT",
                weekname[gmtval.tm_wday], gmtval.tm_mday,
                monthname[gmtval.tm_mon], gmtval.tm_year + 1900,
                gmtval.tm_hour, gmtval.tm_min, gmtval.tm_sec);
    } else {
        sprintf(timbuf, "%s, %02d-%s-%4d %02d:%02d:%02d GMT",
                weekname2[gmtval.tm_wday], gmtval.tm_mday,
                monthname[gmtval.tm_mon], gmtval.tm_year + 1900,
                gmtval.tm_hour, gmtval.tm_min, gmtval.tm_sec);

    }
 
    return 0;
}

/* 2018-03-21 19:01:21 */
int str_datetime (time_t * ptm, void * dtbuf, int len, int fmt)
{
    time_t      curt;
    struct tm   st;
    char      * timbuf = (char *)dtbuf;

    if (!timbuf || len < 20) return -1;

    memset(timbuf, 0, 20);

    if (!ptm) {
        time(&curt);
        ptm = &curt;
    }
    st = *localtime(ptm);

    if (fmt == 0) {
        sprintf(timbuf, "%04d-%02d-%02d %02d:%02d:%02d",
                st.tm_year+1900, st.tm_mon+1, st.tm_mday,
                st.tm_hour, st.tm_min, st.tm_sec);
    } else {
        sprintf(timbuf, "%04d-%02d-%02d",
                st.tm_year+1900, st.tm_mon+1, st.tm_mday);
    }

    return 0;
}

/* Rabin-karp substring matching algorithm based on Rabin-karp hash computation */

#define madd(a, b, pr) ( ((a) + (b)) % (pr) )
#define msub(a, b, pr) ( ((a) > (b)) ? ((a) - (b)) : ((a) + (pr) - (b)) )
#define mmul(a, b, pr) ( ((a) * (b)) % (pr) )

static int64 mpow (int64 a, int b, int64 pr)
{
    int64 res = 1;
    int   i = 0;

    for (i = 0; i < b; i++) {
        res = mmul(res, a, pr);
    }

    return res;
}

/* calculate Rabin-karp hash:
   256^(m-1)*charbuf[0] + 256^(m-2)*charbuf[1] + ... + charbuf[m-1]
 */
static int64 rkhash (void * pb, int len, int64 pr)
{
    uint8 * p = (uint8 *)pb;
    int64   hash = 0;
    int     i = 0;

    for (i = 0; i < len; i++) {
        hash = madd (hash, mmul(p[i], mpow(256, len-i-1, pr), pr), pr);
    }
    return hash;
}

/* Given the rabin-karp hash value (curhash) over substring Y[i],Y[i+1],...,Y[i+m-1]
 * calculate the hash value over Y[i+1],Y[i+2],...,Y[i+m] = curhash * 256 - leftmost * h + rightmost
 * where h is 256 raised to the power m (and given as an argument).
 */
static int64 rkhash_next (int64 curhash, int64 h, uint8 leftmost, uint8 rightmost, int64 pr)
{
    return madd( msub( mmul(curhash, 256, pr), mmul(leftmost, h, pr), pr ), rightmost, pr);
}

void * str_rk_find (void * pbyte, int len, void * pattern, int patlen)
{
    uint8 * pstr = (uint8 *)pbyte;
    uint8 * pat = (uint8 *)pattern;
    int     i;
    int64   h;
    int64   pathash;
    int64   curhash;
    static int64 PRIME = 961748941;

    if (!pstr || !pat || patlen > len) return NULL;

    h = mpow(256, patlen, PRIME);

    pathash = rkhash(pat, patlen, PRIME);
    curhash = rkhash(pstr, patlen, PRIME);

    if (pathash == curhash) {
        if (memcmp(pstr, pat, patlen) == 0)
            return pstr;
    }

    for (i = 1; i < len; i++) {
        curhash = rkhash_next(curhash, h, pstr[i-1], pstr[i + patlen - 1], PRIME);
        if (curhash == pathash) {
            if (memcmp(pstr + i, pat, patlen) == 0)
                return pstr + i;
        }
    }

    return NULL;
}

void * str_find_bytes (void * pbyte, int len, void * pattern, int patlen)
{
    uint8 * pstr = (uint8 *)pbyte;
    uint8 * pat = (uint8 *)pattern;
    int     i, pos;
    uint8 * p = NULL;

    if (!pstr || len <= 0 || !pat || patlen <= 0) 
        return NULL;

    if (patlen > len) return NULL;

    pos = 0;

    do {
        p = memchr (pstr + pos, pat[0], len - pos);
        if (!p) return NULL;

        if (patlen > 1) {
            for (i = 1; i < patlen; i++) {
                if (pat[i] != p[i]) break;
            }
            if (i >= patlen) return p; /*match the string*/
            else pos = (int)(p - pstr + 1);  /*do not match*/
        } else
            return p;
    } while (pos <= len - patlen);

    return NULL;
}


void * str_rfind_bytes (void * pbyte, int pos, void * pattern, int patlen)
{
    uint8 * pstr = (uint8 *)pbyte;
    uint8 * pat = (uint8 *)pattern;
    int     i, j, pind;

    if (!pstr || pos <= 0 || !pat || patlen <= 0) return NULL;

    if (pos - patlen + 1 < 0) return NULL;

    for (i = pos; i >= patlen-1; i--) {
        if (pat[patlen-1] == pstr[i]) {
            if (patlen > 1) {
                for (j = 0, pind = i - patlen + 1; j < patlen - 1; j++) {
                    if (pat[j] != pstr[pind]) break;
                    pind ++;
                }
                if (j >= patlen-1) return &pstr[i-patlen+1];
            } else
                return &pstr[i-patlen+1];
        }
    }
    return NULL;
}


int bin_to_ascii (void * psrc, int binlen, void * pdst, int * asclen, int uppercase)
{   
    uint8  * pbin = (uint8 *)psrc;
    uint8  * pascii = (uint8 *)pdst;
    int      i = 0, covind = 0;
    uint8    byte = 0;
    
    if (!pbin || binlen < 1) return -1;
    if (!pascii) return -2;
    
    for (i = 0, covind = 0; i < binlen; i++) {
        byte = ((pbin[i] >> 4) & 0x0F);
        pascii[covind++] = toHex(byte, uppercase);

        byte = (pbin[i] & 0x0F);
        pascii[covind++] = toHex(byte, uppercase);
    }   
    
    if (asclen) *asclen = covind;
    return covind;   
}


int ascii_to_bin (void * psrc, int asciilen, void * pdst, int * binlen)
{
    uint8  * pascii = (uint8 *)psrc;
    uint8  * pbin = (uint8 *)pdst;
    int      i, conind;
    uint8    byte = 0;

    if (!pascii || asciilen < 1) return -1;

    for (i = 0, conind = 0; i < asciilen; i++) {
        byte <<= 4;
        if (pascii[i] >= 'a' && pascii[i] <= 'f') 
            byte |= pascii[i] - 'a' + 10;
        else if (pascii[i] >= 'A' && pascii[i] <= 'F') 
            byte |= pascii[i] - 'A' + 10;
        else if (pascii[i] >= '0' && pascii[i] <= '9')
            byte |= pascii[i] - '0';
        else
            return -100;

        if ((i+1)%2 == 0) {
            pbin[conind++] = byte;
            byte = 0;
        }
    }

    if (binlen) *binlen = conind;

    return conind;
}


int base64_to_bin (void * psrc, int asclen, void * pdst, int *binlen)
{
    uint8  * pasc = (uint8 *)psrc;
    uint8  * pbin = (uint8 *)pdst;
    uint32   triplet = 0;
    int      pos = 0, to = 0;
    int      quadpos = 0;
    int      warned = 0;
    int      sixbits;
    int      c;

    if (!pasc || asclen <= 0) return -1;
    if (!pbin || !binlen || *binlen <= 0) return -2;
    
    if (*binlen < (asclen + 3)/4 * 3) return -100;

    to = triplet = quadpos = 0;
    for (pos = 0; pos < asclen; pos++) {
        c = pasc[pos];

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
        } else if (ISSPACE(c)) {
            /* skip whitespace */
            continue;
        } else {
            if (!warned) {
                warned++;
            }
            continue;
        }

        triplet = (triplet << 6) | sixbits;
        quadpos++;

        if (quadpos == 4) {
            pbin[to++] = (char)((triplet >> 16) & 0xff);
            pbin[to++] = (char)((triplet >> 8) & 0xff);
            pbin[to++] = (char)(triplet & 0xff);
            quadpos = 0;
        }
    }

    /* Deal with leftover octets */
    switch (quadpos) {
    case 0:
        break;
    case 3:  /* triplet has 18 bits, we want the first 16 */
        pbin[to++] = (char)((triplet >> 10) & 0xff);
        pbin[to++] = (char)((triplet >> 2) & 0xff);
        break;
    case 2:  /* triplet has 12 bits, we want the first 8 */
        pbin[to++] = (char)((triplet >> 4) & 0xff);
        break;
    case 1:
        warned++;
        break;
    }

    *binlen = to;
    pbin[to] = '\0';

    return warned;
}

int bin_to_base64 (void * psrc, int binlen, void * pdst, int * asclen)
{
    uint8  * pbin = (uint8 *)psrc;
    uint8  * pasc = (uint8 *)pdst;
    static const uint8 base64[65] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int     triplets;
    uint32  tripval = 0;
    int     from = 0; 
    int     to = 0;
    
    if (!pbin || binlen <= 0) return -1;
    if (!pasc || !asclen || *asclen <= 0) return -2;

    triplets = (binlen + 2)/3;
    if (*asclen < triplets * 4 + (triplets + 18)/19 * 2 + 1) 
        return -100;

    /* The lines must be 76 characters each (or less), and each
     * triplet will expand to 4 characters, so we can fit 19
     * triplets on one line.  We need a CR LF after each line,
     * which will add 2 octets per 19 triplets (rounded up). */
    triplets = binlen / 3;   /* round up */

    for (to=0, from = 0; triplets > 0; triplets--) {
        tripval = (pbin[from] << 16) | 
                  (pbin[from+1] << 8)  |
                   pbin[from+2];
        from += 3;
        pasc[to++] = base64[(tripval >> 18) % 64];
        pasc[to++] = base64[(tripval >> 12) % 64];
        pasc[to++] = base64[(tripval >> 6) % 64];
        pasc[to++] = base64[(tripval) % 64];

        if (from % 57 == 0) {
            //pasc[to++] = 0x0D;
            //pasc[to++] = 0x0A;
        }
    }

    switch (binlen % 3) {   
    case 0:
        break;
    case 1:
        tripval = pbin[from] << 16;
        pasc[to++] = base64[(tripval >> 18) % 64];
        pasc[to++] = base64[(tripval >> 12) % 64];
        pasc[to++] = '=';
        pasc[to++] = '=';
        //pasc[to++] = 0x0D;
        //pasc[to++] = 0x0A;
        break;
    case 2:
        tripval = (pbin[from] << 16) | 
                  (pbin[from+1] << 8);
        from += 2;
        pasc[to++] = base64[(tripval >> 18) % 64];
        pasc[to++] = base64[(tripval >> 12) % 64];
        pasc[to++] = base64[(tripval >> 6) % 64];
        pasc[to++] = '=';
        //pasc[to++] = 0x0D;
        //pasc[to++] = 0x0A;
        break;
    }

    pasc[to] = 0x00;
    *asclen = to;
    return 0;
}


static int QuotedStrlen (void * p, int len, int start)
{
    uint8 * poct = (uint8 *)p;
    uint8   quote = 0;
    int     i = 0;

    if (!poct || len < 1) return 0;
    if (start >= len) return 0;

    quote = poct[start];
    if (quote != '"' && quote != '\'') return 0;

    for (i = start + 1; i < len; i++) {
        if (poct[i] == '\\') i++;
        else if (poct[i] == quote)
            return i - start + 1;
    }

    return 1; //only one quote
}       
        

void * skipQuoteTo (void * p, int len, void * toch, int num)
{
    uint8 * pbyte = (uint8 *)p;
    uint8 * tochars = (uint8 *)toch;
    int     i = 0, j = 0;
    int     qlen = 0;

    if (!pbyte) return NULL;
    if (len <= 0) return pbyte;
    if (!tochars || num <= 0) return pbyte;

    for (i = 0; i < len; ) {
        if (pbyte[i] == '\\' && i + 1 < len) {
            i += 2;
            continue;
        }

        for (j = 0; j < num; j++) {
            if (tochars[j] == pbyte[i]) return &pbyte[i];
        }
        if (pbyte[i] == '"' || pbyte[i] == '\'') {
            qlen = QuotedStrlen(pbyte, len, i);
            i += qlen;
            continue;
        }
        i++;
    }

    return &pbyte[i];
}

void * skipToPeer (void * p, int len, int leftch, int rightch)
{
    uint8 * pbgn = (uint8 *)p;
    uint8 * pend = NULL;
    int     bra = 0;
    int     revbra = 0;
 
    if (!pbgn) return NULL;
    if (len < 0) len = str_len(pbgn);
    if (len <= 0) return pbgn;
 
    pend = pbgn + len;
 
    if (*pbgn != leftch) return pbgn;
 
    for ( ; pbgn < pend; pbgn++) {
        if (*pbgn == rightch) {
            revbra++;
            if (revbra == bra)
                return pbgn;
 
        } else if (*pbgn == leftch) { 
            bra++;
        } 
    }    
         
    return pbgn;  
}        

void * skipTo (void * p, int len, void * toch, int num)
{
    uint8 * pbyte = (uint8 *)p;
    uint8 * tochars = (uint8 *)toch;
    int     i = 0, j = 0;
        
    if (!pbyte) return NULL;
    if (len <= 0) return pbyte;
    if (!tochars || num <= 0) return pbyte;
    
    for (i = 0; i < len; ) {
        for (j = 0; j < num; j++) {
            if (tochars[j] == pbyte[i]) return &pbyte[i]; 
        }
        i++; 
    }
    
    return &pbyte[i];
}

void * skipEscTo (void * p, int len, void * toch, int num)
{
    uint8 * pbyte = (uint8 *)p;
    uint8 * tochars = (uint8 *)toch;
    int  i = 0, j = 0;
 
    if (!pbyte) return NULL;
    if (len <= 0) return pbyte;
    if (!tochars || num <= 0) return pbyte;
 
    for (i = 0; i < len; i++) {
        if (pbyte[i] == '\\') { i++; continue; }

        for (j = 0; j < num; j++) {
            if (tochars[j] == pbyte[i]) return &pbyte[i];
        }
    }
 
    return &pbyte[i];
}
 
void * rskipTo (void * p, int len, void * toch, int num)
{
    uint8 * pbyte = (uint8 *)p;
    uint8 * tochars = (uint8 *)toch;
    int     i = 0, j = 0;

    if (!pbyte) return NULL;
    if (len <= 0) return pbyte;
    if (!tochars || num <= 0) return pbyte;

    for (i = 0; i < len; i++) {
        for (j = 0; j < num; j++) {
            if (tochars[j] == *(pbyte-i)) return pbyte-i;
        }
    }

    return pbyte-i;
}

void * skipOver (void * p, int len, void * skipch, int num)
{
    uint8 * pbyte = (uint8 *)p;
    uint8 * skippedchs = (uint8 *)skipch;
    int     i = 0, j = 0;

    if (!pbyte) return NULL;
    if (len <= 0) return pbyte;
    if (!skippedchs || num <= 0) return pbyte;

    for (i = 0; i < len; i++) {
        for (j = 0; j < num; j++) {
            if (skippedchs[j] == pbyte[i]) break;
        }
        if (j >= num) return &pbyte[i];
    }

    return &pbyte[i];
}

void * rskipOver (void * p, int rlen, void * skipch, int num)
{
    uint8 * pbyte = (uint8 *)p;
    uint8 * skippedchs = (uint8 *)skipch;
    int     i = 0, j = 0;

    if (!pbyte) return NULL;
    if (rlen <= 0) return pbyte;
    if (!skippedchs || num <= 0) return pbyte;

    for (i = 0; i < rlen; i++) {
        for (j = 0; j < num; j++) {
            if (skippedchs[j] == *(pbyte - i)) break;
        }
        if (j >= num) return pbyte - i;
    }

    return pbyte - i;
}

/* get the value pointer: username = 'hellokitty'    password = '#$!7798' */
int str_value_by_key (void * p, int bytelen, void * key, void ** pval, int * vallen)
{
    uint8 * pbyte = (uint8 *)p;
    uint8 * pbgn = NULL;
    uint8 * poct = NULL;
    uint8 * pend = NULL;
    int     keylen = 0;
    int     len = 0;

    if (pval) *pval = NULL;
    if (vallen) *vallen = 0;

    if (!pbyte || bytelen <= 0) return -1;
    if (!key) return -2;

    keylen = (int)str_len(key);

    pbgn = kmp_find_bytes(pbyte, bytelen, key, keylen, NULL);
    if (!pbgn) return -100;

    pend = pbyte + bytelen;
    pbgn = skipOver(pbgn+keylen, pend-pbgn-keylen, " \t", 2);
    if (*pbgn != '=') return -101;

    pbgn++;
    if (*pbgn == '\'' || *pbgn == '"') {
        len = QuotedStrlen(pbgn, pend-pbgn, 0);
        if (len > 1 && pbgn[len-1] == pbgn[0]) {
            len -= 2;
            if (pval) *pval = pbgn+1;
            if (vallen) *vallen = len;
            return len;
        }
    }
    poct = skipTo(pbgn, pend-pbgn, " \t,;", 4);
    if (poct == pbgn) return 0;
    if (pval) *pval = pbgn;
    if (vallen) *vallen = poct - pbgn;
    return poct - pbgn;
}


int secure_memcpy (void * dst, int dstlen, void * src, int srclen)
{
    if (!dst || dstlen <= 0) return -1;
    if (!src) return -2;

    if (srclen < 0) srclen = str_len(src);
    if (srclen == 0) return 0;

    if (srclen > dstlen) srclen = dstlen;

    memcpy(dst, src, srclen);

    return srclen;
}

long sec_memcpy (void * dst, void * src, long len)
{
    long    i = 0;
    uint8 * pdst = (uint8 *)dst;
    uint8 * psrc = (uint8 *)src;

    if (len <= 0) return 0;

    if (pdst == psrc) return 0;

    if (pdst > psrc) {
        for (i = len - 1; i >= 0; i--) pdst[i] = psrc[i];
    } else {
        for (i = 0; i < len; i++) pdst[i] = psrc[i];
    }

    return len;
}


/* escaped char: " \ ' \n \r \t \b \f \v */ 
int string_escape (void * psrc, int len, void * pesc, int chlen, void * pdststr, int dstlen)
{
    uint8 * p = (uint8 *)psrc;
    uint8 * escch = (uint8 *)pesc;
    uint8 * pdst = (uint8 *)pdststr;
    int     i = 0, num = 0;
    uint8   ch = 0;

    if (!p) return 0;
    if (len < 0) len = str_len(p);
    if (len <= 0) return 0;

    if (!pdst || dstlen <= 0) return 0;
     
    if (escch && chlen < 0) chlen = str_len(escch);
    if (!escch) chlen = 0;

    for (i = 0, num = 0; i < len && num < dstlen; i++) {
        ch = p[i];

        if (chlen > 0 && memchr(escch, ch, chlen) != NULL) {
            if (num + 2 > dstlen) return num;

            pdst[num++] = '\\';

            switch (ch) {
            case '\r':
                pdst[num++] = 'r';
                break;
            case '\n':
                pdst[num++] = 'n';
                break;
            case '\t':
                pdst[num++] = 't';
                break;
            case '\b':
                pdst[num++] = 'b';
                break;
            case '\f':
                pdst[num++] = 'f';
                break;
            default:
                pdst[num++] = ch;
                break;
            }
        } else {
            if (ch <= 0x1F) {
                pdst[num++] = '\\'; pdst[num++] = 'u';
                pdst[num++] = '0'; pdst[num++] = '0';
                pdst[num++] = '0' + ((ch >> 4) & 0xF);
                ch &= 0xF;
                pdst[num++] = (ch < 10) ? ('0' + ch) : ('A' + ch - 10);
            } else {
                pdst[num++] = ch;
            }
        }
    }

    return num;
}

int string_strip (void * psrc, int len, void * pesc, int chlen, void * pstrip, int dstlen)
{
    uint8 * p = (uint8 *)psrc;
    uint8 * pdst = (uint8 *)pstrip;
    uint8 * escch = (uint8 *)pesc;
    int     i = 0, num = 0, ret;
    uint8   ch = 0;
    uint32  uval = 0;
 
    if (pdst && dstlen > 0) pdst[0] = '\0';

    if (!p) return 0;
    if (len < 0) len = str_len(p);
    if (len <= 0) return 0;
 
    if (!pdst || dstlen <= 0) return 0;
 
    if (escch && chlen < 0) chlen = str_len(escch);
    else if (!escch) chlen = 0;
 
    for (i = 0, num = 0; i < len && num < dstlen; ) {
        if (p[i] == '\\' && i+1 <= len) {
            ch = p[i+1];

            switch (ch) {
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

                    if (uval <= 0xFF) pdst[num++] = uval & 0xFF;
                    else if (uval <= 0xFFFF) {
                        if (isBigEndian()) {
                            pdst[num++] = (uval >> 8) & 0xFF;
                            pdst[num++] = (uval & 0xFF);
                        } else {
                            pdst[num++] = (uval & 0xFF);
                            pdst[num++] = (uval >> 8) & 0xFF;
                        }
                    }
                    i += 2 + ret;
                    continue;
                }
                break;
            }

            if (chlen > 0 && memchr(escch, ch, chlen) != NULL) {
                pdst[num++] = ch; i+=2;
            } else {
                pdst[num++] = p[i++];
                pdst[num++] = p[i++];
            }
        } else {
            pdst[num++] = p[i++];
        }
    }
 
    return num;
}
 
void * string_strip_dup (void * p, int bytelen, void * pat, int chlen)
{
    uint8 * pbyte = (uint8 *)p;
    uint8 * escch = (uint8 *)pat;
    uint8 * pdup = NULL;
    int     i = 0, num = 0, ret;
    uint8   ch = 0;
    uint32  uval = 0;
 
    if (escch && chlen < 0) chlen = str_len(escch);
    if (!escch || chlen <= 0) return str_dup(pbyte, bytelen);

    if (!pbyte) return NULL;
    if (bytelen < 0) bytelen = str_len(pbyte);
    if (bytelen <= 0) return NULL;

    pdup = kzalloc(bytelen + 1);
    if (!pdup) return NULL;

    for (i = 0, num = 0; i < bytelen; ) {
        if (pbyte[i] == '\\' && i + 1 < bytelen) {
            ch = pbyte[i+1];

            switch (ch) {
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
            case '"':
                ch = '"';
                break;
            case '\\':
                ch = '\\';
                break;
            case '\'':
                ch = '\'';
                break;
            case '/':
                ch = '/';
                break;
            case 'u':
            case 'x':
                if (bytelen - i >= 3) {
                    ret = str_hextou(pbyte + i + 2, 4, &uval);
                    if (ret <= 0) break;
 
                    if (uval <= 0xFF) pdup[num++] = uval & 0xFF;
                    else if (uval <= 0xFFFF) {
                        if (isBigEndian()) {
                            pdup[num++] = (uval >> 8) & 0xFF;
                            pdup[num++] = (uval & 0xFF);
                        } else {
                            pdup[num++] = (uval & 0xFF);
                            pdup[num++] = (uval >> 8) & 0xFF;
                        }
                    }
                    i += 2 + ret;
                    continue;
                }
                break;
            }

            if (escch && chlen > 0 && memchr(escch, ch, chlen) != NULL) {
                pdup[num++] = ch; i+=2;
            } else {
                pdup[num++] = pbyte[i++];
                pdup[num++] = pbyte[i++];
            }
        } else {
            pdup[num++] = pbyte[i++];
        }
    }
 
    return pdup;
}

int json_escape (void * psrc, int size, void * pdst, int dstlen)
{
    uint8   * dst = (uint8 *)pdst;
    uint8   * src = (uint8 *)psrc;
    uint8     ch;
    int       len = 0;
 
    if (!src || size <= 0)
        return 0;

    if (dst == NULL) {
        while (size) {
            ch = *src++;
 
            if (ch == '\\' || ch == '"') {
                len++;
            } else if (ch <= 0x1f) {
                switch (ch) {
                case '\n':
                case '\r':
                case '\t':
                case '\b':
                case '\f':
                    len++;
                    break;
                default:
                    len += sizeof("\\u001F") - 2;
                }
            }
            len++;
            size--;
        }
 
        return len;
    }
 
    while (size > 0 && len < dstlen) {
        ch = *src++;
        size--;
 
        if (ch > 0x1f) {
            if (ch == '\\' || ch == '"') {
                *dst++ = '\\'; len++;

                if (len + 1 > dstlen) return --len;
            }
            *dst++ = ch; len++;
        } else {
            *dst++ = '\\'; len++;

            if (len + 1 > dstlen) return --len;
 
            switch (ch) {
            case '\n':
                *dst++ = 'n'; len++;
                break;
            case '\r':
                *dst++ = 'r'; len++;
                break;
            case '\t':
                *dst++ = 't'; len++;
                break;
            case '\b':
                *dst++ = 'b'; len++;
                break;
            case '\f':
                *dst++ = 'f'; len++;
                break;
            default:
                if (len + 5 > dstlen) return --len;

                *dst++ = 'u'; *dst++ = '0'; *dst++ = '0';
                *dst++ = '0' + (ch >> 4);
                ch &= 0xf;
                *dst++ = (ch < 10) ? ('0' + ch) : ('A' + ch - 10);
                len += 5;
                break;
            }
        }
    }
 
    return len;
}
 
int json_strip (void * psrc, int size, void * pdst, int dstlen)
{
    uint8   * dst = (uint8 *)pdst;
    uint8   * src = (uint8 *)psrc;
    uint8     ch;
    int       ret, len = 0;
    uint32    uval = 0;
    int       i = 0;
 
    if (!src || size <= 0) return 0;

    if (dst == NULL) {
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
                    len++;
                    break;
                case 'u':
                    if (size - i >= 1) {
                        ret = str_hextou(src + i, 4, &uval);
                        if (ret <= 0) break;

                        i += ret;
                        if (uval <= 0xFF) len += 1;
                        else if (uval <= 0xFFFF) len += 2;
                    }
                    break;
                case 'x':   // \x0B  --> \v
                    ret = str_hextou(src + i, size - i, &uval);
                    if (ret <= 0) break;

                    i += ret;
                    if (uval <= 0xFF) len += 1;
                    else if (uval <= 0xFFFF) len += 2;
                    break;
                default:
                    if (ch >= '0' && ch <= '9') {  // \13 --> \v
                        i--;
                        ret = str_atou(src + i, size - i, &uval);
                        if (ret <= 0) break;

                        i += ret;
                        if (uval <= 0xFF) len += 1;
                        else if (uval <= 0xFFFF) len += 2;
                    } else {
                        len += 1;
                    }
                    break;
                }
            } else {
                len++;
            }
        }
 
        return len;
    }
 
    for (i = 0, len = 0; i < size && len < dstlen; ) {
        ch = src[i++];
 
        if (ch == '\\' && size - i >= 1) {
            ch = src[i++];
            switch (ch) {
            case 'n':
                dst[len++] = '\n';
                break;
            case 'r':
                dst[len++] = '\r';
                break;
            case 't':
                dst[len++] = '\t';
                break;
            case 'b':
                dst[len++] = '\b';
                break;
            case 'f':
                dst[len++] = '\f';
                break;
            case '\\':
                dst[len++] = '\\';
                break;
            case '"':
                dst[len++] = '"';
                break;
            case 'u':    // \u001F
                if (size - i >= 1) {
                    ret = str_hextou(src + i, 4, &uval);
                    if (ret <= 0) break;

                    i += ret;
                    if (uval <= 0xFF) dst[len++] = uval & 0xFF;
                    else if (uval <= 0xFFFF) {
                        if (len + 2 > dstlen) return len;

                        if (isBigEndian()) {
                            dst[len++] = (uval >> 8) & 0xFF;
                            dst[len++] = (uval & 0xFF);
                        } else {
                            dst[len++] = (uval & 0xFF);
                            dst[len++] = (uval >> 8) & 0xFF;
                        }
                    }
                }
                break;
            case 'x':   // \x0B  --> \v
                ret = str_hextou(src + i, size - i, &uval);
                if (ret <= 0) break;

                i += ret;
                if (uval <= 0xFF) dst[len++] = uval & 0xFF;
                else if (uval <= 0xFFFF) {
                    if (len + 2 > dstlen) return len;

                    if (isBigEndian()) { 
                        dst[len++] = (uval >> 8) & 0xFF;
                        dst[len++] = (uval & 0xFF);
                    } else {
                        dst[len++] = (uval & 0xFF); 
                        dst[len++] = (uval >> 8) & 0xFF;
                    }
                }
                break;
            default:
                if (ch >= '0' && ch <= '9') {  // \13 --> \v
                    i--;
                    ret = str_atou(src + i, size - i, &uval);
                    if (ret <= 0) break;

                    i += ret;

                    if (uval <= 0xFF) dst[len++] = uval & 0xFF;
                    else if (uval <= 0xFFFF) {
                        if (len + 2 > dstlen) return len;

                        if (isBigEndian()) {  
                            dst[len++] = (uval >> 8) & 0xFF; 
                            dst[len++] = (uval & 0xFF); 
                        } else { 
                            dst[len++] = (uval & 0xFF);  
                            dst[len++] = (uval >> 8) & 0xFF; 
                        }    
                    }
                } else {
                    dst[len++] = ch;
                }
                break;
            }
        } else {
            dst[len++] = ch;
        }
    }
 
    return len;
}
 

void * json_strip_dup (void * psrc, int size)
{
    uint8   * src = (uint8 *)psrc;
    uint8   * dst = NULL;
    uint8     ch;
    int       ret, num = 0;
    uint32    uval = 0;
    int       i = 0;
 
    if (!src || size <= 0) return NULL;
 
    dst = kalloc(size + 1);
 
    for (i = 0, num = 0; i < size; ) {
        ch = src[i++];
 
        if (ch == '\\' && size - i >= 1) {
            ch = src[i++];
            switch (ch) {
            case 'n':
                dst[num++] = '\n';
                break;
            case 'r':
                dst[num++] = '\r';
                break;
            case 't':
                dst[num++] = '\t';
                break;
            case 'b':
                dst[num++] = '\b';
                break;
            case 'f':
                dst[num++] = '\f';
                break;
            case '\\':
                dst[num++] = '\\';
                break;
            case '"':
                dst[num++] = '"';
                break;
            case 'u':    // \u001F
                if (size - i >= 1) {
                    ret = str_hextou(src + i, 4, &uval);
                    if (ret <= 0) break;
 
                    i += ret;
                    if (uval <= 0xFF) dst[num++] = uval & 0xFF;
                    else if (uval <= 0xFFFF) {
                        if (isBigEndian()) {
                            dst[num++] = (uval >> 8) & 0xFF;
                            dst[num++] = (uval & 0xFF);
                        } else {
                            dst[num++] = (uval & 0xFF);
                            dst[num++] = (uval >> 8) & 0xFF;
                        }
                    }
                }
                break;
            case 'x':   // \x0B  --> \v
                ret = str_hextou(src + i, size - i, &uval);
                if (ret <= 0) break;
 
                i += ret;
                if (uval <= 0xFF) dst[num++] = uval & 0xFF;
                else if (uval <= 0xFFFF) {
                    if (isBigEndian()) {
                        dst[num++] = (uval >> 8) & 0xFF;
                        dst[num++] = (uval & 0xFF);
                    } else {
                        dst[num++] = (uval & 0xFF);
                        dst[num++] = (uval >> 8) & 0xFF;
                    }
                }
                break;
            default:
                if (ch >= '0' && ch <= '9') {  // \13 --> \v
                    i--;
                    ret = str_atou(src + i, size - i, &uval);
                    if (ret <= 0) break;
 
                    i += ret;
 
                    if (uval <= 0xFF) dst[num++] = uval & 0xFF;
                    else if (uval <= 0xFFFF) {
                        if (isBigEndian()) {
                            dst[num++] = (uval >> 8) & 0xFF;
                            dst[num++] = (uval & 0xFF);
                        } else {
                            dst[num++] = (uval & 0xFF);
                            dst[num++] = (uval >> 8) & 0xFF;
                        }
                    }
                } else {
                    dst[num++] = ch;
                }
                break;
            }
        } else {
            dst[num++] = ch;
        }
    }
 
    dst[num] = '\0';
    return dst;
}

int uri_uncoded_char (void * psrc, int size)
{
    uint8 * src = (uint8 *)psrc;
    int     i;
    int     chnum = 0;

                    /* " ", %00-%1F, %7F-%FF */
 
    static uint32   uri[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
 
                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x80000029, /* 0000 0000 0000 0000  0000 0000 0000 0001 */
 
                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
 
                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x80000000, /* 1000 0000 0000 0000  0000 0000 0000 0000 */
 
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };

    /*                         character table                   */
    /* 0x00(  0)  .... .... .... ....  .... .... .... ....  0x1F */
    /* 0x20( 32)   !"# $%&' ()*+ ,-./  0123 4567 89:; <=>?  0x3F */
    /* 0x40( 64)  @ABC DEFG HIJK LMNO  PQRS TUVW XYZ[ \]^_  0x5F */
    /* 0x60( 96)  `abc defg hijk lmno  pqrs tuvw xyz{ |}~.  0x7F */
    /* 0x80(128)  .... .... .... ....  .... .... .... ....  0x9F */
    /* 0xA0(160)  .... .... .... ....  .... .... .... ....  0xBF */
    /* 0xC0(192)  .... .... .... ....  .... .... .... ....  0xDF */
    /* 0xE0(224)  .... .... .... ....  .... .... .... ....  0xFF */

    if (!src) return 0;

    for (i = 0; i < size; i++) {
        if (uri[src[i] >> 5] & (1U << (src[i] & 0x1f))) {
            chnum++;
        }
    }

    if (chnum > 0) return 1;

    return 0;
}

/*  escape type value:
      ESCAPE_URI            0
      ESCAPE_ARGS           1
      ESCAPE_URI_COMPONENT  2
      ESCAPE_HTML           3
      ESCAPE_REFRESH        4
      ESCAPE_MEMCACHED      5
      ESCAPE_MAIL_AUTH      6
 */
int uri_encode (void * psrc, int size, void * pdst, int dstlen, int type)
{
    uint8        * dst = (uint8 *)pdst;
    uint8        * src = (uint8 *)psrc;
    uint8          ch = 0;
    int            n = 0;
    uint32       * escape;
    static uint8   hex[] = "0123456789ABCDEF";
 
                    /* " ", "#", "%", "?", %00-%1F, %7F-%FF */
 
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
 
                    /* " ", "#", "%", "&", "+", "?", %00-%1F, %7F-%FF */
 
    static uint32   args[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
 
                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x88000869, /* 1000 1000 0000 0000  0000 1000 0110 1001 */
 
                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
 
                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x80000000, /* 1000 0000 0000 0000  0000 0000 0000 0000 */
 
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };
 
                    /* not ALPHA, DIGIT, "-", ".", "_", "~" */
 
    static uint32   uri_component[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
 
                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0xfc009fff, /* 1111 1100 0000 0000  1001 1111 1111 1111 */
 
                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x78000001, /* 0111 1000 0000 0000  0000 0000 0000 0001 */
 
                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0xb8000001, /* 1011 1000 0000 0000  0000 0000 0000 0001 */
 
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };
 
                    /* " ", "#", """, "%", "'", %00-%1F, %7F-%FF */
 
    static uint32   html[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
 
                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x000000ad, /* 0000 0000 0000 0000  0000 0000 1010 1101 */
 
                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
 
                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x80000000, /* 1000 0000 0000 0000  0000 0000 0000 0000 */
 
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };
 
                    /* " ", """, "%", "'", %00-%1F, %7F-%FF */
 
    static uint32   refresh[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
 
                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x00000085, /* 0000 0000 0000 0000  0000 0000 1000 0101 */
 
                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
 
                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x80000000, /* 1000 0000 0000 0000  0000 0000 0000 0000 */
 
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };
 
                    /* " ", "%", %00-%1F */
 
    static uint32   memcached[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
 
                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x00000021, /* 0000 0000 0000 0000  0000 0000 0010 0001 */
 
                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
 
                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
 
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
    };
 
                    /* mail_auth is the same as memcached */
 
    static uint32  *map[] =
        { uri, args, uri_component, html, refresh, memcached, memcached };
 
 
    escape = map[type];
 
    if (dst == NULL) {
 
        /* find the number of the characters to be escaped */
 
        while (size) {
            if (escape[*src >> 5] & (1U << (*src & 0x1f))) {
                n += 2;
            }
            src++;
            size--;
            n++;
        }
 
        return n;
    }
 
    while (size) {

        ch = *src++;

        if (escape[ch >> 5] & (1U << (ch & 0x1f))) {
            n += 3;
            if (n <= dstlen) {
                *dst++ = '%'; 
                *dst++ = hex[ch >> 4];
                *dst++ = hex[ch & 0xf];
            }
 
        } else {
            n++;
            if (n <= dstlen)
                *dst++ = ch;
        }

        size--;
    }
 
    return n;
}
 
int uri_decode (void * psrc, int size, void * pdst, int dstlen)
{
    uint8  * s = (uint8 *)psrc;
    uint8  * d = (uint8 *)pdst;
    int      len = 0;
    uint8    ch, c, decoded;
    enum {
        sw_usual = 0,
        sw_quoted,
        sw_quoted_second
    } state;
 
    if (!psrc) return 0;
    if (size < 0) size = str_len(psrc);
    if (size == 0) return 0;
 
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
            if (len > dstlen) break;

            if (ch == '+') *d++ = ' ';
            else *d++ = ch;

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
            if (len > dstlen) break;

            *d++ = ch;
 
            break;
 
        case sw_quoted_second:
 
            state = sw_usual;
 
            if (ch >= '0' && ch <= '9') {
                ch = (uint8) ((decoded << 4) + (ch - '0'));
 
                len++;
                if (len > dstlen) break;

                *d++ = ch;
 
                break;
            }
 
            c = (uint8) (ch | 0x20);
            if (c >= 'a' && c <= 'f') {
                ch = (uint8) ((decoded << 4) + (c - 'a') + 10);
 
                len++;
                if (len > dstlen) break;

                *d++ = ch;
 
                break;
            }
 
            /* the invalid quoted character */
 
            break;
        }
    }
 
    *d = '\0';

    return len; 
}
 
int html_escape (void * psrc, int size, void * pdst, int dstlen)
{
    uint8 * src = (uint8 *)psrc;
    uint8 * dst = (uint8 *)pdst;
    uint8   ch;
    int     len = 0;
 
    if (dst == NULL) {

        len = size;

        while (size) {
            switch (*src++) {
 
            case '<':
                len += sizeof("&lt;") - 2;
                break;
 
            case '>':
                len += sizeof("&gt;") - 2;
                break;
 
            case '&':
                len += sizeof("&amp;") - 2;
                break;
 
            case '"':
                len += sizeof("&quot;") - 2;
                break;
 
            default:
                break;
            }
            size--;
        }
 
        return len;
    }
 
    while (size) {
        ch = *src++;
 
        switch (ch) {
 
        case '<':
            len += 4;
            if (len > dstlen) break;
            *dst++ = '&'; *dst++ = 'l'; *dst++ = 't'; *dst++ = ';';
            break;
 
        case '>':
            len += 4;
            if (len > dstlen) break;
            *dst++ = '&'; *dst++ = 'g'; *dst++ = 't'; *dst++ = ';';
            break;
 
        case '&':
            len += 5;
            if (len > dstlen) break;
            *dst++ = '&'; *dst++ = 'a'; *dst++ = 'm'; *dst++ = 'p';
            *dst++ = ';';
            break;
 
        case '"':
            len += 6;
            if (len > dstlen) break;
            *dst++ = '&'; *dst++ = 'q'; *dst++ = 'u'; *dst++ = 'o';
            *dst++ = 't'; *dst++ = ';';
            break;
 
        default:
            len++;
            if (len > dstlen) break;
            *dst++ = ch;
            break;
        }
        size--;
    }
 
    return len;
}
 

void * string_trim (void * p, int len, void * trim, int trimlen, int * plen)
{
    uint8  * ptmp = (uint8 *)p;
    uint8  * pend = NULL;
    uint8  * poct = NULL;
    int      actlen = 0;

    if (!p) return p;
    if (len < 0) len = str_len(ptmp);
    if (len <= 0) return p;

    poct = ptmp;
    pend = ptmp + len;

    for ( ; poct < pend; poct++) {
        if (ISSPACE(*poct)) continue;
        if (trim && trimlen > 0 && memchr(trim, *poct, trimlen)) continue;
        break;
    }
    if (poct >= pend) return poct;

    for (--pend; pend >= poct; pend--) {
        if (ISSPACE(*pend)) continue;
        if (trim && trimlen > 0 && memchr(trim, *pend, trimlen)) continue;
        break;
    }
    actlen = pend + 1 - poct;
    if (plen) *plen = actlen;

    return poct;
}

int string_tokenize (void * p, int len, void * pat, int seplen, void ** plist, int * plen, int arrnum)
{
    uint8  * ptmp = (uint8 *)p;
    uint8  * sep = (uint8 *)pat;
    uint8  * pbgn = NULL;
    uint8  * pend = NULL;
    uint8  * poct = NULL;
    int      num = 0;

    if (!p) return 0;
    if (len < 0) len = str_len(ptmp);
    if (len <= 0) return 0;

    if (!sep) {
        sep = (uint8 *)" \t,;";
        seplen = str_len(sep);
    }
    if (seplen < 0) seplen = str_len(sep);
    if (seplen <= 0) return 0;

    for (num = 0; num < arrnum; num++) {
        if (plist) plist[num] = NULL;
        if (plen) plen[num] = 0;
    }

    poct = ptmp;
    pend = ptmp + len;

    num = 0;
    while (poct < pend && num < arrnum) {
        pbgn = skipOver(poct, pend-poct, sep, seplen);
        if (pbgn >= pend) break;

        poct = skipQuoteTo(pbgn, pend-pbgn, sep, seplen);

        if (plist) plist[num] = pbgn;
        if (plen) plen[num] = poct - pbgn;
        num++;
    }

    return num;
}



/* randstr: contain the result random string
 * size:    destinated random string length
 * type=0:  only generate Alphabet string
 * type=1:  generate Alphabet and Digit String
 * type=2:  first char must be alphabet, rest char can be any char
 * type=3:  all chars are any char, used for valid string
 * type=4:  all possible displayed chars used as password purpose, etc.
 * type=5:  any bytes
 * type=20: generate lower-case alphabet and digit string
 * type=21: generate upper-case alphabet and digit string
 */
int GetRandStr (void * rstr, int size, int type)
{
    char * randstr = (char *)rstr;
    static char * alphalist = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static char * alphadigitlist = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static char * misclist = "0123456789_-abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static char * alllist = "~!@#$%^&*(),.[]|?:;0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static char * lowalphadigitlist = "123456789abcdefghkmnpqrstuvwxyz";
    static char * upalphadigitlist = "123456789ABCDEFGHJKLMNPQRSTUVWXYZ";
 
    ulong      alphalistlen = (ulong)strlen(alphalist);
    ulong      alphadigitlistlen = (ulong)strlen(alphadigitlist);
    ulong      misclistlen = (ulong)strlen(misclist);
    ulong      alllistlen = (ulong)strlen(alllist);
 
    ulong      lowalphadigitlistlen = (ulong)strlen(lowalphadigitlist);
    ulong      upalphadigitlistlen = (ulong)strlen(upalphadigitlist);
 
    struct timeval tick;
    uint32         seed = 0;
    int            randval = 0;
    int            i=0;
 
    if (!randstr || size <= 0) return 0;
 
    gettimeofday(&tick, NULL);
    seed = tick.tv_sec * 1000000 + tick.tv_usec;
    srand(seed);

    for (i = 0; i < size; i++) {
        randval = rand();
        if (type == 0) randstr[i] = alphalist[randval%alphalistlen];
        else if (type == 1) randstr[i] = alphadigitlist[randval%alphadigitlistlen];
        else if (type == 2) {
            if (i == 0) randstr[i] = alphalist[randval%alphalistlen];
            else randstr[i] = misclist[randval%misclistlen];
        } else if (type == 3) {
            randstr[i] = misclist[randval%misclistlen];
        } else if (type == 4) {
            randstr[i] = alllist[randval%alllistlen];
        } else if (type == 20) {
            randstr[i] = lowalphadigitlist[randval%lowalphadigitlistlen];
        } else if (type == 21) {
            randstr[i] = upalphadigitlist[randval%upalphadigitlistlen];
        } else { // if (type == 5) {
            randstr[i] = randval%256;
        }
    }
 
    return size;
}


#ifdef UNIX
/* perform character set conversion from source stream to destinating stream.
 * input parameters: 
 *    srcchst:  actual source character set name
 *    dstchst:  destination character set name to be converted
 *    orig   :  source byte stream pointer
 *    origlen:  source bytes number
 *    dest   :  destination byte stream pointer
 *    destlen:  destination bytes number pointer. it should carry dest stream space size
 * return value:
 *    *destlen: this pointer will be set the actual converted
 *              byte number contained in destination stream.
 *    return result: if < 0, that indicates error occured
 *                   if >= 0, indicates the actual converted bytes number of source stream
 */                  
int conv_charset (void * srcst, void * dstst,
                  void * psrc, int origlen,
                  void * pdst, int * destlen)
{
    char    * srcchst = (char *)srcst;
    char    * dstchst = (char *)dstst;
    char    * orig = (char *)psrc;
    char    * dest = (char *)pdst;
    iconv_t   hconv = 0;
    char    * pin = NULL;
    size_t    inlen = 0;
    char    * pout = NULL;
    size_t    outlen = 0;
    int       ret = 0;

    if (!srcchst || !dstchst) return -1;

    if (!orig || origlen <= 0) return -2;
    if (!dest || !destlen || *destlen <= 0) return -3;

    hconv = iconv_open(dstchst, srcchst);
    if (hconv == (iconv_t)-1) return -100;

    pin = orig;  inlen = origlen;
    pout = dest; outlen = *destlen;

    for (; inlen > 0 && outlen > 0; ) {
        ret = iconv(hconv, (char **)&pin, (size_t *)&inlen, (char **)&pout, (size_t *)&outlen);
        if (ret < 0) {
            if (errno == E2BIG) {
                /* The output buffer has no more room
                 * for the next converted character */
                iconv_close(hconv);
                return origlen - inlen; //return actual converted bytes in orignal stream
            } else if (errno == EINVAL) {
                /* An incomplete multibyte sequence is encountered
                 * in the input, and the input byte sequence terminates after it.*/
                *pout = *pin;
                pin++; pout++;
                inlen--; outlen--;
                continue;
            } else if (errno == EILSEQ) {
                /* An  invalid multibyte sequence is encountered in the input.
                 * *inbuf  is  left pointing to the beginning of the
                 * invalid multibyte sequence. */
                *pout = *pin;
                pin++; pout++;
                inlen--; outlen--;
                continue;
            } else break;
        }
    }

    iconv_close(hconv);

    *destlen = *destlen - outlen;
    return origlen - inlen;

}

#endif


/* supported formats as following:
    %[flags][width][.precision][length]specifier

  5 Flags: The character % is followed by zero or more of the following flags:
     #  For o,x,X conversion, output string of non-zero result is prepended 0 or 0x or 0X.
        For a,A,e,E,f,F,g,G conversions, the result will always contain a decimal point,
        even if no digits follow it.
        For g,G conversions, trailing zeros are not removed from the result
     0  For d,i,o,u,x,X,a,A,e,E,f,F,g,G conversions, the converted value is padded on the
        left with zeros rather than blanks. If the 0 and - flags both appear, the 0 flag
        is ignored. If a precision is given with a numeric conversion (d,i,o,u,x,and X), 
        the 0 flag is ignored.
     -  The  converted  value is to be left adjusted on the field boundary.
    ' ' (a space) A blank should be  left  before  a  positive  number.
     +  A sign (+ or -) should be placed before a number produced by a signed conversion.

  The Width field:
    number  It specifies a minimum field width. The value of fewer characters is padded
            with spaces on the left or right.
    *       The width is given in the next argument.

  The precision:
    .number It is in the form of a period '.' followed by decimal digit string or *. 
    .*      If * is followed, the precision is given in the next argument.
            For d,i,o,u,x,X conversions, the minimum number of digits is to be appeared.
            For a,A,e,E,f,F conversions, the number of digits is appeared after the radix
            character
            For g,G conversion, the maximum number of significant digits is appeared.
            For s,S conversion, the maximum number of characters are printed from string.

  The Length field:
    h   Integer conversion corresponds to a short int or unsigned short int argument
    l   (ell)Integer conversion corresponds to a long int or unsigned long int argument
    ll  Integer conversion corresponds to a long long int or unsigned long long int argument
    L   A following a,A,e,E,f,F,g,G conversion corresponds to a long double argument

  The conversion specifier:
    c        the int argument is converted to an unsigned char
    d,i      The int argument is converted to signed decimal notation
    o,u,x,X  The unsigned int argument is converted to unsigned octal(o), unsigned
             decimal(u), or unsigned hexadecimal (x and X) notation
    e,E      The double argument is rounded and converted in the style [-]d.ddde±dd
             where there is one digit before the decimal-point character and the number
             of digits after it is equal to the precision; if the precision is missing,
             it is taken as 6; if the precision is  zero, no decimal-point  character
             appears. An E conversion uses the letter E (rather than e) to introduce
             the exponent. The exponent always contains at least two  digits; if the
             value is zero, the exponent is 00.
    f,F      The double argument is rounded and converted to decimal notation in the
             style [-]ddd.ddd, where the number of digits after the decimal-point char-
             acter is equal to the precision specification. If the precision is miss-
             ing, it is taken as 6; if the precision is explicitly  zero,  no  decimal-
             point  character  appears. If a decimal point appears, at least one digit
             appears before it.
    g,G      The double argument is converted in style f,e (or F,E,G  convesions).
             The precision specifies the number of significant digits.If the precision
             is missing, 6 digits are given; if the precision is zero, it is treated as
             1. Style e is used if the exponent from its conversion is less than -4 or
             greater than or equal to the  precision.   Trailing  zeros  are removed
             from the fractional part of the result; a decimal point appears only if it
             is followed by at least one digit.
    s        The char * argument is expected to be a pointer to a string
    p        The void * pointer argument is printed in hexadecimal
    %        A '%' is written
    V        The ckstr_t * argument is printed as string
    W        The frame_t * argument is printed as string.
 */

int fmt_mem_cpy (uint8 * pdst, int dstlen, uint8 * psrc, int srclen, int width, int leftalg)
{
    int i, ret = 0;

    if (!pdst) return 0;
    if (!psrc || srclen <= 0) return 0;

    if (leftalg) {
        ret += str_secpy(pdst, dstlen, psrc, srclen);
        for (i = srclen; ret < dstlen && i < width; i++) {
            pdst[ret++] = ' ';
        }
    } else {
        for (i = 0; ret < dstlen && i < width - srclen; i++) {
            pdst[ret++] = ' ';
        }
        if (ret < dstlen)
            ret += str_secpy(pdst + ret, dstlen - ret, psrc, srclen);
    }

    return ret;
}

int fmt_file_cpy (FILE * fp, uint8 * psrc, int srclen, int width, int leftalg)
{
    int i, ret = 0;
    int ch = 32;

    if (!fp) return 0;
    if (!psrc || srclen <= 0) return 0;

    if (leftalg) {
        ret += fwrite(psrc, 1, srclen, fp);
        if (srclen < width) {
            for (i = srclen; i < width; i++) ret += fputc(ch, fp);
            //ret += fwrite(" ", 1, width - srclen, fp);
        }
    } else {
        if (srclen < width) {
            for (i = srclen; i < width; i++) ret += fputc(ch, fp);
            //ret += fwrite(" ", 1, width - srclen, fp);
        }
        ret += fwrite(psrc, 1, srclen, fp);
    }

    return ret;
}

int fmt_conv_numval (uint8 * buf, int len, uint64 val, uint8 dectype, uint8 ** ppval)
{
    static uint8 hex[] = "0123456789abcdef";
    static uint8 HEX[] = "0123456789ABCDEF";
    int num = 0;
    uint8 * p = NULL;

    if (ppval) *ppval = buf;

    if (!buf || len < 32) return 0;

    p = buf + len - 1; *p = '\0';


    if (dectype == 1) { //octet
        do {
            *--p = (uint8)( (val & 0x07) + '0'); num++;
        } while(val >>= 3);

    } else if (dectype == 2) { //hex-lower case
        do {
            *--p = hex[(uint32)(val & 0x0F)]; num++;
        } while(val >>= 4);

    } else if (dectype == 3) { //hex-upper case
        do {
            *--p = HEX[(uint32)(val & 0x0F)]; num++;
        } while(val >>= 4);

    } else { //decimal
        do {
            *--p = (uint8)((val % 10) + '0'); num++;
        } while(val /= 10);
    }

    if (ppval) *ppval = p;

    return num;
}


int kvsnprintf (void * vdst, int dstlen, FILE * fp, void * vfmt, va_list ap)
{
    uint8   * pdst = (uint8 *)vdst;
    uint8   * fmt = (uint8 *)vfmt;
    uint8     flag = 0;
    int       width = 0;
    int       prec = 0;
    uint8     lenfld = 0;
    uint8     nega = 0;

    int       ival = 0;
    uint64    ulval = 0;
    uint64    ulval2 = 0;
    int64     lval = 0;
    double    fval = 0.0;
    int       expval = 0;
    uint8     expnega = 0;

    frame_p   frm = NULL;
    ckstr_t * cks = NULL;
    uint8   * ptr = NULL;
    uint8     buf[256];
    int       wlen = 0;
    int       slen = 0;
    uint8     dectype = 0;

    int       wmlen = 0;
    int       fplen = 0;

    if (dstlen < 0) dstlen = 1024*1024*1024;
    if (!fmt) return -1;

    while (*fmt) {
        if (pdst && !fp && dstlen >= wmlen) break;

        if (*fmt != '%') {
            wmlen += fmt_mem_cpy(pdst, dstlen - wmlen, fmt, 1, 1, 0);
            fplen += fmt_file_cpy(fp, fmt, 1, 1, 0);

            fmt++;
            continue;
        }
        fmt++;

        flag = 0; width = 0; prec = -1; lenfld = 0; nega = 0;
        ival = ulval = lval = 0;

        /* the flag field */
        if (*fmt == '0') { flag = 1; fmt++; }
        else if (*fmt == '-') { flag = 2; fmt++; }
        else if (*fmt == '+') { flag = 3; fmt++; }
        else if (*fmt == '#') { flag = 4; fmt++; }
        else if (*fmt == ' ') { flag = 5; fmt++; }

        /* the width field */
        if (*fmt == '*') {
            width = va_arg(ap, int);
            fmt++;
        } else {
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + (*fmt++ - '0');
            }
        }

        /* the precision field */
        if (*fmt == '.') {
            fmt++;
            if (*fmt == '*') {
                prec = va_arg(ap, int);
                fmt++;
            } else {
                if (*fmt >= '0' && *fmt <= '9') {
                    prec = 0;
                    while (*fmt >= '0' && *fmt <= '9') {
                        prec = prec * 10 + (*fmt++ - '0');
                    }
                }
            }
        }

        /* the length field */
        if (*fmt == 'h') { lenfld = 1; fmt++; }
        else  if (*fmt == 'l') {
            lenfld = 2; fmt++;
            if (*fmt == 'l') { lenfld = 3; fmt++; }
        } else if (*fmt == 'L') {
            lenfld = 4; fmt++;
        }

        /* the conversion specified */
        switch (*fmt) {
        case 'W':   /* frame_p  */
            fmt++;
            frm = va_arg(ap, frame_p);
            if (!frm) continue;

            slen = wlen = frm->len;
            ptr = (uint8 *)frameP(frm);

            if (prec > 0) slen = min(prec, slen);
            if (width > 0) wlen = max(width, wlen);

            wmlen += fmt_mem_cpy(pdst, dstlen - wmlen, ptr, slen, wlen, flag == 2);
            fplen += fmt_file_cpy(fp, ptr, slen, wlen, flag == 2);
            continue;

        case 'V':  /* ckstr_t *  */
            fmt++;
            cks = va_arg(ap, ckstr_t *);
            if (!cks) continue;

            slen = wlen = cks->len;
            ptr = (uint8 *)cks->p;

            if (prec > 0) slen = min(prec, slen);
            if (width > 0) wlen = max(width, wlen);

            wmlen += fmt_mem_cpy(pdst, dstlen - wmlen, ptr, slen, wlen, flag == 2);
            fplen += fmt_file_cpy(fp, ptr, slen, wlen, flag == 2);
            continue;

        case 's':  /* char * */
            fmt++;
            ptr = va_arg(ap, uint8 *);
            if (!ptr) continue;

            slen = wlen = str_len(ptr);

            if (prec > 0) {
                slen = min(prec, slen);
            }
            wlen = max(width, slen);

            wmlen += fmt_mem_cpy(pdst, dstlen - wmlen, ptr, slen, wlen, flag == 2);
            fplen += fmt_file_cpy(fp, ptr, slen, wlen, flag == 2);
            continue;

        case 'c':
            ival = va_arg(ap, int);
            dectype = ival & 0xFF;

            ptr = &dectype;
            wlen = slen = 1;
            if (width > 0) wlen = max(width, 1);

            wmlen += fmt_mem_cpy(pdst, dstlen - wmlen, ptr, slen, wlen, flag == 2);
            fplen += fmt_file_cpy(fp, ptr, slen, wlen, flag == 2);
            fmt++;
            continue;

        case 'd':
        case 'i':
            fmt++;
            if (lenfld == 1) lval = va_arg(ap, int);
            else if (lenfld == 2) lval = va_arg(ap, long);
            else if (lenfld == 3) lval = va_arg(ap, int64);
            else lval = va_arg(ap, int);

            if (lval < 0) ulval = -lval;
            else ulval = lval;

            slen = fmt_conv_numval(buf, sizeof(buf), ulval, 0, &ptr);
            if (prec >= 0) { // precision field exist
                while (slen < prec && ptr > buf) { *--ptr = '0'; slen++; }
            } else { //precision field not exist
                if (flag == 1) { // flag is 0
                    if (lval < 0)
                        while (slen < width - 1 && ptr > buf + 1) { *--ptr = '0'; slen++; }
                    else
                        while (slen < width && ptr > buf + 1) { *--ptr = '0'; slen++; }
                }
            }

            if (lval < 0) { *--ptr = '-'; slen++; }
            else if (flag == 3) { *--ptr = '+';  slen++; } //flag is +

            wmlen += fmt_mem_cpy(pdst, dstlen - wmlen, ptr, slen, width, flag == 2);
            fplen += fmt_file_cpy(fp, ptr, slen, width, flag == 2);
            continue;

        case 'u':
        case 'o':
        case 'x':
        case 'X':
            if (*fmt == 'o') dectype = 1;
            else if (*fmt == 'x') dectype = 2;
            else if (*fmt == 'X') dectype = 3;
            else dectype = 0;

            fmt++;

            if (lenfld == 1) ulval = va_arg(ap, uint32);
            else if (lenfld == 2) ulval = va_arg(ap, ulong);
            else if (lenfld == 3) ulval = va_arg(ap, uint64);
            else ulval = va_arg(ap, uint32);

            slen = fmt_conv_numval(buf, sizeof(buf), ulval, dectype, &ptr);
            if (prec >= 0) { // precision field exist
                while (slen < prec && ptr > buf + 2) { *--ptr = '0'; slen++; }
            } else { //precision field not exist
                if (flag == 1) { // flag is 0
                    while (slen < width && ptr > buf + 2) { *--ptr = '0'; slen++; }
                }
            }

            if (flag == 4) {  //flag is #
                if (dectype == 1) { *--ptr = '0'; slen++; }
                else if (dectype == 2) { *--ptr = 'x'; *--ptr = '0'; slen += 2; }
                else if (dectype == 3) { *--ptr = 'X'; *--ptr = '0'; slen += 2; }
            }

            wmlen += fmt_mem_cpy(pdst, dstlen - wmlen, ptr, slen, width, flag == 2);
            fplen += fmt_file_cpy(fp, ptr, slen, width, flag == 2);
            continue;

        case 'p':
            ulval = (uint64)va_arg(ap, void *);
            fmt++;

            slen = fmt_conv_numval(buf, sizeof(buf), ulval, 2, &ptr);
            if (prec >= 0) { // precision field exist
                while (slen < prec && ptr > buf + 3) { *--ptr = '0'; slen++; }
            } else { //precision field not exist
                if (flag == 1) { // flag is 0
                    while (slen < width - 2 && ptr > buf + 3) { *--ptr = '0'; slen++; }
                }
            }

            *--ptr = 'x'; *--ptr = '0'; slen += 2;

            if (flag == 3) { *--ptr = '+';  slen++; } //flag is +
            else if (flag == 5) { *--ptr = ' ';  slen++; } //flag is ' '

            wmlen += fmt_mem_cpy(pdst, dstlen - wmlen, ptr, slen, width, flag == 2);
            fplen += fmt_file_cpy(fp, ptr, slen, width, flag == 2);
            continue;

        case 'f':
        case 'F':
            fmt++;
            fval = va_arg(ap, double);
            if (prec < 0) prec = 6;

            if (fval < 0) {
                nega = 1; fval = -fval;
            }

            ulval = (uint64)fval;

            if (prec > 0) {
                uint64  scale = 1;

                for (ival = 0; ival < prec; ival++) scale *= 10;
                ulval2 = (uint64)( (fval - (double)ulval) * scale + 0.5);
                if (ulval2 == scale) {
                    ulval++; ulval2 = 0;
                }
            }

            slen = fmt_conv_numval(buf, sizeof(buf), ulval2, 0, &ptr);
            if (prec >= 0) { // precision field exist
                while (slen < prec && ptr > buf + 3) { *--ptr = '0'; slen++; }
            }
            wlen = fmt_conv_numval(buf, ptr - buf, ulval, 0, &ptr);
            *(ptr + wlen) = '.';
            slen += wlen + 1;

            if (flag == 1) { // flag is 0
                if (nega)
                    while (slen < width - 1 && ptr > buf + 1) { *--ptr = '0'; slen++; }
                else
                    while (slen < width && ptr > buf + 1) { *--ptr = '0'; slen++; }
            }

            if (nega) { *--ptr = '-'; slen++; }
            else if (flag == 3) { *--ptr = '+';  slen++; } //flag is +

            wmlen += fmt_mem_cpy(pdst, dstlen - wmlen, ptr, slen, width, flag == 2);
            fplen += fmt_file_cpy(fp, ptr, slen, width, flag == 2);
            continue;

        case 'e':
        case 'E':
            fval = va_arg(ap, double);
            if (prec < 0) prec = 6;

            if (fval < 0) {
                nega = 1; fval = -fval;
            }

            expval = 0;
            expnega = 0;

            if (fval >= 10.0) {
                expnega = 0;
                for (expval = 0; fval >= 10.0; expval++) fval /= 10.0;
            } else if (fval < 1.0) {
                expnega = 1;
                for (expval = 0; fval < 1.0; expval++) fval *= 10.0;
            }

            ulval = (uint64)fval;

            if (prec > 0) {
                uint64  scale = 1;

                for (ival = 0; ival < prec; ival++) scale *= 10;
                ulval2 = (uint64)( ((long double)fval - (long double)ulval) * scale + 0.5);
                if (ulval2 == scale) {
                    ulval++; ulval2 = 0;
                    if (ulval >= 10) {
                        ulval /= 10;
                        if (expnega) expval--;
                        else expval++;
                    }
                }
            }

            slen = fmt_conv_numval(buf, sizeof(buf), (uint64)expval, 0, &ptr);
            if (slen < 2) {
                while (slen < 2 && ptr > buf + 3) { *--ptr = '0'; slen++; }
            }
            if (expnega > 0) { *--ptr = '-'; slen++; }  //1.2345667e-04
            else { *--ptr = '+'; slen++; } //1.2345678e+04

            wlen = fmt_conv_numval(buf, ptr - buf, ulval2, 0, &ptr); //2345678'\0'-04
            if (prec >= 0) { // precision field exist
                while (wlen < prec && ptr > buf + 3) { *--ptr = '0'; wlen++; }
            }

            *(ptr + wlen) = *fmt;  //e or E
            slen += wlen + 1;

            wlen = fmt_conv_numval(buf, ptr - buf, ulval, 0, &ptr);
            *(ptr + wlen) = '.';
            slen += wlen + 1;

            if (flag == 1) { // flag is 0
                if (nega)
                    while (slen < width - 1 && ptr > buf + 1) { *--ptr = '0'; slen++; }
                else
                    while (slen < width && ptr > buf + 1) { *--ptr = '0'; slen++; }
            }

            if (nega) { *--ptr = '-'; slen++; }
            else if (flag == 3) { *--ptr = '+';  slen++; } //flag is +

            wmlen += fmt_mem_cpy(pdst, dstlen - wmlen, ptr, slen, width, flag == 2);
            fplen += fmt_file_cpy(fp, ptr, slen, width, flag == 2);
            fmt++;
            continue;

        case 'g':
        case 'G':
            fval = va_arg(ap, double);
            if (prec < 0) prec = 6;
            fmt++;
            break;

        case '%':
        default:
            wmlen += fmt_mem_cpy(pdst, dstlen - wmlen, fmt, 1, 1, 0);
            fplen += fmt_file_cpy(fp, fmt, 1, 1, 0);
            fmt++;
            continue;
        }
    }

    if (fp) return fplen;
    return wmlen;
}

int ksnprintf (void * buf, int len, char * fmt, ...)
{
    int ret = 0;
    va_list vap;

    va_start(vap, fmt);
    ret = kvsnprintf(buf, len, NULL, fmt, vap);
    va_end(vap);

    return ret;
}

int ksprintf (void * buf, char * fmt, ...)
{
    int ret = 0;
    va_list vap;

    va_start(vap, fmt);
    ret = kvsnprintf(buf, -1, NULL, fmt, vap);
    va_end(vap);

    return ret;
}

int kfprintf (FILE * fp, char * fmt, ...)
{
    int ret = 0;
    va_list vap;

    va_start(vap, fmt);
    ret = kvsnprintf(NULL, -1, fp, fmt, vap);
    va_end(vap);

    return ret;
}

int kprintf (char * fmt, ...)
{
    int ret = 0;
    va_list vap;

    va_start(vap, fmt);
    ret = kvsnprintf(NULL, -1, stdout, fmt, vap);
    va_end(vap);

    return ret;
}

