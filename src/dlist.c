/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "dynarr.h"

typedef struct list_node_st {
    struct list_node_st * prev;
    struct list_node_st * next;
} dnode_t;

typedef struct list_st {
    int       num;
    dnode_t * first;
    dnode_t * last;
} dlist_t;

static dnode_t * getNode (dlist_t * lt, int loc)
{
    dnode_t * node = NULL;
    int i;

    if (lt == NULL)
        return NULL;

    if (loc < 0 || loc > lt->num - 1) return NULL;

    for (i = 0, node = lt->first; i < loc; i++) {
        node = node->next;
    }

    return node;
}

void * lt_get_next (void * node)
{
    dnode_t * ret = NULL;

    if (!node) return NULL;

    ret = (dnode_t *)node;

    return ret->next;
}


void * lt_get_prev (void * node)
{
    dnode_t * ret = NULL;

    if (!node) return NULL;

    ret = (dnode_t *)node;

    return ret->prev;
}


dlist_t * lt_new ()
{
    dlist_t * ret = NULL;

    if ((ret = (dlist_t *) kzalloc (sizeof(dlist_t))) == NULL)
        return NULL;

    ret->num = 0;
    ret->first = NULL;
    ret->last = NULL;

    return ret;
}


dlist_t * lt_dup (dlist_t * lt)
{
    dlist_t * ret = NULL;

    if (!lt) return NULL;
    
    ret = lt_new();
    if (!ret) return NULL;

    ret->num = lt->num;
    ret->first = lt->first;
    ret->last = lt->last;

    return ret;
}


void lt_zero (dlist_t * lt)
{
    if (!lt) return;

    lt->num = 0;
    lt->first = NULL;
    lt->last = NULL;
}


void lt_free (dlist_t * lt)
{
    if (!lt) return;

    kfree (lt);
}

void lt_free_all (dlist_t * lt, int (*func)())
{
    dnode_t * cur = NULL, * old = NULL;
    int i;
    
    if (!lt) return;

    for (i = 0, cur = lt->first; cur && i < lt->num; i++) {
        old = cur;
        cur = cur->next;
        (*func)((void *)old);
    }

    lt_free(lt);
}

int lt_prepend (dlist_t * lt, void * data)
{
    dnode_t * node = NULL;

    if (!lt || !data) return -1;

    node = (dnode_t *)data;
    node->prev = NULL;
    node->next = lt->first;

    if (lt->first) {
        lt->first->prev = node;
        lt->first = node;
        if (lt->first) lt->first->prev = NULL;
    } else {
        lt->first = node;
        lt->last = node;
        if (lt->first) lt->first->prev = NULL;
        if (lt->last) lt->last->next = NULL;
    }

    lt->num++;
    return lt->num;
}

int lt_append (dlist_t * lt, void * data)
{
    dnode_t * node = NULL;

    if (!lt || !data) return -1;

    node = (dnode_t *)data;
    node->prev = lt->last;
    node->next = NULL;

    if (lt->first) {
        lt->last->next = node;
        lt->last = node;
        if (lt->last) lt->last->next = NULL;
    } else {
        lt->first = node;
        lt->last = node;
        if (lt->first) lt->first->prev = NULL;
        if (lt->last) lt->last->next = NULL;
    }

    lt->num++;
    return lt->num;
}

int lt_head_combine (dlist_t * lt, dlist_t ** plist)
{
    dlist_t * list = NULL;

    if (!plist) return -1;

    list = *plist;

    if (!lt || !list) return -1;
        return -1;

    if (list->num == 0) {
        lt_free(list);
        *plist = NULL;
        return lt->num;
    }

    if (lt->first) {
        list->last->next = lt->first;
        lt->first->prev = list->last;
        lt->first = list->first;
        if (lt->first) lt->first->prev = NULL;
    } else {
        lt->first = list->first;
        lt->last = list->last;
        if (lt->first) lt->first->prev = NULL;
        if (lt->last) lt->last->next = NULL;
    }
    lt->num += list->num;

    lt_free(list);
    *plist = NULL;

    return lt->num;
}


int lt_tail_combine (dlist_t * lt, dlist_t ** plist)
{
    dlist_t * list = NULL;

    if (!plist) return -1;

    list = *plist;

    if (!lt || !list) return -1;

    if (list->num == 0) {
        lt_free(list);
        *plist = NULL;
        return lt->num;
    }

    if (lt->first) {
        list->first->prev = lt->last;
        lt->last->next = list->first;
        lt->last = list->last;
        if (lt->last) lt->last->next = NULL;
    } else {
        lt->first = list->first;
        lt->last = list->last;
        if (lt->first) lt->first->prev = NULL;
        if (lt->last) lt->last->next = NULL;
    }
    lt->num += list->num;
    lt_free(list);
    *plist = NULL;

    return lt->num;
}


