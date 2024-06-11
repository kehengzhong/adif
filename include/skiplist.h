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

#ifndef _SKIPLIST_H_
#define _SKIPLIST_H_ 
    
#ifdef __cplusplus
extern "C" {
#endif 

#define MAX_LEVEL  31

typedef int sklcmp_t (void * a, void * b);
typedef int sklfree_t (void * obj);

typedef struct skipnode_s {
    int      level;
    void   * key;
    void   * value;

    struct skipnode_s * prev;

    /* variable sized array of forward pointers */
    struct skipnode_s * forward[1];

} skipnode_t, *skipnode_p;

#define SKLKey(node) ((node) ? ((skipnode_p)(node))->key : NULL)
#define SKLObj(node) ((node) ? ((skipnode_p)(node))->value: NULL)


/* Data structure diagram of skiplist as following:

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

typedef struct skiplist_s {
    unsigned     level   : 31;
    unsigned     osalloc : 1;
    int          num;

    int          levelnum[MAX_LEVEL];

    sklcmp_t   * cmp;

    skipnode_t * header;

} skiplist_t;

skiplist_t * skiplist_alloc (void * cmp);
skiplist_t * skiplist_osalloc (void * cmp);
void         skiplist_free (skiplist_t * skl);
void         skiplist_free_all (skiplist_t * skl, void * vfree);

long skiplist_memsize (skiplist_t * skl);

int    skiplist_insert (skiplist_t * skl, void * key, void * value);
void * skiplist_delete (skiplist_t * skl, void * key);
void * skiplist_find   (skiplist_t * skl, void * key);

void * skiplist_find_node (skiplist_t * skl, void * key);

#define skiplist_get(skl, key) skiplist_find((skl), (key))
#define skiplist_get_node(skl, key) skiplist_find_node((skl), (key))

/* delete the minimal node that greater than key */
void * skiplist_delete_gemin (skiplist_t * skl, void * key, int gemin);

/* find the minimal node that greater than key */
void * skiplist_find_gemin (skiplist_t * skl, void * key, skipnode_t ** pprev, skipnode_t ** pnode, int * found);

int skiplist_num (skiplist_t * skl);

skipnode_t * skiplist_first (skiplist_t * skl);
skipnode_t * skiplist_last (skiplist_t * skl);

skipnode_t * skiplist_next (skipnode_t * node);
skipnode_t * skiplist_prev (skipnode_t * node);


void skiplist_print (skiplist_t * skl, FILE * fp);

#ifdef __cplusplus
}
#endif 

#endif

