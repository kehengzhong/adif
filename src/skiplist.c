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

#include "btype.h"
#include "memory.h"
#include "btime.h"
#include "skiplist.h"

#define SKL_P      0.25

int random_level ()
{
    int level = 0;

    while ((random() & 0xFFFF) < (SKL_P * 0xFFFF) && level < MAX_LEVEL)
        level++;

    return level;
}

size_t skipnode_memsize (skipnode_t * node)
{
    if (!node) return 0;

    return sizeof(*node) + node->level * sizeof(skipnode_t *);
}

skipnode_t * skipnode_alloc (skiplist_t * skl, int level)
{
    skipnode_t * node = NULL;
    int i;

    if (level < 0) level = 0;

    if (skl && skl->osalloc)
        node = koszmalloc(sizeof(*node) + level * sizeof(skipnode_t *));
    else
        node = kzalloc(sizeof(*node) + level * sizeof(skipnode_t *));
    if (!node) return NULL;

    node->level = level;
    node->key = NULL;
    node->value = NULL;
    node->prev = NULL;

    for (i = 0; i <= level; i++) {
        node->forward[i] = NULL;
    }

    return node;
}

int skipnode_free (skiplist_t * skl, skipnode_t * node, void * vfree)
{
    sklfree_t * freefunc = (sklfree_t *)vfree;

    if (!node) return -1;

    if (freefunc) (*freefunc)(node->value);

    if (skl && skl->osalloc)
        kosfree(node);
    else
        kfree(node);

    return 0;
}


skiplist_t * skiplist_alloc (void * cmp)
{
    skiplist_t * skl = NULL;
    int i;

    skl = kzalloc(sizeof(*skl));
    if (!skl) return NULL;

    skl->level = 0;
    skl->osalloc = 0;
    skl->num = 0;
    skl->cmp = cmp;

    skl->header = skipnode_alloc(skl, MAX_LEVEL);
    if (!skl->header) {
        kfree(skl);
        return NULL;
    }

    for (i = 0; i < MAX_LEVEL; i++) {
        skl->levelnum[i] = 0;
        skl->header->forward[i] = NULL;
    }

    srandom(btime(NULL));

    return skl;
}

skiplist_t * skiplist_osalloc (void * cmp)
{
    skiplist_t * skl = NULL;
    int i;

    skl = koszmalloc(sizeof(*skl));
    if (!skl) return NULL;

    skl->level = 0;
    skl->osalloc = 1;
    skl->num = 0;
    skl->cmp = cmp;

    skl->header = skipnode_alloc(skl, MAX_LEVEL);
    if (!skl->header) {
        kosfree(skl);
        return NULL;
    }

    for (i = 0; i < MAX_LEVEL; i++) {
        skl->levelnum[i] = 0;
        skl->header->forward[i] = NULL;
    }

    srandom(btime(NULL));

    return skl;
}

void skiplist_free_all (skiplist_t * skl, void * vfree)
{
    skipnode_t * node = NULL;
    skipnode_t * next = NULL;

    if (!skl) return;

    node = skl->header->forward[0];
    while (node != NULL) {
        next = node->forward[0];
        skipnode_free(skl, node, vfree);
        node = next;
    }

    skipnode_free(skl, skl->header, NULL);

    if (skl->osalloc)
        kosfree(skl);
    else
        kfree(skl);

    return;
}

void skiplist_free (skiplist_t * skl)
{
    skiplist_free_all(skl, NULL);
}

long skiplist_memsize (skiplist_t * skl)
{
    long size = 0;
    int i, num;
    skipnode_t * node = NULL;

    if (!skl) return 0;

    size = sizeof(*skl) + skipnode_memsize(skl->header);

    node = skiplist_first(skl);
    num = skiplist_num(skl);
    for (i = 0; i < num && node; i++) {
        size += skipnode_memsize(node);
        node = skiplist_next(node);
    }

    return size;
}

/*
level4  O-------------------------------O-------------------------------O
        |                               |                               |
level3  O---------------O---------------O---------------O---------------O
        |               |               |               |               |
level2  O-------O-------O-------O-------O-------O-------O-------O-------O
        |       |       |       |       |       |       |       |       |
level1  O---O---O---O---O---O---O---O---O---O---O---O---O---O---O---O---O
        |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
level0  O-O-O-O-O-O-O-O-O-O-O-O-O-O-O-O-O-O-O-O-O-O-O-O-O-O-O-O-O-O-O-O-O
*/

int skiplist_insert (skiplist_t * skl, void * key, void * value)
{
    skipnode_t * node = NULL;
    skipnode_t * next = NULL;
    skipnode_t * newnode = NULL;
    skipnode_t * down[MAX_LEVEL] = {NULL};
    int level;
    int cmpret = -1;

    if (!skl) return -1;

    node = skl->header;

    /* find the minimum node that greater than key for every level list, record into array down */
    for (level = skl->level; level >= 0; level--) {
        while ((next = node->forward[level]) != NULL) {
            if ((cmpret = (*skl->cmp)(next->key, key)) >= 0)
                break;
            node = next;
        }
        down[level] = node;
    }

    /* if same key/value already exists */
    if (next && cmpret == 0) { //(*skl->cmp)(next->key, key) == 0) {
        return 0;
    }

    /* if level is greater than current level of skiplist*/
    level = random_level();
    if (level > (int)skl->level) {
        level = ++skl->level;
        down[level] = skl->header;
    }

    newnode = skipnode_alloc(skl, level);
    if (!newnode) return -100;

    newnode->key = key;
    newnode->value = value;

    /* prev of header can be regarded as pointing to the last node in skiplist.*/
    newnode->prev = node;
    if (next) next->prev = newnode;
    else skl->header->prev = newnode;

    while (level >= 0) {
        node = down[level];
        newnode->forward[level] = node->forward[level];
        node->forward[level] = newnode;
        skl->levelnum[level]++;
        level--;
    }
    skl->num++;

    return 1;
}


