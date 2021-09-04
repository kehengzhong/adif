/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */
 
#ifndef _STRUTIL_H_
#define _STRUTIL_H_
 
#ifdef __cplusplus
extern "C" {
#endif
 
#define ESCAPE_URI            0
#define ESCAPE_ARGS           1
#define ESCAPE_URI_COMPONENT  2
#define ESCAPE_HTML           3
#define ESCAPE_REFRESH        4
#define ESCAPE_MEMCACHED      5
#define ESCAPE_MAIL_AUTH      6
 
#define ISSPACE(a)  ((a)==' ' || (a)=='\t' || (a)=='\r' || (a)=='\n' || (a)=='\f' || (a)=='\v')
 
/* common key string definition */
typedef struct ckstr_s {
    char    * p;
    int       len;
} ckstr_t;
 
#define ckstr_init(p, len) { p, len }
#define ckstr_null         { NULL, 0}
#define ckstr_set(str, pbyte, bytelen)         \
     (str)->p = pbyte;  (str)->len = bytelen
void ckstr_free(void * str);
void * ckstr_new(void * pbyte, int bytelen);
int ckstr_cmp (void * a, void * b);
int ckstr_casecmp (void * a, void * b);
 
#if defined(_WIN32) || defined(_WIN64)
void ansi_2_unicode(wchar_t * wcrt, char * pstr);
#endif

size_t str_len   (void * p);
 
void * str_cpy   (void * pdst, void * psrc);
void * str_ncpy  (void * pdst, void * psrc, size_t n);
int    str_secpy (void * dst, int dstlen, void * src, int srclen);
 
void * str_cat   (void * s1, const void * s2);
void * str_ncat  (void * s1, const void * s2, size_t n);
void * str_secat (void * dst, int dstlen, void * src, int srclen);
 
void * str_dup   (void * pbyte, int bytelen);
 
int str_cmp      (void * ps1, void * ps2);
int str_ncmp     (void * ps1, void * ps2, int n);
int str_casecmp  (void * ps1, void * ps2);
int str_ncasecmp (void * ps1, void * ps2, size_t n);
 
void * str_str (void * vstr, int len, void * vsub, int sublen);
void * str_str2 (void * vstr, int len, void * vsub, int sublen);
 
/* move the pointer to the first non-space char, and return 
 * the new location pointer */
void * str_trim_head (void * ptrim);
 
/* move the terminated char '\0' to the next location of last 
 * non-space char, then return the original pointer, but the 
 * length of the string may be truncated. */
void * str_trim_tail (void * ptrim);
 
/* remove the space character at both head and tail direction.
 * return new loaction pointer. */
void * str_trim (void * ptrim);
 
int str_tolower (void * pbyte, int bytelen);
int str_toupper (void * pbyte, int bytelen);
 
 
int str_mbi2uint (void * p, int len, uint32 * pmbival);
int str_uint2mbi (uint32 value, void * p);
int str_atoi (void * p, int len, int * pval);
int str_hextoi (void * p, int len, int * pval);
int str_atou (void * p, int len, uint32 * pval);
int str_hextou (void * p, int len, uint32 * pval);
 
/* 2004-12-08 17:45:56+08
   Mon, 24 Feb 2003 03:53:54 GMT       <=== subfmt=0 fmt=0 RFC1123 updated from RFC822
   Saturday, 30-Jun-2007 02:11:10 GMT  <=== subfmt=0 fmt=0 RFC1036 updated from RFC850
   Jan 20 2010 11:10:27                 <=== subfmt=1 fmt=0 */
time_t str_gmt2time (void * p, int timelen, time_t * ptm);
 
/* time format definition 
   RFC1123 - RFC822:  Sun, 06 Nov 1994 08:49:37 GMT
   RFC1036 - RFC850:  Saturday, 30-Jun-2007 02:11:10 GMT  */
int str_time2gmt (time_t * ptm, void * gmtbuf, int len, int fmt);
 
/* 2018-03-21 19:01:21 */
int str_datetime (time_t * ptm, void * dtbuf, int len, int fmt);
 
void * str_rk_find (void * pbyte, int len, void * pattern, int patlen);
 
void * str_find_bytes (void * pstr, int len, void * pat, int patlen);
void * str_rfind_bytes (void * pstr, int pos, void * pat, int patlen);
 
 
/* convert the binary byte stream into ascii format stream, 
 * one byte will be converted into 2 byte, thus the result buffer pascii
 * should be twice than pbin */
int bin_to_ascii (void * pbin, int binlen, void * pascii, int * asclen, int upper);
 
/* convert the ascii stream into binary byte stream. 2 ascii byte will be formed
 * into one binary byte. thus, the pbin buffer may be the half of length of pascii */
int ascii_to_bin (void * pascii, int asciilen, void * pbin, int * binlen);
 
/* convert base64 encoded stream to binary octet stream */
int base64_to_bin (void * pasc, int asclen, void * pbin, int *binlen);
 
/* convert binary octet stream to base64 encoded stream */
int bin_to_base64 (void * pbin, int binlen, void * pasc, int * asclen);
 
/* scan the byte stream untill encountering the given characters, in addition,
 * skip over all characters enclosed by "" */
void * skipQuoteTo (void * pbyte, int len, void * tochars, int num);
 
/* stop to the peer of pair-occured chars, excluding inner embeded pairs, () [] {} <> */
void * skipToPeer (void * p, int len, int leftch, int rightch);
 
/* scan the byte stream untill encountering the given characters */
void * skipTo (void * pbyte, int len, void * tochars, int num);
 
void * skipEscTo (void * pbyte, int len, void * tochars, int num);
 
/* reversely scan the byte stream untill meeting the given characters */
void * rskipTo (void * pbyte, int len, void * tochars, int num);
 
/* skip over the given characters from the byte stream */
void * skipOver (void * pbyte, int len, void * skippedchs, int num);
 
/* reversely skip over the given characters from the byte stream */
void * rskipOver (void * pbyte, int rlen, void * skippedchs, int num);
 
/* get the value pointer: username = 'hellokitty'    password = '#$!7798' */
int str_value_by_key (void * pbyte, int bytelen, void * key, void ** pval, int * vallen);
 
/* extract an integer value from string, base value is 10 or 16, that reprensts decimal or heximal, */
int str_to_int (void * pbyte, int bytelen, int base, void ** pterm);
 
int  secure_memcpy (void * dst, int dstlen, void * src, int srclen);
long sec_memcpy (void * dst, void * src, long len);
 
int    string_escape    (void * p, int len, void * escch, int chlen, void * pdst, int dstlen);
int    string_strip     (void * p, int len, void * escch, int chlen, void * pdst, int dstlen);
void * string_strip_dup (void * pbyte, int bytelen, void * escch, int chlen);
 
int    json_escape    (void * psrc, int size, void * pdst, int dstlen);
int    json_strip     (void * psrc, int size, void * pdst, int dstlen);
void * json_strip_dup (void * psrc, int size);
 
/*  escape type value:
      ESCAPE_URI            0
      ESCAPE_ARGS           1
      ESCAPE_URI_COMPONENT  2
      ESCAPE_HTML           3
      ESCAPE_REFRESH        4
      ESCAPE_MEMCACHED      5
      ESCAPE_MAIL_AUTH      6 */
int uri_encode (void * psrc, int size, void * pdst, int dstlen, int type);
int uri_decode (void * psrc, int size, void * pdst, int dstlen);
 
int html_escape (void * psrc, int size, void * pdst, int dstlen);
 
void * string_trim (void * p, int len, void * trim, int trimlen, int * plen);
 
int string_tokenize (void * p, int len, void * sep, int seplen, void ** plist, int * plen, int arrnum);
 
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
int GetRandStr (void * randstr, int size, int type);
 
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
int conv_charset (void * srcchst, void * dstchst,
                  void * orig, int origlen,
                  void * dest, int * destlen);
 
#ifdef __cplusplus
}
#endif
 
#endif

