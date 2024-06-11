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
#include "kemalloc.h"
#include "mpool.h"
#include "trace.h"
#include "rbtree.h"
 

void * rbtnode_min (void * vnode)
{
    rbtnode_t * node = (rbtnode_t *)vnode;

    if (!node) return NULL;

    while (node->left != NULL)
        node = node->left;

    return node;
}

void * rbtnode_max (void * vnode)
{
    rbtnode_t * node = (rbtnode_t *)vnode;

    if (!node) return NULL;
 
    while (node->right != NULL) 
        node = node->right;
 
    return node;
}


/* find the predecessor of the node, that is, find the maximum node in the
   red-black tree whose data value is less than this node. */

void * rbtnode_prev (void * vnode)
{
    rbtnode_t  * node = (rbtnode_t *)vnode;
    rbtnode_t  * prev = NULL;

    if (!node)
        return NULL;
 
    /* if the node has a left child, its predecessor is the maximm node of
       the subtree with its left child as the root" */

    if (node->left != NULL)
        return rbtnode_max(node->left);

    /* If the node has no left children. There are two possibilities:
     * (1) If the node is a right child, its predecessor is its parent node.
     * (2) If the node is a left child, find the lowest parent of the node, 
     *     and the parent node should have a right child. The found parent
     *     node is the predecessor of the node */

    prev = node->parent;

    while (prev != NULL && node == prev->left) {
        node = prev;
        prev = prev->parent;
    }
 
    return prev;
}

/* find the successor of the node, that is, find the minimum node whose data
   value is greater than this node in the red-black tree. */

void * rbtnode_next (void * vnode)
{
    rbtnode_t  * node = (rbtnode_t *)vnode;
    rbtnode_t  * next = NULL;
 
    if (!node) return NULL;
 
    /* if the node has a right child, its successor is the minimum node of the
       subtree with its right child as the root */

    if (node->right != NULL)
        return rbtnode_min(node->right);
 
    /* If the node has no right child. There are two possibilities:
     * (1) If the node is a left child, its successor is its parent node.
     * (2) If the node is a right child, find the lowest parent of the node, and
           the parent node should have a left child. The "lowest parent node"
           found is the successor of the node */

    next = node->parent;

    while (next != NULL && node == next->right) {
        node = next;
        next = next->parent;
    }
 
    return next;
}

void * rbtnode_get (void * vnode, void * key, int (*cmp)(void *,void *))
{
    rbtnode_t * node = (rbtnode_t *)vnode;
    int         ret = 0;
 
    if (!node) return NULL;
 
    while (node != NULL) {
        ret = (*cmp)(node, key);
        if (ret == 0)
            return node;
 
        if (ret > 0)
            node = node->left;
        else
            node = node->right;
    }
 
    return NULL;
}

void * rbtnode_alloc (rbtree_t * rbt)
{
    rbtnode_t * node = NULL;
 
    if (rbt->rbtnode_pool) {
        node = mpool_fetch((mpool_t *)rbt->rbtnode_pool);
        if (node) memset(node, 0, sizeof(*node));
    } else {
	node = k_mem_zalloc(sizeof(*node), rbt->alloctype, rbt->mpool);
    }

    if (node) node->alloc = rbt->alloc_node;

    return node;
}

int rbtnode_free (void * vtree, void * vnode, rbtfree_t * freefunc)
{
    rbtree_t * rbt = (rbtree_t *)vtree;
    rbtnode_t * node = (rbtnode_t *)vnode;

    if (!rbt) return -1;
    if (!node) return -2;

    if (rbt->alloc_node) {
        if (freefunc)
            (*freefunc)(node->obj);

        if (rbt->rbtnode_pool) {
            mpool_recycle((mpool_t *)rbt->rbtnode_pool, node);
        } else {
	    k_mem_free(node, rbt->alloctype, rbt->mpool);
        }

    } else {
        if (freefunc)
            (*freefunc)(node);
    }

    return 0;
}

