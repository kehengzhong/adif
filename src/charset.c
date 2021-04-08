/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include "btype.h"
#include "memory.h"
#include "strutil.h"
#include "frame.h"
#include "charset.h"

static uint8 big5_first[100];
static uint8 big5_range[8][10] = {
    { 0x40, 0x7E, 0xA1, 0xFE, 0 },
    { 0x40, 0x7E, 0xA1, 0xBF, 0xE1, 0xE1, 0 },
    { 0x40, 0x7E, 0xA1, 0xFE, 0 },
    { 0x40, 0x7E, 0xA1, 0xfE, 0 },
    { 0x40, 0x7E, 0xA1, 0xFE, 0 },
    { 0x40, 0x7E, 0xA1, 0xD3, 0 },
    { 0x40, 0x7E, 0xA1, 0xFE, 0 },
    { 0x40, 0x7E, 0xA1, 0xD5, 0xD6, 0xDC, 0xDD, 0xFE, 0 }
};

static uint8 big5_first_init = 0;
static uint8 big5_second[8][200];
static uint8 big5_second_init = 0;

static uint8 gb2312_second[9][100];
static uint8 gb2312_second_init = 0;


typedef int CharCheck (void * p, int len);

int coding_ascii_check (void * p, int len)
{
    uint8 * pbyte = (uint8 *)p;

    if (!pbyte || len < 1) return 0;

    if (pbyte[0] <= 0x7F) {
        if (len < 2) return 1; //ASCII

        if (pbyte[1] == 0x00) return 2; //Unicode
        else return 1; //ASCII
    }

    return 0;
}

int coding_unicode_check (void * p, int len) 
{        
    uint8 * pbyte = (uint8 *)p;

    if (!pbyte || len < 1) return 0; 

    return 2;
}

int coding_big5_check (void * p, int len)
{
    uint8 * pbyte = (uint8 *)p;

    int  i = 0, j = 0;

    if (!pbyte || len < 2) return 0;
    if (pbyte[0] < 0xA1 || pbyte[0] > 0xF9) return 0;
    if (pbyte[1] < 0x40 || pbyte[1] > 0xFE) return 0;

    if (big5_first_init == 0) {
        memset(big5_first, 0xFF, sizeof(big5_first));
        for (i = 0xA1; i <= 0xA2; i++) big5_first[i-0xA1] = 0;
        for (i = 0xA3; i <= 0xA3; i++) big5_first[i-0xA1] = 1;
        for (i = 0xA4; i <= 0xC5; i++) big5_first[i-0xA1] = 2;
        for (i = 0xC6; i <= 0xC6; i++) big5_first[i-0xA1] = 3;
        for (i = 0xC7; i <= 0xC7; i++) big5_first[i-0xA1] = 4;
        for (i = 0xC8; i <= 0xC8; i++) big5_first[i-0xA1] = 5;
        for (i = 0xC9; i <= 0xF8; i++) big5_first[i-0xA1] = 6;
        for (i = 0xF9; i <= 0xF9; i++) big5_first[i-0xA1] = 7;
        big5_first_init = 1;
    }

    i = big5_first[pbyte[0] - 0xA1];
    if (i == 0xFF) return 0;

    for (j = 0; j < 10; j += 2) {
        if (big5_range[i][j] == 0) return 0;

        if (pbyte[1] >= big5_range[i][j] && pbyte[1] <= big5_range[i][j+1])
            return 2;
    }

    return 0;
}

