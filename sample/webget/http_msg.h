
#ifndef _HTTP_MSG_H_
#define _HTTP_MSG_H_

#include "btype.h"
#include "frame.h"
#include "chunk.h"
#include "http_uri.h"
#include "http_chunk.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HTTPMsg_s {
    HTTPUri     uri;

    uint8       ssllink; //0-http://  1-https://

    char        host[128];
    int         port;

    uint8       meth;    //0-GET 1-POST  2-PUT  3-OPTION  4-CONNECT  5-DELETE  6-HEAD  7-TRACE

    frame_t   * ckiefrm;

    uint8       useproxy;
    char        proxy_host[128];
    int         proxy_port;

    int         req_hdrlen;
    uint8       req_hdrenc;  //0-not encoded  1-encoded
    frame_t   * req_hdrfrm;

    uint8       req_bodytype; //0-no body 1-buffer pointer 2-bytestream body 3-filename body
    char        req_mime[128];
    uint32      req_mimeid;
    uint8       req_httpchunk; //0-content-length 1-chunked
    chunk_t   * req_chunk;

    int         req_sentnum;

    /* response status line, header and body information of HTTP Response */

    uint8       res_hdrgot;
    int         res_hdrlen;
    frame_t   * res_hdrfrm;

    char        res_ver[16];
    int         res_status;

    char        res_mime[64];
    uint32      res_mimeid;
    char        res_ext[16];

    uint8       res_contenc; //0-raw 1-chunk
    uint8       res_keepalive;

    long        res_length;

    char      * res_loc;
    int         res_loclen;

    char        res_file[256];

    http_chunk_t * res_httpchk;

    frame_t   * resfrm;

} HTTPMsg, http_msg_t;


void * http_msg_alloc ();
void   http_msg_free  (void * vreq);
int    http_msg_init  (void * vreq);


int    http_req_init  (void * vmsg);

void * http_req_set_uri   (void * vmsg, char * url, int urllen);
int    http_req_set_meth  (void * vmsg, int meth);
int    http_req_set_proxy (void * vmsg, char * proxy, int proxylen);

int    http_req_clear_body  (void * vmsg);
int    http_req_set_body    (void * vmsg, uint8 bodytype, char * pbyte, int bytelen);
int    http_req_set_contype (void * vmsg, char * ctype, int ctypelen);
int    http_req_set_transenc(void * vmsg, int httpchunk);

int    http_req_encode (void * vmsg);

int    http_req_send (void * vmsg, void * vssltcp);


int    http_resp_init   (void * vmsg);
int    http_resp_decode (void * vmsg, char * pbyte, int bytelen);
int    http_resp_recv   (void * vmsg, void * vssltcp, int * crashed);

#ifdef __cplusplus
}
#endif

#endif


