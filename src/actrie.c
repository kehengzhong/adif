/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#include "btype.h"
#include "memory.h"
#include "mpool.h"
#include "strutil.h"
#include "arfifo.h"
#include "dynarr.h"
#include "actrie.h"

void * acnode_alloc ();
int    acnode_free (void * vnode);
int    acnode_recursive_free (void * vnode);
 
static int acnode_cmp_item (void * a, void * b)
{
    acnode_t * anode = (acnode_t *)a;
    acnode_t * bnode = (acnode_t *)b;

    return anode->ch - bnode->ch;
}

static int acnode_cmp_char (void * vnode, void * vword)
{
    acnode_t * node = (acnode_t *)vnode;
    uint8      ch = *(uint8 *)vword;

    return node->ch - ch;
}

void * acnode_alloc ()
{
    acnode_t * node = NULL;
 
    node = kzalloc(sizeof(*node));
    if (!node) return NULL;
 
    node->alloc = 1;
    return node;
}
 
int acnode_free (void * vnode)
{
    acnode_t * node = (acnode_t *)vnode;
 
    if (!node) return -1;
 
    if (node->sublist) {
        arr_free(node->sublist);
        node->sublist = NULL;
    }

    if (node->alloc) kfree(node);
 
    return 0;
}
 
int acnode_recursive_free (void * vnode)
{
    acnode_t * node = (acnode_t *)vnode;
    acnode_t * subnode = NULL;
 
    if (!node) return -1;
 
    while (arr_num(node->sublist) > 0) {
        subnode = arr_pop(node->sublist);
        acnode_recursive_free(subnode);
    }

    if (arr_num(node->sublist) <= 0) {
        acnode_free(node);
    }
 
    return 0;
}
 
int acnode_init (void * vnode)
{
    acnode_t * node = (acnode_t *)vnode;
 
    if (!node) return -1;
 
    node->ch = 0;
    node->depth = 0;
    node->phrase_end = 0;
    node->alloc = 0;
    node->count = 0;

    if (node->sublist) {
        node->sublist = arr_new(4);
    }
    arr_zero(node->sublist);

    node->parent = NULL;
    node->failjump = NULL;
    node->para = NULL;
 
    return 0;
}

void * acnode_fetch (void * vtrie) 
{
    actrie_t  * trie = (actrie_t *)vtrie;
    acnode_t * node = NULL;

    if (!trie) return NULL;

    node = mpool_fetch(trie->itempool);
    if (!node) node = acnode_alloc();

    return node;
}

int acnode_recycle (void * vtrie, void * vnode)
{
    actrie_t * trie = (actrie_t *)vtrie;
    acnode_t * node = (acnode_t *)vnode;
 
    if (!trie) return -1;
    if (!node) return -2;

    if (node->alloc)
        return acnode_free(node);

    mpool_recycle(trie->itempool, node);
    return 0;
}

void * actrie_init (int entries, void * matchcb, int reverse)
{
    actrie_t * trie = NULL;
 
    trie = kzalloc(sizeof(*trie));
    if (!trie) return NULL;
 
    if (entries < 128) entries = 128;

    trie->entries = entries;
    trie->matchcb = matchcb;
    trie->reverse = reverse;

    trie->itempool = mpool_alloc();
    mpool_set_unitsize(trie->itempool, sizeof(acnode_t));
    mpool_set_allocnum(trie->itempool, entries);
    mpool_set_initfunc(trie->itempool, acnode_init);
    mpool_set_freefunc(trie->itempool, acnode_free);

    trie->root = acnode_fetch(trie);

    return trie;
}
 