static void rbtree_free_node (rbtree_t * rbt, rbtnode_t * node, rbtfree_t * freefunc)
{
    if (!rbt || !node) return;

    if (node->left)
        rbtree_free_node(rbt, node->left, freefunc);

    if (node->right)
        rbtree_free_node(rbt, node->right, freefunc);

    rbtnode_free(rbt, node, freefunc);
}

/* The definition about alloc_node paramenter:
   The red-black tree is composed of many internal nodes including key and data, which
   establish tree-type relations through their internal pointers. Internal nodes contain
   pointers to parent, left subtree, right subtree, and other data members. When adding
   data to the red-black tree, it is necessary to allocate memory space for internal nodes,
   in which case the alloc_node value is 1. However, if the memory space required by the
   internal node is already included in the data object to be added, there is no need to
   allocate additional memory for the internal node. In this case, the alloc_node value is 0. */

int rbtree_init (void * vptree, rbtcmp_t * cmp, int alloc_node, int alloctype, void * mpool, void * nodepool)
{
    rbtree_t * rbt = (rbtree_t *)vptree;

    if (!rbt) return -1;

    rbt->root = NULL;
    rbt->num = 0;
    rbt->cmp = cmp;
    rbt->alloc_node = alloc_node > 0 ? 1 : 0;

    rbt->alloc_tree = 0;
    rbt->alloctype = alloctype;

    rbt->mpool = mpool;
    rbt->rbtnode_pool = nodepool;

    rbt->depth = 0;
    memset(rbt->layer, 0, sizeof(rbt->layer));
    rbt->lwidth = 0;
    rbt->rwidth = 0;

    return 0;
}

void * rbtree_alloc (rbtcmp_t * cmp, int alloc_node, int alloctype, void * mpool, void * nodepool)
{
    rbtree_t * rbt = NULL;

    rbt = k_mem_zalloc(sizeof(*rbt), alloctype, mpool);
    if (!rbt) return rbt;

    rbtree_init(rbt, cmp, alloc_node, alloctype, mpool, nodepool);

    rbt->alloc_tree = 1;

    return rbt;
}

void * rbtree_new (rbtcmp_t * cmp, int alloc_node)
{
    return rbtree_alloc(cmp, alloc_node, 0, NULL, NULL);
}

void * rbtree_osalloc (rbtcmp_t * cmp, int alloc_node, void * nodepool)
{
    return rbtree_alloc(cmp, alloc_node, 1, NULL, nodepool);
}

void rbtree_free (void * vptree)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;

    if (!ptree) return;

    if (ptree->alloc_node)
        rbtree_free_node(ptree, ptree->root, NULL);

    if (!ptree->alloc_tree) 
       return;

    k_mem_free(ptree, ptree->alloctype, ptree->mpool);
}
 
void rbtree_free_all (void * vptree, void * vfunc)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;
    rbtfree_t * freefunc = (rbtfree_t *)vfunc;

    if (!ptree) return;

    rbtree_free_node(ptree, ptree->root, freefunc);

    if (!ptree->alloc_tree) 
       return;

    k_mem_free(ptree, ptree->alloctype, ptree->mpool);
}
 
void rbtree_zero (void * vptree)
{
    rbtree_t  * rbt = (rbtree_t *)vptree;

    if (!rbt) return;

    if (rbt->alloc_node)
        rbtree_free_node(rbt, rbt->root, NULL);

    rbt->root = NULL;
    rbt->num = 0;

    rbt->depth = 0;
    memset(rbt->layer, 0, sizeof(rbt->layer));
    rbt->lwidth = 0;
    rbt->rwidth = 0;
}

