/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include "btype.h"
#include "memory.h"
#include "arfifo.h"
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


/* 找结点node的前驱结点predecessor, 即查找红黑树中数据值小于该结点的最大结点 */
void * rbtnode_prev (void * vnode)
{
    rbtnode_t  * node = (rbtnode_t *)vnode;
    rbtnode_t  * prev = NULL;

    if (!node)
        return NULL;
 
    /* 如果node存在左孩子，则其前驱结点为 "以其左孩子为根的子树的最大结点" */

    if (node->left != NULL)
        return rbtnode_max(node->left);

    /* 如果node没有左孩子。则有以下两种可能：
     * (1)node是一个右孩子，则其前驱结点为它的父结点
     * (2)node是一个左孩子，则查找node的最低的父结点，并且该父结点要具有右孩子，
     *    找到的这个"最低的父结点"就是"node的前驱结点 */

    prev = node->parent;

    while (prev != NULL && node == prev->left) {
        node = prev;
        prev = prev->parent;
    }
 
    return prev;
}

/* 找结点node的后继结点successor, 即查找红黑树中数据值大于该结点的最小结点 */
void * rbtnode_next (void * vnode)
{
    rbtnode_t  * node = (rbtnode_t *)vnode;
    rbtnode_t  * next = NULL;
 
    if (!node) return NULL;
 
    /* 如果node存在右孩子，则其后继结点为 "以其右孩子为根的子树的最小结点" */

    if (node->right != NULL)
        return rbtnode_min(node->right);
 
    /* 如果node没有右孩子。则有以下两种可能：
     * (1)node是一个左孩子，则其后继结点为它的父结点
     * (2)node是一个右孩子，则查找node的最低的父结点，并且该父结点要具有左孩子，
     *    找到的这个"最低的父结点"就是"node的后继结点 */

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
 
    if (!node|| !key)
        return NULL;
 
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



void * rbtnode_alloc ()
{
    rbtnode_t * node = NULL;
 
    node = kzalloc(sizeof(*node));
    return node;
}


int rbtnode_free (void * vnode, rbtfree_t * freefunc, int alloc_node)
{
    rbtnode_t * node = (rbtnode_t *)vnode;

    if (node != NULL) {

        if (alloc_node) {
            if (freefunc)
                (*freefunc)(node->obj);

            kfree(node);

        } else {

            if (freefunc)
                (*freefunc)(node);
        }
    }

    return 0;
}


static void rbtree_free_node (rbtnode_t * node, rbtfree_t * freefunc, int alloc_node)
{
    if (!node) return;

    if (node->left)
        rbtree_free_node(node->left, freefunc, alloc_node);

    if (node->right)
        rbtree_free_node(node->right, freefunc, alloc_node);

    rbtnode_free(node, freefunc, alloc_node);
}

/* alloc_node = 1 means that:
          the caller's data object contains no space for rbtree-node pointer.
          when inserting a new node, an memory block for rbtnode_t will be allocated
          and one of member pointers is assigned the caller's data object.
   alloc_node = 0 means:
          the caller's data object must reserved 40-bytes at the begining for rbtnode_t.
*/

void * rbtree_new (rbtcmp_t * cmp, int alloc_node)
{
    rbtree_t * rbt = NULL;

    rbt = kzalloc(sizeof(*rbt));
    if (!rbt) return rbt;

    rbt->root = NULL;
    rbt->num = 0;
    rbt->cmp = cmp;
    rbt->alloc_node = alloc_node;

    rbt->depth = 0;
    memset(rbt->layer, 0, sizeof(rbt->layer));
    rbt->lwidth = 0;
    rbt->rwidth = 0;

    return rbt;
}

void rbtree_free (void * vptree)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;

    if (!ptree) return;

    if (ptree->alloc_node)
        rbtree_free_node(ptree->root, NULL, ptree->alloc_node);

    kfree(ptree);
}
 

void rbtree_free_all (void * vptree, rbtfree_t * freefunc)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;

    if (!ptree) return;

    rbtree_free_node(ptree->root, freefunc, ptree->alloc_node);

    kfree(ptree);
}
 
 
void rbtree_zero (void * vptree)
{
    rbtree_t  * rbt = (rbtree_t *)vptree;

    if (!rbt) return;

    if (rbt->alloc_node)
        rbtree_free_node(rbt->root, NULL, rbt->alloc_node);

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

    if (!ptree || !key)
        return NULL;

    node = ptree->root;

    while (node != NULL) {
        if (ptree->alloc_node)
            ret = (*ptree->cmp)(node->obj, key);
        else
            ret = (*ptree->cmp)(node, key);

        if (ret == 0)
            return node;

        if (ret > 0)
            node = node->left;
        else
            node = node->right;
    }

    return NULL;
}

