/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#ifndef _AC_TRIE_H_
#define _AC_TRIE_H_

#ifdef __cplusplus
extern "C" {
#endif      

/* implementation of Aho-Corasick multi-pattern matching algorithm, base Trie data-structure */

typedef int ACSucc (void * para, void * p, int len, void * matpos, int patlen);

typedef struct acnode_ {
    unsigned   ch:8;

    unsigned   depth:8;
    unsigned   phrase_end:1;
    unsigned   alloc:1;
    unsigned   count:14;
 
    arr_t    * sublist;

    void     * parent;
    void     * failjump;

    void     * para;

} acnode_t, *acnode_p;

typedef struct actrie_ {

    acnode_t  * root;

    void      * itempool;

    ACSucc    * matchcb;

    unsigned    entries        : 30;
    unsigned    build_failjump : 1;
    unsigned    reverse        : 1;

} actrie_t, *actrie_p;

void * actrie_init (int entries, void * matchcb, int reverse);
int    actrie_free (void * vac);

int    actrie_add (void * vac, void * p, int len, void * para);
int    actrie_del (void * vac, void * p, int len);

int    actrie_failjump (void * vac);

int    actrie_get (void * vac, void * p, int len, void ** pvar);

int    actrie_match (void * vtrie, void * vbyte, int len, void ** pres, int * reslen, void ** pvar);
int    actrie_fwmaxmatch (void * vtrie, void * vbyte, int len, void ** pres, int * reslen, void ** pvar);

#ifdef __cplusplus
}
#endif
 
#endif