int rbtree_num (void * ptree)
{
    rbtree_t  * rbt = (rbtree_t *)ptree;

    if (!rbt) return 0;
    return rbt->num;
}

 
void * rbtree_get_node (void * vptree, void * key)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;
    rbtnode_t * node = NULL;
    int         ret = 0;

    if (!ptree) return NULL;

    node = ptree->root;

    while (node != NULL) {
        if (ptree->alloc_node)
            ret = (*ptree->cmp)(node->obj, key);
        else
            ret = (*ptree->cmp)(node, key);

        if (ret > 0)
            node = node->left;
        else if (ret < 0)
            node = node->right;
        else
            return node;
    }

    return NULL;
}

void * rbtree_get (void * vptree, void * key)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;
    rbtnode_t * node = NULL;
    int         ret = 0;

    if (!ptree)
        return NULL;

    node = ptree->root;

    while (node != NULL) {
        if (ptree->alloc_node)
            ret = (*ptree->cmp)(node->obj, key);
        else
            ret = (*ptree->cmp)(node, key);

        if (ret > 0)
            node = node->left;
        else if (ret < 0)
            node = node->right;
        else
            return ptree->alloc_node ? node->obj : node;
    }

    return NULL;
}

void * rbtree_get_node_pn (void * vptree, void * key, void ** pprev, void ** pnext)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;
    rbtnode_t * parent = NULL;
    rbtnode_t * node = NULL;
    int         ret = 0;

    if (pprev) *pprev = NULL;
    if (pnext) *pnext = NULL;

    if (!ptree) return NULL;

    node = ptree->root;

    while (node != NULL) {
        parent = node;

        if (ptree->alloc_node)
            ret = (*ptree->cmp)(node->obj, key);
        else
            ret = (*ptree->cmp)(node, key);

        if (ret > 0) {
            node = node->left;
        } else if (ret < 0) {
            node = node->right;
        } else {
            if (pprev) *pprev = rbtnode_prev(node);
            if (pnext) *pnext = rbtnode_next(node);
            return node;
        }
    }

    if (ret > 0) {
        if (pprev) *pprev = rbtnode_prev(parent);
        if (pnext) *pnext = parent;
    } else if (ret < 0) {
        if (pprev) *pprev = parent;
        if (pnext) *pnext = rbtnode_next(parent);
    }

    return NULL;
}


void * rbtree_get_gemin (void * vptree, void * key)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;
    rbtnode_t * parent = NULL;
    rbtnode_t * node = NULL;
    int         ret = 0;

    if (!ptree) return NULL;

    node = ptree->root;

    while (node != NULL) {
        parent = node;

        if (ptree->alloc_node)
            ret = (*ptree->cmp)(node->obj, key);
        else
            ret = (*ptree->cmp)(node, key);

        if (ret > 0) {
            node = node->left;
        } else if (ret < 0) {
            node = node->right;
        } else {
            return ptree->alloc_node ? node->obj : node;
        }
    }

    if (ret > 0) {
        return ptree->alloc_node ? parent->obj : parent;
    } else if (ret < 0) {
        node = rbtnode_next(parent);
        if (node)
            return ptree->alloc_node ? node->obj : node;
    }

    return NULL;
}

void * rbtree_get_node_gemin (void * vptree, void * key)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;
    rbtnode_t * parent = NULL;
    rbtnode_t * node = NULL;
    int         ret = 0;

    if (!ptree) return NULL;

    node = ptree->root;

    while (node != NULL) {
        parent = node;

        if (ptree->alloc_node)
            ret = (*ptree->cmp)(node->obj, key);
        else
            ret = (*ptree->cmp)(node, key);

        if (ret > 0) {
            node = node->left;
        } else if (ret < 0) {
            node = node->right;
        } else {
            return node;
        }
    }

    if (ret > 0) {
        return parent;
    } else if (ret < 0) {
        return rbtnode_next(parent);
    }

    return NULL;
}


void * rbtree_min_node (void * vptree)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;
    rbtnode_t * node = NULL;

    if (!ptree)
        return NULL;

    node = ptree->root;
    if (!node) return NULL;

    while (node->left != NULL)
        node = node->left;

    return node;
}

