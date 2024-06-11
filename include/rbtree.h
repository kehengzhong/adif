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
/*
   A red-black tree is a special type of binary search tree, used to organize pieces of
   comparable data. The leaf nodes of red-black trees (NIL nodes) do not contain keys or
   data. Red-black trees offer worst-case guarantees for insertion time, deletion time,
   and search time. A binary search tree (BST), also called an ordered or sorted binary
   tree, is a rooted binary tree data structure with the key of each internal node being
   greater than all the keys in the respective node's left subtree and less than the ones
   in its right subtree. The following requirements must be satisfied by a red-black tree:
   (1) Every node is either red or black.
   (2) Root node is black.
   (3) All NIL nodes are considered black.
   (4) A red node does not have a red child.
   (5) Every path from a given node to any of its descendant NIL nodes goes through the
       same number of black nodes.
*/

#ifndef _RBTREE_H_
#define _RBTREE_H_
 
#ifdef __cplusplus
extern "C" {
#endif

#define RBT_RED    0
#define RBT_BLACK  1

typedef int rbtcmp_t (void * a, void * b);
typedef int rbtfree_t (void * obj);
typedef int rbtcb_t (void * para, void * key, void * obj, int index);

#define RBTKey(node) ((node && ((rbtnode_p)node)->alloc) ? ((rbtnode_p)(node))->key : node)
#define RBTObj(node) ((node && ((rbtnode_p)node)->alloc) ? ((rbtnode_p)(node))->obj : node)

typedef struct RBTNode {
    struct RBTNode * left;
    struct RBTNode * right;
    struct RBTNode * parent;
    unsigned         color   : 1;
    unsigned         alloc   : 1;
    unsigned         depth   : 5;
    unsigned         width   : 25;

    void           * key;
    void           * obj;
} RBTNode, rbtnode_t, *rbtnode_p;

typedef struct RBTree {
    rbtnode_t   * root;
    rbtcmp_t    * cmp;
    int           num;

    unsigned      alloc_node : 28;
    unsigned      alloc_tree : 2;
    unsigned      alloctype  : 2; //0-default kalloc/kfree 1-os-specific malloc/free 2-kmempool alloc/free 3-kmemblk alloc/free

    void        * mpool;
    void        * rbtnode_pool;

    int           layer[32];
    int           depth;
    int           lwidth;
    int           rwidth;
} RBTree, rbtree_t, *rbtree_p;

void * rbtnode_min (void * node);
void * rbtnode_max (void * node);

void * rbtnode_next (void * node);
void * rbtnode_prev (void * node);

void * rbtnode_get (void * vnode, void * key, int (*cmp)(void *,void *));

int    rbtnode_free (void * vtree, void * vnode, rbtfree_t * freefunc);

/* The definition about alloc_node paramenter:
   The red-black tree is composed of many internal nodes including key and data, which
   establish tree-type relations through their internal pointers. Internal nodes contain
   pointers to parent, left subtree, right subtree, and other data members. When adding
   data to the red-black tree, it is necessary to allocate memory space for internal nodes,
   in which case the alloc_node value is 1. However, if the memory space required by the
   internal node is already included in the data object to be added, there is no need to
   allocate additional memory for the internal node. In this case, the alloc_node value is 0. */

int rbtree_init (void * vrbt, rbtcmp_t * cmp, int alloc_node, int atype, void * mpool, void * nodepool);

void * rbtree_alloc (rbtcmp_t * cmp, int alloc_node, int atype, void * mpool, void * nodepool);
void * rbtree_new  (rbtcmp_t * cmp, int alloc_node);

/* RBTree and RBTNode are allocated by calling malloc function of the os-system
 * instead of memory pool. */
void * rbtree_osalloc (rbtcmp_t * cmp, int alloc_node, void * nodepool);


void   rbtree_free (void * ptree);
void   rbtree_free_all (void * ptree, void * vfunc);

void   rbtree_zero (void * vptree);
int    rbtree_num (void * ptree);

void * rbtree_get_node  (void * ptree, void * key);
void * rbtree_get       (void * ptree, void * key);

void * rbtree_get_node_pn (void * vptree, void * key, void ** pprev, void ** pnext);

/* get the minimum node greater than key */
void * rbtree_get_gemin (void * vptree, void * key);
void * rbtree_get_node_gemin (void * vptree, void * key);

void * rbtree_min_node (void * ptree);
void * rbtree_min      (void * ptree);
void * rbtree_max_node (void * ptree);
void * rbtree_max      (void * ptree);

int    rbtree_insert (void * vptree, void * key, void * obj, void ** pnode);

int    rbtree_delete_node (void * vptree, void * vnode);

void * rbtree_delete (void * ptree, void * key);
void * rbtree_delete_gemin (void * vptree, void * key);

void * rbtree_delete_min (void * ptree);
void * rbtree_delete_max (void * ptree);

int    rbtree_inorder  (void * ptree, rbtcb_t * cb, void * cbpara);
int    rbtree_preorder (void * ptree, rbtcb_t * cb, void * cbpara);
int    rbtree_postorder(void * ptree, rbtcb_t * cb, void * cbpara);

void   rbtree_set_dimen (void * vptree);

#ifdef __cplusplus
}
#endif
 
#endif

