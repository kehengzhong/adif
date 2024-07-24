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

#ifdef HAVE_OPENSSL

#ifndef _SSLTCP_H_
#define _SSLTCP_H_ 

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "tsock.h"

#ifdef __cplusplus
extern "C" {
#endif 

typedef struct ssl_tcp_s {
    uint8         ssllink    : 1;  //0-TCP 1-SSL over TCP
    uint8         handshaked : 1;  
    uint8         contype    : 6;  //0-Connected  1-Accepted

    SOCKET        fd;

    SSL_CTX     * sslctx;
    SSL         * ssl;
} SSLTcp, ssl_tcp_t, *ssl_tcp_p;

int ssl_tcp_init ();

void * ssl_server_ctx_init (char * cert, char * prikey, char * cacert);
void * ssl_client_ctx_init (char * cert, char * prikey, char * cacert);
int    ssl_ctx_free (void * vctx);

int    ssl_servername_select (SSL * ssl, int * ad, void * arg);

void * ssl_tcp_bind      (SOCKET fd, int ssllink, void * vctx, int contype);
int    ssl_tcp_handshake (void * vssltcp, int * perr);
int    ssl_tcp_verify    (void * vssltcp, char * peername);
int    ssl_tcp_close     (void * vssltcp);
int    ssl_tcp_unbind    (void * vssltcp);

void * ssl_tcp_from_ssl (SSL * ssl);

int    ssl_tcp_read   (void * vssltcp, frame_p frm, int * num, int * perr);
int    ssl_tcp_write  (void * vssltcp, void * pbyte, int bytelen, int * num, int * perr);

int    ssl_tcp_writev   (void * vssltcp, void * piov, int iovcnt, int * num, int * perr);
int    ssl_tcp_sendfile (void * vssltcp, int filefd, int64 pos, int64 size, int * num, int * perr);

#ifdef __cplusplus
}
#endif 
 
#endif

#endif