void * rbtree_min (void * vptree)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;
    rbtnode_t * node = NULL;

    if (!ptree)
        return NULL;

    node = ptree->root;
    if (!node) return NULL;

    while (node->left != NULL)
        node = node->left;

    return ptree->alloc_node ? node->obj : node;
}

void * rbtree_max_node (void * vptree)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;
    rbtnode_t * node = NULL;
 
    if (!ptree)
        return NULL;
 
    node = ptree->root;
    if (!node) return NULL;

    while (node->right != NULL)
        node = node->right;

    return node;
}

void * rbtree_max (void * vptree)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;
    rbtnode_t * node = NULL;
 
    if (!ptree)
        return NULL;
 
    node = ptree->root;
    if (!node) return NULL;

    while (node->right != NULL)
        node = node->right;

    return ptree->alloc_node ? node->obj : node;
}


/* Rotate the node (x) of the red-black tree to the left.
 *      px                              px
 *     /                               /
 *    x                               y
 *   /  \    --(left rotation)-->    / \                #
 *  lx   y                          x  ry
 *     /   \                       /  \
 *    ly   ry                     lx  ly   
 */
static void rbtree_left_rotate (rbtree_t * ptree, rbtnode_t * x)
{
    rbtnode_t  * y = NULL;

    if (!ptree || !x || !x->right) return;

    y = x->right;

    /* Set the left child of Y as the right child of X. If the left child of Y is not
       NULL, set X as the parent of the left child of Y. */
    x->right = y->left;
    if (y->left != NULL)
        y->left->parent = x;
 
    /* Set the parent of x as the parent of y */
    y->parent = x->parent;
    if (x->parent == NULL) {
        /* If the parent of X is NULL, set Y as the root node. */
        ptree->root = y;
    } else {
        if (x->parent->left == x) {
            /* If X is the left child of its parent, set Y as the left child of
               X's parent */
            x->parent->left = y;
        } else {
            /* If X is the right child of its parent, set Y as the right child of
               X's parent */
            x->parent->right = y;
        }
    }
 
    /* Set X as the left child of Y */
    y->left = x;

    /* Set the parent of X to Y */
    x->parent = y;
}

/* Rotate the node y of the red-black tree to the right.
 *            py                               py
 *           /                                /
 *          y                                x
 *         /  \    --(right rotation)-->    /  \                     #
 *        x   ry                           lx   y
 *       / \                                   / \                   #
 *      lx  rx                                rx  ry
 */
static void rbtree_right_rotate (rbtree_t * ptree, rbtnode_t * y)
{
    rbtnode_t * x = NULL;

    if (!ptree || !y || !y->left) return;
 
    x = y->left;

    /* Set the right child of X as the left child of Y. If the right child of X is not
       NULL, set Y as the parent of the right child of X */
    y->left = x->right;
    if (x->right != NULL)
        x->right->parent = y;
 
    /* Set y's parent as x's parent. */
    x->parent = y->parent;
    if (y->parent == NULL) {
        /* If Y's parent is NULL, set X as the root node */
        ptree->root = x;
    } else {
        if (y == y->parent->right) {
            /* If Y is the right child of its parent, set X as the right child of Y's parent */
            y->parent->right = x;
        } else {
            /* If Y is the left child of its parent, set X as the left child of Y's parent */
            y->parent->left = x;
        }
    }
 
    /* Set y as the right child of x */
    x->right = y;
 
    /* Set the parent of y to x */
    y->parent = x;
}