int actrie_free (void *vtrie)
{
    actrie_t  * trie = (actrie_t *)vtrie;
 
    if (!trie) return -1;
 
    if (trie->root) {
        acnode_recursive_free(trie->root);
    }

    if (trie->itempool) {
        mpool_free(trie->itempool);
        trie->itempool = NULL;
    }

    kfree(trie);
    return 0;
}
 
 
int actrie_add (void * vtrie, void * vbyte, int bytelen, void * para)
{
    actrie_t    * trie = (actrie_t *)vtrie;
    uint8       * pbyte = (uint8 *)vbyte;
    acnode_t    * node = NULL;
    acnode_t    * subnode = NULL;
    int           i;
    uint8         ch;
    int           depth = 0;
 
    if (!trie) return -1;
    if (!pbyte) return -2;
    if (bytelen < 0) bytelen = str_len(pbyte);
    if (bytelen <= 0) return -3;
 
    if ((node = trie->root) == NULL) {
        trie->root = node = acnode_fetch(trie);
        node->depth = 0;
        node->parent = NULL;
    }
 
    for (depth = 1, i = 0; i < bytelen; i++, depth++) {
        if (!node->sublist)
            node->sublist = arr_new(2);

        ch = trie->reverse ? pbyte[bytelen - 1 - i] : pbyte[i];

        subnode = arr_find_by(node->sublist, &ch, acnode_cmp_char);
        if (!subnode) {
            subnode = acnode_fetch(trie);

            subnode->ch = ch;
            subnode->depth = depth;
            subnode->parent = node;

            arr_insert_by(node->sublist, subnode, acnode_cmp_item);
        }

        subnode->count++;
 
        node = subnode;
        subnode = NULL;
    }
 
    node->phrase_end = 1;
    node->para = para;
 
    return 0;
}
 
int actrie_del (void * vtrie, void * vbyte, int bytelen)
{
    actrie_t    * trie = (actrie_t *)vtrie;
    uint8       * pbyte = (uint8 *)vbyte;
    acnode_t    * node = NULL;
    acnode_t    * subnode = NULL;
    int           i;
    uint8         ch;

    if (!trie) return -1;
    if (!pbyte) return -2;
    if (bytelen < 0) bytelen = str_len(pbyte);
    if (bytelen <= 0) return -3;
 
    node = trie->root;
    if (!node) return -100;
 
    for (i = 0; i < bytelen && arr_num(node->sublist) > 0; i++) {

        ch = trie->reverse ? pbyte[bytelen - 1 - i] : pbyte[i];

        subnode = arr_find_by(node->sublist, &ch, acnode_cmp_char);
        if (!subnode) break;
 
        node = subnode;
    }

    if (i < bytelen) return -10; //not found all words
 
    if (node->phrase_end)
        node->phrase_end = 0;

    while (node && node->depth > 0) {
        subnode = node;
        node = (acnode_t *)subnode->parent;

        subnode->count--;

        if (subnode->count <= 0 && arr_num(subnode->sublist) <= 0) {
            arr_delete_by(node->sublist, subnode, acnode_cmp_item);
            acnode_recycle(trie, subnode);
        }
    }

    return 0;
}
 
int actrie_failjump (void * vtrie)
{
    actrie_t    * trie = (actrie_t *)vtrie;
    acnode_t    * root = NULL;
    acnode_t    * node = NULL;
    acnode_t    * subnode = NULL;
    acnode_t    * failnode = NULL;
    acnode_t    * jmpnode = NULL;
    void        * fifo = NULL;
    int           i = 0;

    if (!trie) return -1;

    if (trie->build_failjump) return 0;

    fifo = ar_fifo_new(4);

    root = trie->root;
    root->failjump = NULL;

    ar_fifo_push(fifo, root);

    while (ar_fifo_num(fifo) > 0) {
        node = ar_fifo_out(fifo);

        for (i = 0; i < arr_num(node->sublist); i++) {
            subnode = arr_value(node->sublist, i);
            if (!subnode) continue;

            if (node == root) {
                subnode->failjump = root;

            } else {
                /* get the failjump node of subnode's parent */
                failnode = node->failjump;

                while (failnode != NULL) {
                    /* find the subnode of fail-node that has same 'ch' as the subnode */
                    jmpnode = arr_find_by(failnode->sublist, subnode, acnode_cmp_item);
                    if (jmpnode) {
                        subnode->failjump = jmpnode;
                        break;
                    }

                    failnode = failnode->failjump;
                }

                if (failnode == NULL) {
                    subnode->failjump = root;
                }
            }

            ar_fifo_push(fifo, subnode);
        }
    }

    ar_fifo_free(fifo);

    trie->build_failjump = 1;

    return 1;
}