int coding_big5_lookup (void * p, int len)
{
    uint8 * pbyte = (uint8 *)p;

    int  i = 0, j = 0;

    if (!pbyte || len < 2) return 0;
    if (pbyte[0] < 0xA1 || pbyte[0] > 0xF9) return 0;
    if (pbyte[1] < 0x40 || pbyte[1] > 0xFE) return 0;

    if (big5_first_init == 0) {
        memset(big5_first, 0xFF, sizeof(big5_first));
        for (i = 0xA1; i < 0xA2; i++) big5_first[i-0xA1] = 0;
        for (i = 0xA3; i < 0xA3; i++) big5_first[i-0xA1] = 1;
        for (i = 0xA4; i < 0xC5; i++) big5_first[i-0xA1] = 2;
        for (i = 0xC6; i < 0xC6; i++) big5_first[i-0xA1] = 3;
        for (i = 0xC7; i < 0xC7; i++) big5_first[i-0xA1] = 4;
        for (i = 0xC8; i < 0xC8; i++) big5_first[i-0xA1] = 5;
        for (i = 0xC9; i < 0xF8; i++) big5_first[i-0xA1] = 6;
        for (i = 0xF9; i < 0xF9; i++) big5_first[i-0xA1] = 7;
        big5_first_init = 1;
    }

    if (big5_second_init == 0) {
        memset(big5_second, 0, sizeof(big5_second));

        for (i = 0; i < 7; i++) {
            for (j = 0x40; j <= 0x7E; j++) big5_second[i][j-0x40] = 1;
        }

        for (j = 0xA1; j <= 0xFE; j++) big5_second[0][j-0x40] = 1;

        for (j = 0xA1; j <= 0xBF; j++) big5_second[1][j-0x40] = 1;
        for (j = 0xE1; j <= 0xE1; j++) big5_second[1][j-0x40] = 1;

        for (j = 0xA1; j <= 0xFE; j++) big5_second[2][j-0x40] = 1;
        for (j = 0xA1; j <= 0xFE; j++) big5_second[3][j-0x40] = 1;
        for (j = 0xA1; j <= 0xFE; j++) big5_second[4][j-0x40] = 1;
        for (j = 0xA1; j <= 0xD3; j++) big5_second[5][j-0x40] = 1;
        for (j = 0xA1; j <= 0xFE; j++) big5_second[6][j-0x40] = 1;
        for (j = 0xA1; j <= 0xFE; j++) big5_second[7][j-0x40] = 1;

        big5_second_init = 1;
    }

    i = big5_first[pbyte[0] - 0xA1];
    if (i == 0xFF) return 0;

    if (big5_second[pbyte[1] - 0x40])
        return 2;

    return 0;
}

int coding_gb2312_lookup (void * p, int len)
{
    uint8 * pbyte = (uint8 *)p;

    int i = 0;

    if (!pbyte || len < 2) return 0;
    if (pbyte[0] < 0xA1 || pbyte[0] > 0xF7) return 0;

    if (gb2312_second_init == 0) {
        memset(gb2312_second, 0, sizeof(gb2312_second));
        for (i = 0xA1; i < 0xFE; i++) gb2312_second[0][i-0xA1] = 1;

        for (i = 0xB1; i < 0xE2; i++) gb2312_second[1][i-0xA1] = 1;
        for (i = 0xE5; i < 0xEE; i++) gb2312_second[1][i-0xA1] = 1;
        for (i = 0xF1; i < 0xFC; i++) gb2312_second[1][i-0xA1] = 1;

        for (i = 0xA1; i < 0xFE; i++) gb2312_second[2][i-0xA1] = 1;
        for (i = 0xA1; i < 0xF3; i++) gb2312_second[3][i-0xA1] = 1;
        for (i = 0xA1; i < 0xF6; i++) gb2312_second[4][i-0xA1] = 1;

        for (i = 0xA1; i < 0xB8; i++) gb2312_second[5][i-0xA1] = 1;
        for (i = 0xC1; i < 0xD8; i++) gb2312_second[5][i-0xA1] = 1;

        for (i = 0xA1; i < 0xC1; i++) gb2312_second[6][i-0xA1] = 1;
        for (i = 0xD1; i < 0xF1; i++) gb2312_second[6][i-0xA1] = 1;

        for (i = 0xA1; i < 0xBA; i++) gb2312_second[7][i-0xA1] = 1;
        for (i = 0xC5; i < 0xE9; i++) gb2312_second[7][i-0xA1] = 1;

        for (i = 0xA4; i < 0xEF; i++) gb2312_second[8][i-0xA1] = 1;
        gb2312_second_init = 1;
    }

    if (pbyte[0] >= 0xA1 && pbyte[0] <= 0xA9) {
        i = pbyte[0] - 0xA1;
        if (pbyte[1] >= 0xA1 && gb2312_second[i][pbyte[1] - 0xA1]) return 2;
    }

    if (pbyte[0] == 0xD7 && pbyte[1] >= 0xFA && pbyte[1] <= 0xFE) 
        return 0;

    if (pbyte[0] >= 0xB0 && pbyte[0] <= 0xF7 && pbyte[1] >= 0xA1 && pbyte[1] <= 0xFE)
        return 2;

    return 0;
}