static void rbtree_insert_fixup (rbtree_t * ptree, rbtnode_t * node)
{
    rbtnode_t  * parent = NULL;
    rbtnode_t  * gparent = NULL;
    rbtnode_t  * uncle = NULL;
    rbtnode_t  * tmp = NULL;

    if (!ptree || !node) return;

    /* If the parent node exists and the color of the parent node is red */
    while ((parent = node->parent) && parent->color == RBT_RED) {
        gparent = parent->parent;
 
        /* If the parent node is the left child of the grandparent node */
        if (parent == gparent->left) {
            /* Case 1: Uncle node is red */
            uncle = gparent->right;
            if (uncle && uncle->color == RBT_RED) {
                uncle->color = RBT_BLACK;
                parent->color = RBT_BLACK;
                gparent->color = RBT_RED;
                node = gparent;
                continue;
            }

            /* Case 2: Uncle is black and the current node is the right child */
            if (parent->right == node) {
                rbtree_left_rotate(ptree, parent);
                tmp = parent;
                parent = node;
                node = tmp;
            }
 
            /* Case 3: Uncle is black and the current node is the left child */
            parent->color = RBT_BLACK;
            gparent->color = RBT_RED;
            rbtree_right_rotate(ptree, gparent);

        } else { /* if the parent of z is the right child of the grandparent of z */
            /* Case 1: Uncle node is red */
            uncle = gparent->left;
            if (uncle && uncle->color == RBT_RED) {
                uncle->color = RBT_BLACK;
                parent->color = RBT_BLACK;
                gparent->color = RBT_RED;
                node = gparent;
                continue;
            }
 
            /* Case 2: Uncle is black and the current node is the left child */
            if (parent->left == node) {
                rbtree_right_rotate(ptree, parent);
                tmp = parent;
                parent = node;
                node = tmp;
            }
 
            /* Case 3: Uncle is black and the current node is the right child */
            parent->color = RBT_BLACK;
            gparent->color = RBT_RED;
            rbtree_left_rotate(ptree, gparent);
        }
    }
 
    /* Set the root node to black */
    ptree->root->color = RBT_BLACK;
}

int rbtree_insert (void * vptree, void * key, void * obj, void ** pnode)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;
    rbtnode_t * parent = NULL;
    rbtnode_t * node = NULL;
    rbtnode_t * newnode = NULL;
    int         ret = 0;

    if (!ptree) return -1;

    node = ptree->root;

    while (node != NULL) {
        parent = node;

        if (ptree->alloc_node)
            ret = (*ptree->cmp)(node->obj, key);
        else
            ret = (*ptree->cmp)(node, key);

        if (ret > 0)
            node = node->left;
        else if (ret < 0)
            node = node->right;
        else {
            if (pnode) *pnode = node;
            return 0;
        }
    }

    if (ptree->alloc_node) {
        newnode = rbtnode_alloc(ptree);
        if (!newnode)
            return -100;

        newnode->key = key;
        newnode->obj = obj;
    } else {
        newnode = (rbtnode_t *)obj;
    }

    newnode->parent = parent;
    newnode->left = newnode->right = NULL;
    newnode->color = RBT_RED;

    if (parent != NULL) {
        if (ret >= 0)
            parent->left = newnode;
        else
            parent->right = newnode;
    } else {
        ptree->root = newnode;
    }

    ptree->num++;

    rbtree_insert_fixup(ptree, newnode);

    if (pnode) *pnode = newnode;

    return 1;
}