int actrie_get (void * vtrie, void * vbyte, int bytelen, void ** pvar)
{
    actrie_t    * trie = (actrie_t *)vtrie;
    uint8       * pbyte = (uint8 *)vbyte;
    acnode_t    * node = NULL;
    acnode_t    * subnode = NULL;
    int           i;
    uint8         ch;

    if (!trie) return 0;
    if (!pbyte) return 0;
    if (bytelen < 0) bytelen = str_len(pbyte);
    if (bytelen <= 0) return 0;

    node = trie->root;
    if (!node) return 0;

    for (i = 0; i < bytelen && arr_num(node->sublist) > 0; i++) {

        ch = trie->reverse ? pbyte[bytelen - 1 - i] : pbyte[i];

        subnode = arr_find_by(node->sublist, &ch, acnode_cmp_char);
        if (!subnode) break;
 
        node = subnode;
    }
 
    while (node && node->depth > 1 && !node->phrase_end) {
        node = (acnode_t *)node->parent;
    }
 
    if (node && node->phrase_end) {
        if (pvar) *pvar = node->para;
        return node->depth;
    }
 
    return 0;
}
 

int actrie_match (void * vtrie, void * vbyte, int len, void ** pres, int * reslen, void ** pvar)
{
    actrie_t   * trie = (actrie_t *)vtrie;
    uint8      * pbyte = (uint8 *)vbyte;
    acnode_t   * node = NULL;
    acnode_t   * subnode = NULL;
    acnode_t   * iter = NULL;
    int          i;
    uint8        ch;
    int          mat = 0;
 
    if (pres) *pres = NULL;
    if (reslen) *reslen = 0;
 
    if (!trie) return -1;
    if (!pbyte) return -2;
    if (len < 0) len = str_len(pbyte);
    if (len <= 0) return -3;
 
    node = trie->root;
    if (!node) return -100;

    if (!trie->build_failjump)
        actrie_failjump(trie);

    for (i = 0; i < len; i++) {

        ch = trie->reverse ? pbyte[len - 1 - i] : pbyte[i];

        subnode = arr_find_by(node->sublist, &ch, acnode_cmp_char);
        if (subnode) {
            node = subnode;

            for (iter = node; iter != trie->root; ) {
                if (iter->phrase_end > 0 && trie->matchcb) {
                    (*trie->matchcb)(iter->para,
                                     pbyte, len,
                                     trie->reverse ? pbyte + len - 1 - i : pbyte + i - iter->depth + 1,
                                     iter->depth);
                    mat++;
                }

                iter = iter->failjump;
            }

        } else {
            if (node == trie->root) {
                continue;
            }

            node = node->failjump;
            i--;
        }
    }

    return mat ? mat : -200;
}


int actrie_fwmaxmatch (void * vtrie, void * vbyte, int len, void ** pres, int * reslen, void ** pvar)
{
    actrie_t   * trie = (actrie_t *)vtrie;
    uint8      * pbyte = (uint8 *)vbyte;
    int          i;
    int          ret = 0;

    if (pres) *pres = NULL;
    if (reslen) *reslen = 0;

    if (!trie) return -1;
    if (!pbyte) return -2;
    if (len < 0) len = str_len(pbyte);
    if (len <= 0) return -3;

    for (i = 0; i < len; i++) {

        if (trie->reverse) {
            ret = actrie_get(trie, pbyte, len - i, pvar);

            if (ret > 0) { //matched one words or phrase
                if (pres) *pres = pbyte + len - 1 - i;
                if (reslen) *reslen = ret;
                return len - 1 - i;
            }

        } else {
            ret = actrie_get(trie, pbyte + i, len - i, pvar);

            if (ret > 0) { //matched one words or phrase
                if (pres) *pres = pbyte + i;
                if (reslen) *reslen = ret;
                return i;
            }
        }
    }

    return -100;
}