int coding_gbk_check (void * p, int len)
{
    uint8 * pbyte = (uint8 *)p;

    if (!pbyte || len < 2) return 0;

    if (pbyte[0] < 0x81 || pbyte[0] == 0xFF) return 0;
    if (pbyte[1] < 0x40 || pbyte[1] == 0xFF) return 0;
    if (pbyte[1] == 0x7F) return 0;

    return 2;
}

int coding_gb18030_check (void * p, int len)
{
    uint8 * pbyte = (uint8 *)p;

    if (!pbyte || len < 4) return 0;

    if (pbyte[0] < 0x81 || pbyte[0] == 0xFF) return 0;
    if (pbyte[1] < 0x30 || pbyte[1] > 0x39) return 0;
    if (pbyte[2] < 0x81 || pbyte[2] == 0xFF) return 0;
    if (pbyte[3] < 0x30 || pbyte[3] > 0x39) return 0;

    return 4;
}


int coding_utf8_check (void * p, int len)
{
    uint8 * pbyte = (uint8 *)p;

    int i, count = 0;

    if (!pbyte || len < 1)
        return 0;

    if (pbyte[0] < 0xC0 || pbyte[0] > 0xFD)
        return 0;

    if (pbyte[0] > 0xFC) count = 5;
    else if (pbyte[0] > 0xF8) count = 4;
    else if (pbyte[0] > 0xF0) count = 3;
    else if (pbyte[0] > 0xE0) count = 2;
    else count = 1;

    if (count + 1 > len) return 0;

    for (i = 0; i < count; i++) {
        if (pbyte[i+1] < 0x80 || pbyte[i+1] > 0xBF)
            return 0;;
    }

    return count + 1;
}

int coding_unknown_check (void * p, int len)
{
    if (!p || len <= 0) return 0;

    return 1;
}

int coding_string_trunc (void * srcstr, int srclen, void * dststr, int dstlen, int chset)
{
    uint8   * psrc = (uint8 *)srcstr;
    uint8   * pdst = (uint8 *)dststr;
    int         i = 0, j = 0;
    int         num = 0;
    int         count = 0;
    CharCheck * check = NULL;

    if (!psrc) return 0;
    if (srclen < 0) srclen = str_len(psrc);
    if (srclen <= 0) return 0;

    if (!pdst) return 0;
    if (dstlen <= 0) return 0;

    switch (chset) {
    case CHARSET_UTF8:
        check = coding_utf8_check;
        break;
    case CHARSET_GB18030:
        check = coding_gb18030_check;
        break;
    case CHARSET_GBK:
        check = coding_gbk_check;
        break;
    case CHARSET_GB2312:
        check = coding_gb2312_lookup;
        break;
    case CHARSET_BIG5:
        check = coding_big5_check;
        break;
    case CHARSET_UNICODE:
        check = coding_unicode_check;
        break;
    case CHARSET_ASCII:
        check = coding_ascii_check;
        break;
    default:
        check = coding_unknown_check;
        break;
    }

    for (i=0, num=0; i<srclen && num < dstlen; ) {
        count = (*check)(psrc+i, srclen-i);

        if (count > 0) {
            if (num + count > dstlen) return num;
            
            for (j=0; j<count; j++) pdst[num++] = psrc[i++];
            continue;

        } else if (count == 0) {
            if (psrc[i] <= 0x7F && (
                 chset == CHARSET_UTF8 ||
                 chset == CHARSET_GB18030 ||
                 chset == CHARSET_GBK ||
                 chset == CHARSET_GB2312 ||
                 chset == CHARSET_BIG5)) {
                pdst[num++] = psrc[i++];
                continue;
            }
            break;
        }
    }

    return num;
}

