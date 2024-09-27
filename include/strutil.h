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
void   ckstr_free    (void * str);
void * ckstr_new     (void * pbyte, int bytelen);
int    ckstr_cmp     (void * a, void * b);
int    ckstr_casecmp (void * a, void * b);
 
#if defined(_WIN32) || defined(_WIN64)
void ansi_2_unicode(wchar_t * wcrt, char * pstr);
#endif

int    toHex (int ch, int upercase);

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
int str_atol (void * p, int len, long * pval);
int str_atoll (void * p, int len, int64 * pval);
int str_atou (void * p, int len, uint32 * pval);
int str_atoul (void * p, int len, ulong * pval);
int str_atoull (void * p, int len, uint64 * pval);
int str_hextoi (void * p, int len, int * pval);
int str_hextou (void * p, int len, uint32 * pval);
 
/* convert string to integer, base 10 or 16, decimal or heximal */
int str_to_int (void * pbyte, int bytelen, int base, void ** pterm);
 
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
 
int  secure_memcpy (void * dst, int dstlen, void * src, int srclen);
long sec_memcpy (void * dst, void * src, long len);
 
int    string_escape    (void * p, int len, void * escch, int chlen, void * pdst, int dstlen);
int    string_strip     (void * p, int len, void * escch, int chlen, void * pdst, int dstlen);
void * string_strip_dup (void * pbyte, int bytelen, void * escch, int chlen);
 
int    json_escape    (void * psrc, int size, void * pdst, int dstlen);
int    json_strip     (void * psrc, int size, void * pdst, int dstlen);
void * json_strip_dup (void * psrc, int size);
 
int uri_uncoded_char (void * psrc, int size);

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
 
/* formatted output conversion functions 
   supported formats as following:
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

int kprintf (char * fmt, ...);
int kfprintf (FILE * fp, char * fmt, ...);
int ksprintf (void * buf, char * fmt, ...);
int ksnprintf (void * buf, int len, char * fmt, ...);

#ifdef __cplusplus
}
#endif
 
#endif