int lt_tail_combine_nodel (dlist_t * lt, dlist_t * list)
{
    if (!lt || !list) return -1;

    if (list->num == 0) {
        return lt->num;
    }

    if (lt->first) {
        list->first->prev = lt->last;
        lt->last->next = list->first;
        lt->last = list->last;
        if (lt->last) lt->last->next = NULL;
    } else {
        lt->first = list->first;
        lt->last = list->last;
        if (lt->first) lt->first->prev = NULL;
        if (lt->last) lt->last->next = NULL;
    }
    lt->num += list->num;
    lt_zero(list);

    return lt->num;
}

int lt_insert (dlist_t * lt, void * data, int loc)
{
    dnode_t * node = NULL, * cur = NULL;

    if (!lt || !data) return -1;

    if (loc < 0) loc = 0;
    if (loc > lt->num) loc = lt->num;

    if (loc == 0) {
        return lt_prepend (lt, data);
    } else if (loc == lt->num) {
        return lt_append (lt, data);
    }

    cur = getNode(lt, loc);
    if (cur == NULL) return -1;

    node = (dnode_t *)data;
    node->prev = cur->prev;
    node->next = cur;

    cur->prev->next = node;
    cur->prev = node;

    lt->num++;
    return lt->num;
}

int lt_insert_before (dlist_t * lt, void * curData, void * data)
{
    dnode_t * node = NULL, * cur = NULL;

    if (!lt || !curData || !data) return -1;

    cur = (dnode_t *) curData;
    node = (dnode_t *) data;

    if (lt->first == cur)
        return lt_prepend(lt, data);
    
    node->prev = cur->prev;
    node->next = cur;

    cur->prev->next = node;
    cur->prev = node;

    lt->num++;
    return lt->num;
}


int lt_insert_after (dlist_t * lt, void * curData, void * data)
{
    dnode_t * node = NULL, * cur = NULL;

    if (!lt || !curData || !data) return -1;
    
    cur = (dnode_t *) curData;
    node = (dnode_t *) data;

    if (lt->last == cur)
        return lt_append(lt, data);
    
    node->prev = cur;
    node->next = cur->next;

    cur->next->prev = node;
    cur->next = node;

    lt->num++;
    return lt->num;
}

int lt_sort_insert_by (dlist_t * lt, void * data, int (*cmp)(void *, void *))
{
    dnode_t * cur = NULL;
    int       ret = 0, ind = 0;

    if (!lt || !data) return -1;

    cur = (dnode_t *) lt->first;
    
    if (lt->num <= 0 || !cur)
        return lt_append(lt, data);

    if (!cmp) return -2;

    for (ind = 0; ind < lt->num; ind++) {
        ret = (*cmp)(cur, data);
        
        if (ret > 0) return lt_insert_before(lt, cur, data);
        if (ret == 0) return lt_insert_after(lt, cur, data);

        cur = cur->next;
        if (!cur) return lt_append(lt, data);
    }

    return lt_append(lt, data);
}


int lt_insert_list (dlist_t * lt, dlist_t ** plist, int loc)
{
    dlist_t * list = NULL;
    dnode_t * node = NULL;

    if (!plist) return -1;

    list = *plist;

    if (!lt || !list) return -1;

    if (list->num == 0) {
        lt_free(list);
        *plist = NULL;
        return lt->num;
    }
    
    if (loc < 0) loc = 0;
    if (loc > lt->num) loc = lt->num;

    if (loc == 0) {
        return lt_head_combine (lt, plist);
    } else if (loc == lt->num) {
        return lt_tail_combine (lt, plist);
    }

    node = getNode(lt, loc);
    if (!node) return -1;
    
    node->prev->next = list->first;
    list->first->prev = node->prev;

    node->prev = list->last;
    list->last->next = node;

    lt->num += list->num;
    
    lt_free(list);
    *plist = NULL;

    return lt->num;
}

