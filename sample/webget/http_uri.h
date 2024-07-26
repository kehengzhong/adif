
#ifndef _HTTP_URI_H_
#define _HTTP_URI_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct http_uri_s {

    frame_t   * uri;
    uint8       type;      //0-relative 1-absolute  2-connect uri
    uint8       needfree  : 1;  //0-regular  1-ssl
    uint8       ssl_link  : 5;  //0-regular  1-ssl
    uint8       alloctype : 2;  //0-default kalloc/kfree 1-os-specific malloc/free 2-kmempool alloc/free 3-kmemblk alloc/free

    void      * mpool;

    char      * reluri;
    int         relurilen;

    char      * baseuri;
    int         baseurilen;

    char      * rooturi;
    int         rooturilen;

    char      * scheme;
    int         schemelen;

    char      * host;
    int         hostlen;
    int         port;

    char      * path;
    int         pathlen;

    char      * query;
    int         querylen;

    char      * dir;
    int         dirlen;

    char      * file;
    int         filelen;

    char      * file_base;
    int         file_baselen;

    char      * file_ext;
    int         file_extlen;

} HTTPUri, http_uri_t;

void * http_uri_alloc(int alloctype, void * mpool);
void   http_uri_free (void * vuri);

void   http_uri_init (void * vuri);

int    http_uri_set (void * vuri, char * p, int len, int decode);

int    http_uri_parse (void * vuri, char * paddr, int addrlen);
int    http_uri_path_parse (void * vuri, char * paddr, int addrlen);

char * http_uri_string (void * vuri);

#ifdef __cplusplus
}
#endif

#endif


