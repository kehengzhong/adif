
#ifndef _HTTP_COOKIE_H_
#define _HTTP_COOKIE_H_

#include "actrie.h"
#include "hashtab.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct http_cookie_ {
    uint8     alloctype;//0-default kalloc/kfree 1-os-specific malloc/free 2-kmempool alloc/free 3-kmemblk alloc/free
    void    * mpool;

    /* do not contain following characters:
        CTRLs Space \t ( ) < > @ , ; : \ " /  [ ] ? = { } */
    char    * name;
    int       namelen;

    /* do not contain following characters:
       CTRLs Space " , ; \ */
    char    * value;
    int       valuelen;

    char    * path;
    int       pathlen;

    char    * domain;
    int       domainlen;

    time_t    expire;
    int       maxage;

    uint8     httponly;
    uint8     secure;

    uint8     samesite;  //0-unknown  1-Strict  2-Lax

    time_t    createtime;
    void    * ckpath;

} HTTPCookie, cookie_t;

void * http_cookie_alloc (int alloctype, void * mpool);
void   http_cookie_free  (void * vckie);


typedef struct cookie_path_ {
    uint8     alloctype;//0-default kalloc/kfree 1-os-specific malloc/free 2-kmempool alloc/free 3-kmemblk alloc/free
    void    * mpool;

    char       path[128];
    arr_t    * cookie_list;

} cookie_path_t, CookiePath;

void * cookie_path_alloc (int alloctype, void * mpool);
void   cookie_path_free  (void * vpath);

typedef struct cookie_domain_ {

    uint8     alloctype;//0-default kalloc/kfree 1-os-specific malloc/free 2-kmempool alloc/free 3-kmemblk alloc/free
    void    * mpool;

    char         domain[128];
    actrie_t   * cookie_path_trie;
    arr_t      * cookie_path_list;

} cookie_domain_t, CookieDomain;

void * cookie_domain_alloc (int alloctype, void * mpool);
void   cookie_domain_free  (void * vdomain);

typedef struct http_cookie_mgmt {

    uint8     alloctype;//0-default kalloc/kfree 1-os-specific malloc/free 2-kmempool alloc/free 3-kmemblk alloc/free
    void    * mpool;

    /* reverse multi-pattern matching based on domain */
    CRITICAL_SECTION   cookieCS;
    actrie_t         * domain_trie;
    hashtab_t        * domain_table;

    /* all cookies list under different domains and paths */
    arr_t            * cookie_list;

    char             * cookie_file;

} CookieMgmt, cookie_mgmt_t;

void * cookie_mgmt_alloc (char * ckiefile, int allctype, void * mpool);
void   cookie_mgmt_free (void * vmgmt);

int    cookie_mgmt_read  (void * vmgmt, char * cookiefile);
int    cookie_mgmt_write (void * vmgmt, char * cookiefile);

int    cookie_mgmt_scan (void * vmgmt);

int    cookie_mgmt_add (void * vmgmt, void * vckie);

void * cookie_mgmt_get (void * vmgmt, char * domain, int domainlen,
                         char * path, int pathlen, char * ckiename, int ckienlen);
int    cookie_mgmt_mget(void * vmgmt, char * domain, int domainlen, char * path, int pathlen, arr_t ** cklist);

int    cookie_mgmt_set (void * vmgmt, char * ckname, int cknlen, char * ckvalue, int ckvlen,
                        char * domain, int domainlen, char * path, int pathlen, time_t expire,
                        int maxage, uint8 httponly, uint8 secure, uint8 samesite);

int    cookie_mgmt_parse (void * vmgmt, char * setcookie, int len, char * defdom, int defdomlen, int needwrite);

int    http_cookie_add       (void * vckmgmt, void * vmsg);
int    http_set_cookie_parse (void * vckmgmt, void * vmsg, void * pbyte, int bytelen);


#ifdef __cplusplus
}
#endif

#endif