void * rbtree_get (void * vptree, void * key)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;
    rbtnode_t * node = NULL;
    int         ret = 0;

    if (!ptree || !key)
        return NULL;

    node = ptree->root;

    while (node != NULL) {
        if (ptree->alloc_node)
            ret = (*ptree->cmp)(node->obj, key);
        else
            ret = (*ptree->cmp)(node, key);

        if (ret == 0) 
            return ptree->alloc_node ? node->obj : node;

        if (ret > 0)
            node = node->left;
        else
            node = node->right;
    }

    return NULL;
}


int rbtree_mget_node (void * vptree, void * key, void ** plist, int listsize)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;
    rbtnode_t * node = NULL;
    rbtnode_t * iter = NULL;
    int         ret = 0;
    int         num = 0;
 
    if (!ptree || !key) return -1;
 
    node = rbtree_get_node(ptree, key);
    if (!node) return 0;

    if (plist && num < listsize)
        plist[num] = node;
    num++;

    for (iter = node; iter != NULL; ) {
        iter = rbtnode_prev(iter);
        if (!iter) break;

        if (ptree->alloc_node)
            ret = (*ptree->cmp)(iter->obj, key);
        else
            ret = (*ptree->cmp)(iter, key);

        if (ret != 0) break;

        if (plist && num < listsize)
            plist[num] = iter;
        num++;
    }

    for (iter = node; iter != NULL; ) {
        iter = rbtnode_next(iter);
        if (!iter) break;

        if (ptree->alloc_node)
            ret = (*ptree->cmp)(iter->obj, key);
        else
            ret = (*ptree->cmp)(iter, key);

        if (ret != 0) break;
     
        if (plist && num < listsize)
            plist[num] = iter;
        num++;
    }

    return num;
}

int rbtree_mget (void * vptree, void * key, void ** plist, int listsize)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;
    rbtnode_t * node = NULL;
    rbtnode_t * iter = NULL;
    int         ret = 0;
    int         num = 0;
 
    if (!ptree || !key) return -1;
 
    node = rbtree_get_node(ptree, key);
    if (!node) return 0;

    if (plist && num < listsize)
        plist[num] = ptree->alloc_node ? node->obj : node;
    num++;

    for (iter = node; iter != NULL; ) {
        iter = rbtnode_prev(iter);
        if (!iter) break;

        if (ptree->alloc_node)
            ret = (*ptree->cmp)(iter->obj, key);
        else
            ret = (*ptree->cmp)(iter, key);

        if (ret != 0) break;

        if (plist && num < listsize)
            plist[num] = ptree->alloc_node ? iter->obj : iter;
        num++;
    }

    for (iter = node; iter != NULL; ) {
        iter = rbtnode_next(iter);
        if (!iter) break;

        if (ptree->alloc_node)
            ret = (*ptree->cmp)(iter->obj, key);
        else
            ret = (*ptree->cmp)(iter, key);

        if (ret != 0) break;
     
        if (plist && num < listsize)
            plist[num] = ptree->alloc_node ? iter->obj : iter;
        num++;
    }

    return num;
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


/* 对红黑树的节点(x)进行左旋转
 *      px                              px
 *     /                               /
 *    x                               y
 *   /  \      --(左旋)-->           / \                #
 *  lx   y                          x  ry
 *     /   \                       /  \
 *    ly   ry                     lx  ly   
 */
static void rbtree_left_rotate (rbtree_t * ptree, rbtnode_t * x)
{
    rbtnode_t  * y = NULL;

    if (!ptree || !x) return;

    y = x->right;

    /* 将y的左孩子设为x的右孩子, 如果y的左孩子非空，将x设为y的左孩子的父亲 */
    x->right = y->left;
    if (y->left != NULL)
        y->left->parent = x;
 
    /* 将x的父亲设为y的父亲 */
    y->parent = x->parent;
    if (x->parent == NULL) {
        /* 如果x的父亲是空节点，则将y设为根节点 */
        ptree->root = y;
    } else {
        if (x->parent->left == x) {
            /* 如果x是它父节点的左孩子，则将y设为x的父节点的左孩子 */
            x->parent->left = y;
        } else {
            /* 如果x是它父节点的右孩子，则将y设为x的父节点的右孩子 */
            x->parent->right = y;
        }
    }
 
    /* 将x设为y的左孩子 */
    y->left = x;

    /* 将x的父节点设为y */
    x->parent = y;
}