int lt_insert_list_before (dlist_t * lt, dlist_t ** plist, void * data)
{
    dlist_t * list = NULL;
    dnode_t * node = NULL;

    if (!plist) return -1;

    list = *plist;

    if (!lt || !list) return -1;

    if (list->num == 0) {
        lt_free(list);
        *plist = NULL;
        return lt->num;
    }

    node = (dnode_t *) data;
    if (!node) return -1;

    if (lt->first == node) {
        return lt_head_combine (lt, plist);
    }
    
    node->prev->next = list->first;
    list->first->prev = node->prev;

    node->prev = list->last;
    list->last->next = node;

    lt->num += list->num;

    lt_free(list);
    *plist = NULL;

    return lt->num;
}


int lt_insert_list_after (dlist_t * lt, dlist_t ** plist, void * data)
{
    dlist_t * list = NULL;
    dnode_t * node = NULL;

    if (!plist) return -1;

    list = *plist;

    if (!lt || !list) return -1;

    if (list->num == 0) {
        lt_free(list);
        *plist = NULL;
        return lt->num;
    }

    node = (dnode_t *) data;
    if (!node) return -1;

    if (lt->last == node) {
        return lt_tail_combine (lt, plist);
    }
    
    node->next->prev = list->last;
    list->last->next = node->next;

    node->next = list->first;
    list->first->prev = node;

    lt->num += list->num;

    lt_free(list);
    *plist = NULL;

    return lt->num;
}


/* the following are the routines that delete the elements of the list */

void * lt_rm_head (dlist_t * lt)
{
    dnode_t * ret = NULL;

    if (!lt || lt->num == 0)
        return NULL;

    ret = lt->first;
    if (!ret) return NULL;

    lt->first = ret->next;
    if (lt->first) lt->first->prev = NULL;

    if (ret->next) {
        ret->next->prev = ret->prev;
    } else {
        lt->last = ret->prev;
        if (lt->last) lt->last->next = NULL;
    }

    lt->num -= 1;

    ret->next = ret->prev = NULL;

    return ret;
}

void * lt_rm_tail (dlist_t * lt)
{
    dnode_t * ret = NULL;

    if (!lt || lt->num == 0)
        return NULL;

    ret = lt->last;
    if (!ret) return NULL;

    if (ret->prev) {
        ret->prev->next = NULL;
    } else {
        lt->first = NULL;
    }

    lt->last = ret->prev;
    if (lt->last) lt->last->next = NULL;

    lt->num -= 1;

    ret->next = ret->prev = NULL;

    return ret;
}

void * lt_delete (dlist_t * lt, int loc)
{
    dnode_t * ret = NULL;

    if (!lt || lt->num == 0 || loc < 0 || loc >= lt->num)
        return NULL;
                
    ret = getNode(lt, loc);
    if (ret == NULL) return NULL;

    if (ret->prev) {
        ret->prev->next = ret->next;
    } else {
        lt->first = ret->next;
        if (lt->first) lt->first->prev = NULL;
    }

    if (ret->next) {
        ret->next->prev = ret->prev;
    } else {
        lt->last = ret->prev;
        if (lt->last) lt->last->next = NULL;
    }

    lt->num -= 1;

    ret->next = ret->prev = NULL;
    return ret;
}

void * lt_delete_ptr (dlist_t * lt, void * node)
{
    dnode_t * ret = NULL;

    if (!lt || lt->num == 0 || !node) return NULL;

    ret = (dnode_t *) node;

    if (!ret->prev && lt->first != ret) return NULL;
    if (!ret->next && lt->last != ret) return NULL;

    if (ret->prev) {
        ret->prev->next = ret->next;
    } else {
        lt->first = ret->next;
        if (lt->first)
            lt->first->prev = NULL;
    }

    if (ret->next) {
        ret->next->prev = ret->prev;
    } else {
        lt->last = ret->prev;
        if (lt->last)
            lt->last->next = NULL;
    }

    lt->num--;

    ret->next = ret->prev = NULL;
    return ret;
}


void * lt_delete_from_loc (dlist_t * lt, int from, int num)
{
    dnode_t * ret = NULL, * remainHead = NULL;

    if (!lt || lt->num == 0 || from >= lt->num || num == 0) 
        return NULL;

    if (from < 0) from = 0;
    if (from + num > lt->num) num = lt->num - from;

    ret = getNode(lt, from);
    if (ret == NULL) return NULL;

    remainHead = getNode(lt, from + num);

    if (ret->prev) {
        ret->prev->next = remainHead;
    } else {
        lt->first = remainHead;
        if (lt->first) lt->first->prev = NULL;
    }

    if (remainHead) {
        remainHead->prev = ret->prev;
    } else {
        lt->last = ret->prev;
        if (lt->last) lt->last->next = NULL;
    }

    lt->num -= num;

    ret->prev = NULL;
    return ret;
}