static void rbtree_delete_fixup (rbtree_t * ptree, rbtnode_t * node, rbtnode_t * parent)
{
    rbtnode_t  * other = NULL;

    while ( (!node || node->color == RBT_BLACK) && node != ptree->root) {
        if (parent->left == node) { //left child
            other = parent->right;
            if (other && other->color == RBT_RED) {
                /* Case 1: x's brother w is red */
                other->color = RBT_BLACK;
                parent->color = RBT_RED;
                rbtree_left_rotate(ptree, parent);
                other = parent->right;
            }

            if (!other) {
                tolog(1, "Panic: rbtree_delete_fixup deleted left black has no right brother, "
                         "tree=%p node=%p parent=%p\n",
                      ptree, node, parent);
            }

            if ( (!other->left || other->left->color == RBT_BLACK) && 
                 (!other->right || other->right->color == RBT_BLACK) ) {
                /* Case 2: x's brother w is black, and both of w's children are black */
                other->color = RBT_RED;
                node = parent;
                parent = node->parent;
            } else {
                if (!other->right || other->right->color == RBT_BLACK) {
                    /* Case 3: x's brother w is black, and w's left child is red and its right child is black */
                    other->left->color = RBT_BLACK;
                    other->color = RBT_RED;
                    rbtree_right_rotate(ptree, other);
                    other = parent->right;
                }

                /* Case 4: x's brother w is black, and w's right child is red, and his left child is any color */
                other->color = parent->color;
                parent->color = RBT_BLACK;
                other->right->color = RBT_BLACK;
                rbtree_left_rotate(ptree, parent);
                node = ptree->root;
                break;
            }
        } else {
            other = parent->left;
            if (other && other->color == RBT_RED) {
                /* Case 1: x's brother w is red */
                other->color = RBT_BLACK;
                parent->color = RBT_RED;
                rbtree_right_rotate(ptree, parent);
                other = parent->left;
            }

            if (!other) {
                tolog(1, "Panic: rbtree_delete_fixup deleted right black has no left brother "
                         "tree=%p node=%p parent=%p\n",
                      ptree, node, parent);
            }

            if ( (!other->left || other->left->color == RBT_BLACK) && 
                 (!other->right || other->right->color == RBT_BLACK) ) {
                /* Case 2: x's brother w is black, and both of w's children are black */
                other->color = RBT_RED;
                node = parent;
                parent = node->parent;
            } else {
                if (!other->left || other->left->color == RBT_BLACK) {
                    /* Case 3: x's brother w is black, and w's right child is red and its left child is black */
                    other->right->color = RBT_BLACK;
                    other->color = RBT_RED;
                    rbtree_left_rotate(ptree, other);
                    other = parent->left;
                }

                /* Case 4: x's brother w is black, and w's left child is red, and its right child is any color */
                other->color = parent->color;
                parent->color = RBT_BLACK;
                other->left->color = RBT_BLACK;
                rbtree_right_rotate(ptree, parent);
                node = ptree->root;
                break;
            }
        }
    }

    if (node) node->color = RBT_BLACK;
}

int rbtree_delete_node (void * vptree, void * vnode)
{
    rbtree_t   * ptree = (rbtree_t *)vptree;
    rbtnode_t  * node = (rbtnode_t *)vnode;
    rbtnode_t  * child = NULL;
    rbtnode_t  * parent = NULL;
    rbtnode_t  * replace = NULL;
    int          color = 0;

    if (!ptree || !node) return -1;

    if (!node->parent && !node->left && !node->right && ptree->root != node) {
        tolog(1, "Panic: rbtree_delete_node parent/left/right are NULL, num=%d tree=%p root=%p node=%p\n",
              ptree->num, ptree, ptree->root, node);
        return -40;
    }

    /* check if node is in rbtree roughly */

    parent = node->parent;

    if (!parent && node != ptree->root) {
        tolog(1, "Panic: rbtree_delete_node parent NULL, num=%d tree=%p root=%p node=%p\n",
              ptree, ptree->num, ptree->root, node);
        return -100;
    }

    if (parent && parent->left != node && parent->right != node) {
        tolog(1, "Panic: rbtree_delete_node is not son of parent, num=%d tree=%p root=%p parent=%p node=%p\n",
              ptree->num, ptree, ptree->root, parent, node);
        return -101;
    }

    if (node->left && node->left->parent != node) {
        tolog(1, "Panic: rbtree_delete_node parent of left son is not self, num=%d tree=%p root=%p node=%p\n",
              ptree->num, ptree, ptree->root, node);
        return -102;
    }

    if (node->right && node->right->parent != node) {
        tolog(1, "Panic: rbtree_delete_node parent of right son is not self, num=%d tree=%p root=%p node=%p\n",
              ptree->num, ptree, ptree->root, node);
        return -103;
    }

    if (node->left != NULL && node->right != NULL) {
        /* Replaces the "deleted node" with the successor of the deleted node, and then removes the "deleted node" */
        replace = node->right;
        while (replace->left != NULL) replace = replace->left;

        if (node->parent != NULL) {
            if (node->parent->left == node) node->parent->left = replace;
            else node->parent->right = replace;
        } else {
            ptree->root = replace;
        }

        child = replace->right;
        parent = replace->parent;
        color = replace->color;

        if (parent == node) {
            parent = replace;
        } else {
            if (child) child->parent = parent;
            parent->left = child;

            replace->right = node->right;
            node->right->parent = replace;
        }

        replace->parent = node->parent;
        replace->color = node->color;
        replace->left = node->left;
        node->left->parent = replace;

        if (color == RBT_BLACK) 
            rbtree_delete_fixup(ptree, child, parent);

        --ptree->num;

        node->parent = node->left = node->right = NULL;
        node->color = node->depth = node->width = 0;

        if (ptree->alloc_node)
            rbtnode_free(ptree, node, NULL);

        return 1;
    }

    if (node->left != NULL) child = node->left;
    else child = node->right;
    parent = node->parent;
    color = node->color;

    if (child) child->parent = parent;

    if (parent) {
        if (parent->left == node) parent->left = child;
        else parent->right = child;
    } else {
        ptree->root = child;
    }

    if (color == RBT_BLACK)
        rbtree_delete_fixup(ptree, child, parent);

    --ptree->num;

    node->parent = node->left = node->right = NULL;
    node->color = node->depth = node->width = 0;

    if (ptree->alloc_node)
        rbtnode_free(ptree, node, NULL);

    return 0;
}