/* 对红黑树的节点y进行右旋转
 *            py                               py
 *           /                                /
 *          y                                x
 *         /  \      --(右旋)-->            /  \                     #
 *        x   ry                           lx   y
 *       / \                                   / \                   #
 *      lx  rx                                rx  ry
 */
static void rbtree_right_rotate (rbtree_t * ptree, rbtnode_t * y)
{
    rbtnode_t * x = NULL;

    if (!ptree || !y) return;
 
    x = y->left;

    /* 将x的右孩子设为y的左孩子, 如果x的右孩子非空，将y设为x的右孩子的父亲 */
    y->left = x->right;
    if (x->right != NULL)
        x->right->parent = y;
 
    /* 将y的父亲设为x的父亲 */
    x->parent = y->parent;
    if (y->parent == NULL) {
        /* 如果y的父亲是空节点，则将x设为根节点 */
        ptree->root = x;
    } else {
        if (y == y->parent->right) {
            /* 如果y是它父节点的右孩子，则将x设为y的父节点的右孩子 */
            y->parent->right = x;
        } else {
            /* 如果y是它父节点的左孩子, 将x设为y的父节点的左孩子 */
            y->parent->left = x;
        }
    }
 
    /* 将y设为x的右孩子 */
    x->right = y;
 
    /* 将y的父节点设为x */
    y->parent = x;
}

static void rbtree_insert_fixup (rbtree_t * ptree, rbtnode_t * node)
{
    rbtnode_t  * parent = NULL;
    rbtnode_t  * gparent = NULL;
    rbtnode_t  * uncle = NULL;
    rbtnode_t  * tmp = NULL;

    if (!ptree || !node) return;

    /* 若父节点存在, 并且父节点的颜色是红色 */
    while ((parent = node->parent) && parent->color == RBT_RED) {
        gparent = parent->parent;
 
        /* 若父节点是祖父节点的左孩子 */
        if (parent == gparent->left) {
            /* Case 1条件：叔叔节点是红色 */
            uncle = gparent->right;
            if (uncle && uncle->color == RBT_RED) {
                uncle->color = RBT_BLACK;
                parent->color = RBT_BLACK;
                gparent->color = RBT_RED;
                node = gparent;
                continue;
            }

            /* Case 2条件：叔叔是黑色，且当前节点是右孩子 */
            if (parent->right == node) {
                rbtree_left_rotate(ptree, parent);
                tmp = parent;
                parent = node;
                node = tmp;
            }
 
            /* Case 3条件：叔叔是黑色，且当前节点是左孩子 */
            parent->color = RBT_BLACK;
            gparent->color = RBT_RED;
            rbtree_right_rotate(ptree, gparent);

        } else { /* 若z的父节点是z的祖父节点的右孩子 */
            /* Case 1条件：叔叔节点是红色 */
            uncle = gparent->left;
            if (uncle && uncle->color == RBT_RED) {
                uncle->color = RBT_BLACK;
                parent->color = RBT_BLACK;
                gparent->color = RBT_RED;
                node = gparent;
                continue;
            }
 
            /* Case 2条件：叔叔是黑色，且当前节点是左孩子 */
            if (parent->left == node) {
                rbtree_right_rotate(ptree, parent);
                tmp = parent;
                parent = node;
                node = tmp;
            }
 
            /* Case 3条件：叔叔是黑色，且当前节点是右孩子 */
            parent->color = RBT_BLACK;
            gparent->color = RBT_RED;
            rbtree_left_rotate(ptree, gparent);
        }
    }
 
    /* 将根节点设为黑色 */
    ptree->root->color = RBT_BLACK;
}

