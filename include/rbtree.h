/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
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

#define RBTKey(node) ((node) ? ((rbtnode_p)(node))->key : NULL)
#define RBTObj(node) ((node) ? ((rbtnode_p)(node))->obj : NULL)

#pragma pack(1)

typedef struct RBTNode {
    struct RBTNode * left;
    struct RBTNode * right;
    struct RBTNode * parent;
    unsigned         color:1;
    unsigned         depth:5;
    unsigned         width:26;

    void           * key;
    void           * obj;
} RBTNode, rbtnode_t, *rbtnode_p;

#pragma pack()


typedef struct RBTree {
    rbtnode_t   * root;
    int           num;
    rbtcmp_t    * cmp;
    int           alloc_node;

    int           depth;
    int           layer[32];
    int           lwidth;
    int           rwidth;
} RBTree, rbtree_t, *rbtree_p;



void * rbtnode_min (void * node);
void * rbtnode_max (void * node);

void * rbtnode_next (void * node);
void * rbtnode_prev (void * node);

void * rbtnode_get (void * vnode, void * key, int (*cmp)(void *,void *));

int    rbtnode_free (void * vnode, rbtfree_t * freefunc, int alloc_node);


/* alloc_node = 1 means that:
          the caller's data object does not reserve space for rbtree-node pointer.
          when inserting a new node, an 44-bytes memory block for rbtnode_t will be allocated
          and one of member pointers is assigned the caller's data object.
   alloc_node = 0 means: 
          the caller's data object must reserved 32-bytes at the begining for rbtnode_t.
*/   

void * rbtree_new  (rbtcmp_t * cmp, int alloc_node);
void   rbtree_free (void * ptree);
void   rbtree_free_all (void * ptree, rbtfree_t * freefunc);

void   rbtree_zero (void * vptree);

int    rbtree_num (void * ptree);


void * rbtree_get_node  (void * ptree, void * key);
void * rbtree_get       (void * ptree, void * key);
int    rbtree_mget_node (void * ptree, void * key, void ** plist, int listsize);
int    rbtree_mget      (void * ptree, void * key, void ** plist, int listsize);

void * rbtree_min_node (void * ptree);
void * rbtree_min      (void * ptree);
void * rbtree_max_node (void * ptree);
void * rbtree_max      (void * ptree);

int    rbtree_insert (void * vptree, void * key, void * obj, void ** pnode);

int    rbtree_delete_node (void * vptree, void * vnode);

void * rbtree_delete (void * ptree, void * key);
int    rbtree_mdelete (void * vptree, void * key, void ** plist, int listnum);

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