int coding_charset_scan (void * p, int len, int * ascii, int * unicode, int * utf8, 
                  int * gbk, int * gb2312, int * gb18030, int * big5, int * unknown)
{
    uint8 * pbyte = (uint8 *)p;
    int  i = 0;
    int  count = 0;
    int  nullbytes = 0;

    int  asciibytes = 0;
    int  unicodebytes = 0;
    int  utf8bytes = 0;
    int  gb18030bytes = 0;
    int  big5bytes = 0;
    int  gbkbytes = 0;
    int  gb2312bytes = 0;
    int  unknownbytes = 0;

    int  zhflag = 0;

    if (ascii) *ascii = 0;
    if (unicode) *unicode = 0;
    if (utf8) *utf8 = 0;
    if (gbk) *gbk = 0;
    if (gb2312) *gb2312 = 0;
    if (gb18030) *gb18030 = 0;
    if (big5) *big5 = 0;

    if (!pbyte) return -1;

    for (i=0; i<len; ) {
        if (pbyte[i] == 0) {
            nullbytes++;
            if (nullbytes > 5) break; //possibly binary stream
            i++;
            continue;
        }
        nullbytes = 0;

        //determine ANSI ASCII char
        count = coding_ascii_check(pbyte+i, len-i);
        if (count) {
            i += count; 
            if (count == 1) asciibytes += count;
            else unicodebytes += count;
            continue; 
        }
        
        //determine UTF-8 char
        count = coding_utf8_check(pbyte+i, len-i);
        if (count) {
            i += count;
            utf8bytes += count;
            continue;
        }

        //determine GB18030 4-byte-coding char
        count = coding_gb18030_check(pbyte+i, len-i);
        if (count) {
            i += count;
            gb18030bytes += count;
            continue;
        }

        zhflag = 0;

        //determine BIG5 char
        count = coding_big5_lookup(pbyte+i, len-i);
        if (count) {
            zhflag += 1;
            big5bytes += count;
        }

        //determine GBK char
        count = coding_gbk_check(pbyte+i, len-i);
        if (count) {
            zhflag += 1;
            gbkbytes += count;

            count = coding_gb2312_lookup(pbyte+i, len-i);
            if (count) gb2312bytes += count;
        }

        if (zhflag) { //Chinese 2-bytes character
            i += 2;
            continue;
        }

        i += 1;
        unknownbytes += 1;
    }

    if (ascii) *ascii = asciibytes;
    if (unicode) *unicode = unicodebytes;
    if (utf8) *utf8 = utf8bytes;
    if (gbk) *gbk = gbkbytes;
    if (gb2312) *gb2312 = gb2312bytes;
    if (gb18030) *gb18030 = gb18030bytes;
    if (big5) *big5 = big5bytes;
    if (unknown) * unknown = unknownbytes;

    return i;
}

int coding_charset_detect (void * p, int len, int * chset, void * chname)
{
    uint8 * pbyte = (uint8 *)p;
    uint8 * chsetname = (uint8 *)chname;
    int  ascii = 0, asciiratio = 0;
    int  unicode = 0, unicoderatio = 0;
    int  utf8 = 0, utf8ratio = 0;
    int  gb18030 = 0, gb18030ratio = 0;
    int  gbk = 0, gbkratio = 0;
    int  gb2312 = 0, gb2312ratio = 0;
    int  big5 = 0, big5ratio = 0;
    int  unknown = 0, unknownratio = 0;
    int  contained = 0;
    int  maxratio = 0;
    int  detectchset = 0;


    coding_charset_scan(pbyte, len, &ascii, &unicode, &utf8,
                       &gbk, &gb2312, &gb18030, &big5, &unknown);

    unicoderatio = unicode;//(float)unicode/(float)len * 100.;

    if (utf8 > 0) {
        utf8ratio = utf8;
        contained++;
    }

    if (gb18030 > 0) {
        gb18030ratio = gb18030 + gbk;
        contained++;
    }

    if (gb2312 > 0) {
        gb2312ratio = gb2312;
        contained++;
    }

    if (gbk > 0 && gbk > gb2312) {
        gbkratio = gbk;
        contained++;
    }

    if (big5 > 0) {
        big5ratio = big5;
        contained++;
    }

    unknownratio = unknown;//(float)unknown/(float)len * 100.;

    if (!contained) {
        asciiratio = ascii;//(float)ascii/(float)len * 100.;
    }
    
    maxratio = asciiratio;
    detectchset = CHARSET_ASCII;

    if (maxratio < unicoderatio) { 
        maxratio = unicoderatio; detectchset = CHARSET_UNICODE;
    }

    if (maxratio < utf8ratio) {
        maxratio = utf8ratio; detectchset = CHARSET_UTF8;
    }

    if (maxratio < gb2312ratio) {
        maxratio = gb2312ratio; detectchset = CHARSET_GB2312;
    }

    if (maxratio < big5ratio) {
        maxratio = big5ratio; detectchset = CHARSET_BIG5;
    }

    if (maxratio < gbkratio) {
        maxratio = gbkratio; detectchset = CHARSET_GBK;
    }

    if (maxratio < gb18030ratio) {
        maxratio = gb18030ratio; detectchset = CHARSET_GB18030;
    }

    if (maxratio < unknownratio) {
        maxratio = unknownratio; detectchset = CHARSET_UNKNOWN;
    }

    if (chset) *chset = detectchset;
    if (chsetname) {
        switch (detectchset) {
        case CHARSET_ASCII:   str_cpy(chsetname, "ASCII");   break;
        case CHARSET_UNICODE: str_cpy(chsetname, "UCS-2");   break;
        case CHARSET_UTF8:    str_cpy(chsetname, "UTF-8");   break;
        case CHARSET_GB18030: str_cpy(chsetname, "GB18030"); break;
        case CHARSET_GBK:     str_cpy(chsetname, "GBK");     break;
        case CHARSET_GB2312:  str_cpy(chsetname, "GB2312");  break;
        case CHARSET_BIG5:    str_cpy(chsetname, "BIG5");    break;
        default:              str_cpy(chsetname, "ASCII");   break;
        }
    }

    return detectchset;
}