void * lt_delete_from_node (dlist_t * lt, void * node, int num)
{
    dnode_t * ret = NULL, * remainHead = NULL;
    int i;
    
    if (!lt || lt->num == 0 || !node || num == 0)
        return NULL;
    
    ret = (dnode_t *)node;
    
    for (i = 0, remainHead = ret; i < num && remainHead; i++)
        remainHead = remainHead->next;
    
    lt->num -= i;
    
    if (ret->prev) {
        ret->prev->next = remainHead;
    } else {
        lt->first = remainHead;
        if (lt->first) lt->first->prev = NULL;
    }

    if (remainHead) {
        remainHead->prev = ret->prev;
    } else {
        lt->last = ret->prev;
        if (lt->last) lt->last->next = NULL;
    }

    return ret;
}


/* following is the routines that access the elements of the list */

int lt_num (dlist_t * lt)
{
    if (!lt) return 0;

    return lt->num;
}


void * lt_get (dlist_t * lt, int loc)
{
    dnode_t * ret = NULL;

    if (!lt || lt->num == 0 || loc < 0 || loc >= lt->num)
    return NULL;

    ret = lt->first;
    while (loc--) ret = ret->next;

    return ret;
}


void * lt_value (dlist_t * lt, int loc)
{
    dnode_t * ret = NULL;

    if (!lt || lt->num == 0 || loc < 0 || loc >= lt->num)
        return NULL;

    ret = lt->first;
    while (loc--) ret = ret->next;

    return ret;
}


void * lt_first (dlist_t * lt) 
{
    dnode_t * ret = NULL;

    if (!lt || lt->num == 0)
        return NULL;

    ret = lt->first;
    return ret;        
}


void * lt_last (dlist_t * lt)
{
    dnode_t * ret = NULL;

    if (!lt || lt->num == 0)
        return NULL;

     ret = lt->last;
     return ret;
}


int lt_index (dlist_t * lt, void * node)
{
    int ind = -1;
    dnode_t * cur = NULL;

    if (!lt || lt->num == 0 || !node)
        return -1;

    for (cur = lt->first, ind = 0; cur && cur != node && ind < lt->num; ind++)
    cur = cur->next;

    if (cur == node) return ind;

    return -1;
}



/* mutually convertion between arr_t and dlist_t for some particular purpose */

arr_t * lt_convert_to_linear (dlist_t * lt)
{
    arr_t   * ar = NULL;
    dnode_t * node;
    int       i;

    if (!lt || lt->num == 0)
    return NULL;

    ar = arr_new(lt->num + 4);
    if (!ar) return NULL;
    
    for (i = 0, node = lt->first; i < lt->num && node; i++, node = node->next) {
        arr_push(ar, node);
    }

    return ar;
}


dlist_t * lt_new_from_linear (arr_t * st)
{
    dlist_t * ret = NULL;
    int i;
    dnode_t * node = NULL;

    if (!st) return NULL;

    ret = lt_new();
    if (!ret) return NULL;

    for (i = 0; i < arr_num(st); i++) {
        node = arr_value(st, i);
        lt_append(ret, node);
    }

    return ret;
}



/* the following routines are adopted for sorting and searching */

void * lt_search (dlist_t * lt, void * pattern, int (*cmp)(void *, void *))
{
    dnode_t * item = NULL;
    int i;

    if (!lt || !pattern) return NULL;

    for (i = 0, item = lt->first; i < lt->num && item; i++) {
        if (cmp) { 
            if ((*cmp)(item, pattern) == 0) return item;
        } else {
            if (item == pattern) return item;
        }
        item = item->next;
    }

    return NULL;
}


arr_t * lt_search_all (dlist_t * lt, void * pattern, int (*cmp)(void *, void *))
{
    dnode_t * item = NULL;
    arr_t * ret = NULL;
    int i;

    if (!lt || !pattern) return NULL;

    ret = arr_new(4);
    if (!ret) return NULL;

    for (i = 0, item = lt->first; i < lt->num && item; i++) {
        if (cmp) {
            if ((*cmp)(item, pattern) == 0) arr_push(ret, item);
        } else {
            if (item == pattern) arr_push(ret, item);
        }

        item = item->next;
    }
    
    if (arr_num(ret) == 0) {
        arr_free(ret);
        return NULL;
    }

    return ret;            
}