int rbtree_insert (void * vptree, void * key, void * obj, void ** pnode)
{
    rbtree_t  * ptree = (rbtree_t *)vptree;
    rbtnode_t * parent = NULL;
    rbtnode_t * node = NULL;
    rbtnode_t * newnode = NULL;
    int         ret = 0;

    if (!ptree || !key || !obj)
        return -1;

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

            if (ptree->alloc_node) {
                if (node->obj == obj) return 0;
            } else {
                if (node == obj) return 0;
            }

            node = node->left;
        }
    }

    if (ptree->alloc_node) {
        newnode = rbtnode_alloc();
        if (!newnode)
            return -100;

        newnode->key = key;
        newnode->obj = obj;
    } else {
        newnode = (rbtnode_t *)obj;
    }

    newnode->parent = parent;
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
                /* Case 1: x的兄弟w是红色的 */
                other->color = RBT_BLACK;
                parent->color = RBT_RED;
                rbtree_left_rotate(ptree, parent);
                other = parent->right;
            }

            if ( (!other->left || other->left->color == RBT_BLACK) && 
                 (!other->right || other->right->color == RBT_BLACK) ) {
                /* Case 2: x的兄弟w是黑色，且w的俩个孩子也都是黑色的 */
                other->color = RBT_RED;
                node = parent;
                parent = node->parent;
            } else {
                if (!other->right || other->right->color == RBT_BLACK) {
                    /* Case 3: x的兄弟w是黑色的，并且w的左孩子是红色，右孩子为黑色 */
                    other->left->color = RBT_BLACK;
                    other->color = RBT_RED;
                    rbtree_right_rotate(ptree, other);
                    other = parent->right;
                }

                /* Case 4: x的兄弟w是黑色的；并且w的右孩子是红色的，左孩子任意颜色 */
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
                /* Case 1: x的兄弟w是红色的 */
                other->color = RBT_BLACK;
                parent->color = RBT_RED;
                rbtree_right_rotate(ptree, parent);
                other = parent->left;
            }

            if ( (!other->left || other->left->color == RBT_BLACK) && 
                 (!other->right || other->right->color == RBT_BLACK) ) {
                /* Case 2: x的兄弟w是黑色，且w的俩个孩子也都是黑色的 */
                other->color = RBT_RED;
                node = parent;
                parent = node->parent;
            } else {
                if (!other->left || other->left->color == RBT_BLACK) {
                    /* Case 3: x的兄弟w是黑色的，并且w的左孩子是红色，右孩子为黑色 */
                    other->right->color = RBT_BLACK;
                    other->color = RBT_RED;
                    rbtree_left_rotate(ptree, other);
                    other = parent->left;
                }

                /* Case 4: x的兄弟w是黑色的；并且w的右孩子是红色的，左孩子任意颜色 */
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

    /* check if node is in rbtree roughly */

    parent = node->parent;

    if (!parent && node != ptree->root)
         return -100;

    else if (parent && parent->left != node && parent->right != node)
         return -101;

    if (node->left != NULL && node->right != NULL) {
        /* 被删节点的后继节点, 用它来取代"被删节点"的位置，然后再将"被删节点"去掉 */
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
            rbtnode_free(node, NULL, ptree->alloc_node);

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
        rbtnode_free(node, NULL, ptree->alloc_node);

    return 0;
}

void * rbtree_delete (void * vptree, void * key)
{
    rbtree_t   * ptree = (rbtree_t *)vptree;
    rbtnode_t  * node = NULL;
    void       * obj = NULL;

    if (!ptree || !key) return NULL;

    node = rbtree_get_node(ptree, key);
    if (node) {
        if (ptree->alloc_node)
            obj = node->obj;

        if (rbtree_delete_node(ptree, node) >= 0) {
            return ptree->alloc_node ? obj : node;
        }
    }

    return NULL;
}
 
int rbtree_mdelete (void * vptree, void * key, void ** plist, int listnum)
{
    rbtree_t   * ptree = (rbtree_t *)vptree;
    rbtnode_t  * node = NULL;
    int          num = 0;
    void       * obj = NULL;
 
    if (!ptree || !key) return -1;
 
    while ((node = rbtree_get_node(ptree, key)) != NULL) {
        if (ptree->alloc_node)
            obj = node->obj;

        if (rbtree_delete_node(ptree, node) >= 0) {
            if (plist && num < listnum)
                plist[num] = ptree->alloc_node ? obj : node;
            num++;
        } else
            break;
    }
 
    return num;
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
    if (node != NULL) {
        if (cb) {
            if (alloc_node)
                (*cb)(cbpara, node->key, node->obj, index);
            else
                (*cb)(cbpara, node, node, index);
        }
        index++;

        if (node->left)
            index = rbtree_preorder_node(node->left, cb, cbpara, index, alloc_node);

        if (node->right)
            index = rbtree_preorder_node(node->right, cb, cbpara, index, alloc_node);
    }

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
    if (node != NULL) {
        if (node->left)
            index = rbtree_inorder_node(node->left, cb, cbpara, index, alloc_node);

        if (cb) {
            if (alloc_node)
                (*cb)(cbpara, node->key, node->obj, index);
            else
                (*cb)(cbpara, node, node, index);
        }
        index++;

        if (node->right)
            index = rbtree_inorder_node(node->right, cb, cbpara, index, alloc_node);
    }
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
    if (node != NULL) {
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
    }
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

    if (node != NULL) {
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