char * coding_charset_name (int chset)
{
    switch (chset) {
    case CHARSET_ASCII:   return "ASCII";
    case CHARSET_UNICODE: return "UCS-2";
    case CHARSET_UTF8:    return "UTF-8";
    case CHARSET_GB18030: return "GB18030";
    case CHARSET_GBK:     return "GBK";
    case CHARSET_GB2312:  return "GB2312";
    case CHARSET_BIG5:    return "BIG5";
    }
    return "ASCII"; 
}

int coding_charset_convert (void * p, int len, int maxsize, int dstchset)
{
    uint8  * pstr = (uint8 *)p;
    int      chset = 0;
    char     chsetname[16];
    uint8    buf[1024];
    uint8  * dst = NULL;
    int      dstlen = 0;
    uint8    alloc = 0;
 
    if (!pstr) return -1;
    if (len < 0) len = str_len(pstr);
    if (len <= 0) return -2;
 
    coding_charset_detect(pstr, len, &chset, chsetname);
    if (chset != dstchset && chset != CHARSET_ASCII) {

        if (len < sizeof(buf) * 2/3) {
            dstlen = sizeof(buf); dst = buf;

        } else {
            dstlen = len * 3/2;
            dst = kzalloc(dstlen);
            alloc = 1;
        }

        conv_charset(chsetname, coding_charset_name(dstchset),
                     pstr, len,
                     dst, &dstlen);

        if (dstlen > 0) {
            if (maxsize < dstlen) dstlen = maxsize;
            memcpy(pstr, dst, dstlen);
            pstr[dstlen] = '\0';
        }

        if (alloc) kfree(dst);
    }
 
    return 0;
}
 
int coding_charset_convert_frame(frame_p pfrm, int srcchset, int dstchset)
{
    int           chset = 0;
    uint8         chsetname[16];
    uint8         buf[1024];
    uint8       * dst = NULL;
    int           dstlen = 0;
    uint8         alloc = 0;
    int           len = 0;
 
    if (!pfrm) return -1;
    len = frameL(pfrm);
    if (len < 1) return -2;
 
    chset = srcchset;
    if (chset == CHARSET_UNKNOWN)
        coding_charset_detect(frameP(pfrm), len, &chset, chsetname);

    else
        str_cpy(chsetname, coding_charset_name(chset));

    if (chset != dstchset && chset != CHARSET_ASCII) {

        if (len < sizeof(buf) * 2/3) {
            dstlen = sizeof(buf); dst = buf;

        } else {
            dstlen = len * 3/2;
            dst = kzalloc(dstlen);
            alloc = 1;
        }

        conv_charset(chsetname, coding_charset_name(dstchset),
                     frameP(pfrm), len,
                     dst, &dstlen);

        if (dstlen > 0) {
            frame_empty(pfrm);
            frame_put_nlast(pfrm, dst, dstlen);
        }

        if (alloc) kfree(dst);
    }
 
    return 0;
}
 