void * skiplist_delete (skiplist_t * skl, void * key)
{
    return skiplist_delete_gemin(skl, key, 0);
}

void * skiplist_delete_gemin (skiplist_t * skl, void * key, int gemin)
{
    skipnode_t * node = NULL;
    skipnode_t * next = NULL;
    skipnode_t * down[MAX_LEVEL] = {NULL};
    void       * value = NULL;
    int level;
    int sklev = 0;
    int cmpret = -1;

    if (!skl || skl->num <= 0) return NULL;

    node = skl->header;

    /* find the minimum node that greater than key for every level list, record into array down */
    for (level = skl->level; level >= 0; level--) {
        while ((next = node->forward[level]) != NULL) {
            if ((cmpret = (*skl->cmp)(next->key, key)) >= 0)
                break;
            node = next;
        }
        down[level] = node;
    }

    /* if key not exists */
    if (!next) return NULL;

    if ((gemin != 0 &&  cmpret < 0) || (gemin == 0 && cmpret != 0)) {
        return NULL;
    }

    value = next->value;
    if (next->forward[0]) next->forward[0]->prev = node;
    else {
        if (skl->header != node) skl->header->prev = node;
        else skl->header->prev = NULL;
    }

    level = 0;
    sklev = skl->level;

    while (level <= sklev && (node = down[level])->forward[level] == next) {
        skl->levelnum[level]--;
        node->forward[level] = next->forward[level];
        level++;
    }

    skipnode_free(skl, next, NULL);
    skl->num--;

    /* if the node list of index level is empty */
    while (skl->header->forward[sklev] == NULL && sklev > 0)
        sklev--;
    skl->level = sklev;

    return value;
}

void * skiplist_find (skiplist_t * skl, void * key)
{
    skipnode_t * node = NULL;
    skipnode_t * next = NULL;
    int level;
    int cmpret = -1;

    if (!skl || skl->num <= 0) return NULL;

    node = skl->header;

    /* find the minimum node that greater than key for every level list, record into array down */
    for (level = skl->level; level >= 0; level--) {
        while ((next = node->forward[level]) != NULL) {
            if ((cmpret = (*skl->cmp)(next->key, key)) >= 0)
                break;
            node = next;
        }
    }

    /* if key not exists */
    if (!next || cmpret != 0) { //(*skl->cmp)(next->key, key) != 0) {
        return NULL;
    }

    return next->value;
}

void * skiplist_find_node (skiplist_t * skl, void * key)
{
    skipnode_t * node = NULL;
    skipnode_t * next = NULL;
    int level;
    int cmpret = -1;

    if (!skl || skl->num <= 0) return NULL;

    node = skl->header;

    /* find the minimum node that greater than key for every level list, record into array down */
    for (level = skl->level; level >= 0; level--) {
        while ((next = node->forward[level]) != NULL) {
            if ((cmpret = (*skl->cmp)(next->key, key)) >= 0)
                break;
            node = next;
        }
    }

    /* if key not exists */
    if (!next || cmpret != 0) { //(*skl->cmp)(next->key, key) != 0) {
        return NULL;
    }

    return next;
}

/* find the minimum node that greater than key */
void * skiplist_find_gemin (skiplist_t * skl, void * key, skipnode_t ** pprev, skipnode_t ** pnode, int * found)
{
    skipnode_t * node = NULL;
    skipnode_t * next = NULL;
    int level;
    int cmpret = -1;

    if (found) *found = 0;
    if (pprev) *pprev = NULL;
    if (pnode) *pnode = NULL;

    if (!skl || skl->num <= 0) return NULL;

    node = skl->header;

    /* find the minimum node that greater than key for every level list, record into array down */
    for (level = skl->level; level >= 0; level--) {
        while ((next = node->forward[level]) != NULL) {
            if ((cmpret = (*skl->cmp)(next->key, key)) >= 0)
                break;
            node = next;
        }
    }

    if (pprev && node != skl->header) *pprev = node;

    /* if key not exists */
    if (!next || cmpret < 0) { //(*skl->cmp)(next->key, key) < 0) {
        return NULL;
    }

    if (cmpret == 0) {
        if (found) *found = 1;
        if (pnode) *pnode = next->forward[0];
        return next->value;
    }

    if (found) *found = 0;
    if (pnode) *pnode = next;

    return next->value;
}

int skiplist_num (skiplist_t * skl)
{
    if (!skl) return 0;

    return skl->levelnum[0];
}

skipnode_t * skiplist_first (skiplist_t * skl)
{
    if (!skl) return NULL;

    return skl->header->forward[0];
}

skipnode_t * skiplist_last (skiplist_t * skl)
{
    if (!skl) return NULL;

    return skl->header->prev;
}

skipnode_t * skiplist_next (skipnode_t * node)
{
    if (!node) return NULL;

    return node->forward[0];
}

skipnode_t * skiplist_prev (skipnode_t * node)
{
    if (!node) return NULL;

    if (node->prev && !node->prev->key && !node->prev->value)
        return NULL;

    return node->prev;
}


void skiplist_print (skiplist_t * skl, FILE * fp)
{
    int i;

    if (!skl) return;

    printf("skiplist: num=%d level=%d levelnum=[", skl->num, skl->level);
    for (i = 0; i < (int)skl->level; i++) printf("%d ", skl->levelnum[i]);
    printf("]\n");
}