void * rbtree_delete (void * vptree, void * key)
{
    rbtree_t   * ptree = (rbtree_t *)vptree;
    rbtnode_t  * node = NULL;
    void       * obj = NULL;
    int          ret = -5;

    if (!ptree) return NULL;

    node = rbtree_get_node(ptree, key);
    if (node) {
        if (ptree->alloc_node)
            obj = node->obj;

        ret = rbtree_delete_node(ptree, node);
        if (ret >= 0) {
            return ptree->alloc_node ? obj : node;
        }
    }

    if (node)
        tolog(1, "Panic: rbtree_delete ret=%d num=%d tree=%p root=%p node=%p\n",
              ret, ptree->num, ptree, ptree->root, node);

    return NULL;
}
 
void * rbtree_delete_gemin (void * vptree, void * key)
{
    rbtree_t   * ptree = (rbtree_t *)vptree;
    rbtnode_t  * node = NULL;
    void       * obj = NULL;
    int          ret = -5;

    if (!ptree) return NULL;

    node = rbtree_get_node_gemin(ptree, key);
    if (node) {
        if (ptree->alloc_node)
            obj = node->obj;

        ret = rbtree_delete_node(ptree, node);
        if (ret >= 0) {
            return ptree->alloc_node ? obj : node;
        }
    }

    //tolog(1, "Panic: rbtree_delete_gemin ret=%d num=%d\n", ret, ptree->num);

    return NULL;
}


void * rbtree_delete_min (void * vptree)
{
    rbtree_t   * ptree = (rbtree_t *)vptree;
    rbtnode_t  * node = NULL;
    void       * obj = NULL;

    if (!ptree) return NULL;

    node = rbtnode_min(ptree->root);
    if (node) {
        if (ptree->alloc_node)
            obj = node->obj;

        if (rbtree_delete_node(ptree, node) >= 0) {
            return ptree->alloc_node ? obj : node;
        }
    }

    return NULL;
}

void * rbtree_delete_max (void * vptree)
{
    rbtree_t   * ptree = (rbtree_t *)vptree;
    rbtnode_t  * node = NULL;
    void       * obj = NULL;

    if (!ptree) return NULL;

    node = rbtnode_max(ptree->root);
    if (node) {
        if (ptree->alloc_node)
            obj = node->obj;

        if (rbtree_delete_node(ptree, node) >= 0) {
            return ptree->alloc_node ? obj : node;
        }
    }

    return NULL;
}
 

