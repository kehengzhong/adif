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

#ifndef _CHARSET_H_
#define _CHARSET_H_ 
    
#ifdef __cplusplus
extern "C" {
#endif 

#define CHARSET_UNKNOWN     0
#define CHARSET_ASCII       1
#define CHARSET_UNICODE     2
#define CHARSET_UTF8        3
#define CHARSET_GB18030     4
#define CHARSET_GBK         5
#define CHARSET_GB2312      6
#define CHARSET_BIG5        7

/* Determine the character set to which a character belongs by comparing or
   looking up a table. Byte stream and length are input parameters, and the
   output result is the byte length of the corresponding character set char.
   Otherwise, 0 is returned, indicating that the current char pointer does not
   belong to the given charset */

int coding_ascii_check   (void * pbyte, int len);
int coding_unicode_check (void * pbyte, int len);
int coding_big5_check    (void * pbyte, int len);
int coding_big5_lookup   (void * pbyte, int len);
int coding_gb2312_lookup (void * pbyte, int len);
int coding_gbk_check     (void * pbyte, int len);
int coding_gb18030_check (void * pbyte, int len);
int coding_utf8_check    (void * pbyte, int len);

int coding_string_trunc (void * psrc, int srclen, void * pdst, int dstlen, int chset);

int coding_charset_scan  (void * pbyte, int len, int * ascii, int * unicode, int * utf8,
                          int * gbk, int * gb2312, int * gb18030, int * big5, int * unknown);

int coding_charset_detect(void * pbyte, int len, int * chset, void * chsetname);

char * coding_charset_name (int chset);

int coding_charset_convert (void * pstr, int len, int maxsize, int dstchset);
int coding_charset_convert_frame(frame_p pfrm, int srcchset, int dstchset);


#ifdef __cplusplus
}
#endif 
 
#endif