static int rbtree_preorder_node (rbtnode_t * node, rbtcb_t * cb, 
                   void * cbpara, int index, int alloc_node)
{
    rbtnode_t * left = NULL;
    rbtnode_t * right = NULL;

    if (!node) return index;

    left = node->left;
    right = node->right;

    if (cb) {
        if (alloc_node)
            (*cb)(cbpara, node->key, node->obj, index);
        else
            (*cb)(cbpara, node, node, index);
    }

    index++;

    if (left)
        index = rbtree_preorder_node(left, cb, cbpara, index, alloc_node);

    if (node->right)
        index = rbtree_preorder_node(right, cb, cbpara, index, alloc_node);

    return index;
}

int rbtree_preorder (void * vptree, rbtcb_t * cb, void * cbpara)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;

    if (!ptree) return -1;

    return rbtree_preorder_node(ptree->root, cb, cbpara, 0, ptree->alloc_node);
}


static int rbtree_inorder_node (rbtnode_t * node, rbtcb_t * cb,
              void * cbpara, int index, int alloc_node)
{
    rbtnode_t * left = NULL;
    rbtnode_t * right = NULL;

    if (!node) return index;

    left = node->left;
    right = node->right;

    if (left)
        index = rbtree_inorder_node(left, cb, cbpara, index, alloc_node);

    if (cb) {
        if (alloc_node)
            (*cb)(cbpara, node->key, node->obj, index);
        else
            (*cb)(cbpara, node, node, index);
    }

    index++;

    if (right)
        index = rbtree_inorder_node(right, cb, cbpara, index, alloc_node);

    return index;
}


int rbtree_inorder  (void * vptree, rbtcb_t * cb, void * cbpara)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;

    if (!ptree) return -1;

    return rbtree_inorder_node(ptree->root, cb, cbpara, 0, ptree->alloc_node);
}


static int rbtree_postorder_node (rbtnode_t * node, rbtcb_t * cb,
             void * cbpara, int index, int alloc_node)
{
    if (!node) return index;

    if (node->left)
        index = rbtree_postorder_node(node->left, cb, cbpara, index, alloc_node);

    if (node->right)
        index = rbtree_postorder_node(node->right, cb, cbpara, index, alloc_node);

    if (cb) {
        if (alloc_node)
            (*cb)(cbpara, node->key, node->obj, index);
        else
            (*cb)(cbpara, node, node, index);
    }

    index++;

    return index;
}

int rbtree_postorder(void * vptree, rbtcb_t * cb, void * cbpara)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;

    if (!ptree) return -1;

    return rbtree_postorder_node(ptree->root, cb, cbpara, 0, ptree->alloc_node);
}


void rbtnode_set_dimen (void * vptree, void * vnode, int depth, int width)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;
    rbtnode_t * node = (rbtnode_t *)vnode;

    if (!node) return;

    node->depth = depth;
    node->width = width;

    if (ptree) {
        if (ptree->depth < depth)
            ptree->depth = depth;

        if (depth < sizeof(ptree->layer)/sizeof(int))
            ptree->layer[depth]++;

        if (width < 0 && ptree->lwidth > width)
            ptree->lwidth = width;

        if (width > 0 && ptree->rwidth < width)
            ptree->rwidth = width;
    }

    if (node->left)
        rbtnode_set_dimen(ptree, node->left, depth+1, width-1);

    if (node->right)
        rbtnode_set_dimen(ptree, node->right, depth+1, width+1);
}
 
void rbtree_set_dimen (void * vptree)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;
 
    if (!ptree) return;

    ptree->depth = 0;
    memset(ptree->layer, 0, sizeof(ptree->layer));
    ptree->lwidth = 0;
    ptree->rwidth = 0;
 
    rbtnode_set_dimen(ptree, ptree->root, 0, 0);
}

