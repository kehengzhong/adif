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

#include "kemalloc.h"
#include "memory.h"
#include "rbtree.h"
#include "strutil.h"
#include "trace.h"


typedef struct kem_unit {
    void             * res[4];
    void             * rbnode[4];

    long               pos;   //actual offset of memory unit
    int                len;   //actual length of memory unit
    int                size;  //caller-requested size, size <= len

#ifdef _KEMDBG
    char               file[36];
    int                line;
#endif
} KemUnit;


int power_exponent_of_2 (int n)
{
    int cnt = 0;

    if (n <= 0) return 0;

    if ((n & (n-1)) == 0) n--;

    while (n != 0) {
        n >>= 1;
        cnt++;
    }

    return cnt;
}

int kemunit_cmp_kemunit (void * a, void * b)
{
    KemUnit * mua = (KemUnit *)a;
    KemUnit * mub = (KemUnit *)b;

    if (!mua && !mub) return 0;
    if (!mua) return -1;
    if (!mub) return 1;

    if (mua->pos < mub->pos) return -1;
    if (mua->pos > mub->pos) return 1;
    return 0;
}

int kemunit_cmp_pos (void * a, void * b)
{
    KemUnit * mu = (KemUnit *)a;
    long      pos = (long)b;

    return mu->pos - pos;
}

int kemunit_cmp_kemunit_by_len (void * a, void * b)
{
    KemUnit * mua = (KemUnit *)structfrom(a, KemUnit, rbnode);
    KemUnit * mub = (KemUnit *)b;

    if (!mua && !mub) return 0;
    if (!mua) return -1;
    if (!mub) return 1;

    if (mua->len < mub->len) return -1;
    if (mua->len > mub->len) return 1;

    if (mua->pos < mub->pos) return -1;
    if (mua->pos > mub->pos) return 1;
    return 0;
}


int kemunit_init (KemUnit * unit)
{
    if (!unit) return -1;

    unit->pos = 0;
    unit->len = 0;
    unit->size = 0;
    return 0;
}

KemUnit * kemunit_alloc (KemBlk * blk, long pos, int len, int size)
{
    KemUnit * unit = NULL;

    if (!blk) return NULL;

    if (blk->kemunit_pool) {
        unit = mpool_fetch(blk->kemunit_pool);
    } else if (blk->alloctype == 1) {
        unit = kosmalloc(sizeof(*unit));
    } else {
        unit = kalloc(sizeof(*unit));
    }
    if (!unit) return NULL;

    unit->res[0] = unit->res[1] = unit->res[2] = unit->res[3] = NULL;
    unit->rbnode[0] = unit->rbnode[1] = unit->rbnode[2] = unit->rbnode[3] = NULL;

    unit->pos = pos;
    unit->len = len; //align_size(size, sizeof(void *));
    unit->size = size;

#ifdef _KEMDBG
    unit->file[0] = '\0';
    unit->line = 0;
#endif

    return unit;
}

int kemunit_free (KemBlk * blk, KemUnit * unit)
{
    if (!blk) return -1;
    if (!unit) return -2;

    if (blk->kemunit_pool) {
        mpool_recycle(blk->kemunit_pool, unit);
    } else if (blk->alloctype == 1) {
        kosfree(unit);
    } else {
        kfree(unit);
    }

    return 0;
}

int kemunit_traverse_cb (void * vblk, void * key, void * obj, int index)
{
    KemBlk  * blk = (KemBlk *)vblk;
    KemUnit * unit = (KemUnit *)obj;

    return kemunit_free(blk, unit);
}


int kemblk_cmp_kemblk (void * a, void * b)
{
    KemBlk  * blka = (KemBlk *)a;
    KemBlk  * blkb = (KemBlk *)b;

    if (!blka) return -1;
    if (!blkb) return 1;

    if (blka->pbgn > blkb->pbgn) return 1;
    if (blka->pbgn < blkb->pbgn) return -1;
    return 0;
}

int kemblk_cmp_p (void * a, void * b)
{
    KemBlk  * blk = (KemBlk *)a;
    uint8   * pmem = (uint8 *)b;

    if (blk->pbgn > pmem) return 1;
    if (blk->pbgn + blk->actsize < pmem) return -1;

    return 0;
}

int kemblk_init_member (void * vblk)
{
    KemBlk  * blk = (KemBlk *)vblk;
    KemUnit * unit = NULL;

    if (!blk) return -1;

    InitializeCriticalSection(&blk->memCS);

    InitializeCriticalSection(&blk->alloctreeCS);
    InitializeCriticalSection(&blk->idletreeCS);

    blk->alloc_tree = &blk->tree_mem[0];
    rbtree_init(blk->alloc_tree, kemunit_cmp_pos, 0, 0, NULL, NULL);

    blk->pos_idle_tree = &blk->tree_mem[1];
    rbtree_init(blk->pos_idle_tree, kemunit_cmp_pos, 0, 0, NULL, NULL);

    blk->size_idle_tree = &blk->tree_mem[2];
    rbtree_init(blk->size_idle_tree, kemunit_cmp_kemunit_by_len, 0, 0, NULL, NULL);

    blk->pbgn = align_ptr(blk->pmem, sizeof(void *));

    blk->actsize = blk->size - (blk->pbgn - (uint8 *)blk); //blk->pmem + blk->size - blk->pbgn;

    unit = kemunit_alloc(blk, 0, blk->actsize, 0);

    rbtree_insert(blk->pos_idle_tree, (void *)0, unit, NULL);
    rbtree_insert(blk->size_idle_tree, unit, &unit->rbnode[0], NULL);

    blk->allocsize = 0;
    blk->restsize = blk->actsize;
    blk->allocnum = 0;

    return 0;
}

int kemblk_init (void * vmp, void * vblk, long size, int mpunit)
{
    KemPool * mp = (KemPool *)vmp;
    KemBlk  * blk = (KemBlk *)vblk;

    if (!blk) return -2;

    memset(blk, 0, sizeof(*blk));

    blk->stamp = time(0);
    blk->size = size; //align_size(size, sizeof(void *));
    blk->allocflag = 0;  //indicates blk is not allocated

    if (mp) {
        blk->alloctype = mp->alloctype;
        if (mpunit) 
            blk->kemunit_pool = mp->kemunit_pool;
    } else {
        blk->alloctype = 0; //kalloc/kfree
    }

    blk->kmpool = mp;

    return kemblk_init_member(blk);
}

void * kemblk_alloc (void * vmp, long size)
{
    KemPool * mp = (KemPool *)vmp;
    KemBlk  * blk = NULL;

    if (!mp) return NULL;

    if (mp->alloctype == 1) {
        blk = kosmalloc(sizeof(*blk) + size);
    } else {
        blk = kalloc(sizeof(*blk) + size);
    }
    if (!blk) return NULL;

    memset(blk, 0, sizeof(*blk));

    blk->stamp = time(0);
    blk->size = size + sizeof(*blk); //align_size(size, sizeof(void *));
    blk->allocflag = 1;  //indicates blk is allocated

    blk->alloctype = mp->alloctype;
    blk->kemunit_pool = mp->kemunit_pool;

    blk->kmpool = mp;

    kemblk_init_member(blk);

    return blk;
}

void kemblk_free_dbg (void * vblk, char * file, int line)
{
    KemBlk    * blk = (KemBlk *)vblk;
    KemUnit   * unit = NULL;
    rbtnode_t * rbt = NULL;
    int         i, num;

    if (!blk) return;

    EnterCriticalSection(&blk->alloctreeCS);

    num = rbtree_num(blk->alloc_tree);
    if (num > 0) {
        tolog(1, "Panic: %d unfreed memory when kemblk_free %p allocnum=%d idlenum=%d %s:%d\n",
              num, blk->pbgn, blk->allocnum, rbtree_num(blk->pos_idle_tree), file, line);

        rbt = rbtree_min_node(blk->alloc_tree);
        for (i = 0; i < num && rbt; i++) {
            unit = RBTObj(rbt); rbt = rbtnode_next(rbt);
            if (!unit) continue;

#ifdef _KEMDBG
            tolog(0, "  No.%d/%d: unfreed memory %p size=%d len=%d pos=%ld %s:%d\n",
                  i, num, blk->pbgn + unit->pos, unit->size, unit->len,
                  unit->pos, unit->file, unit->line);
#else
            tolog(0, "  No.%d/%d: unfreed memory %p size=%d len=%d pos=%ld\n",
                  i, num, blk->pbgn + unit->pos, unit->size, unit->len, unit->pos);
#endif
        }        
    }

    rbtree_inorder(blk->alloc_tree, kemunit_traverse_cb, blk);
    rbtree_free(blk->alloc_tree);

    LeaveCriticalSection(&blk->alloctreeCS);

    DeleteCriticalSection(&blk->alloctreeCS);

    EnterCriticalSection(&blk->idletreeCS);

    rbtree_inorder(blk->pos_idle_tree, kemunit_traverse_cb, blk);
    rbtree_free(blk->pos_idle_tree);

    rbtree_free(blk->size_idle_tree);

    LeaveCriticalSection(&blk->idletreeCS);

    DeleteCriticalSection(&blk->idletreeCS);

    if (blk->allocflag) {
        if (blk->alloctype == 1) kosfree(blk);
        else kfree(blk);
    }
}

int kemblk_brief (void * vblk, char * buf, int size)
{
    KemBlk * blk = (KemBlk *)vblk;

    if (!blk) return -1;
    if (!buf) return -2;

    return snprintf(buf, size-1, "%p size=%ld actsize=%ld alloctype=%d allocnum=%d allocsize=%ld restsize=%ld "
                          "alloctreenum=%d sizetreenum=%d postreenum=%d pbgn=%p\n",
             blk, blk->size, blk->actsize, blk->alloctype, blk->allocnum, blk->allocsize,
             blk->restsize, rbtree_num(blk->alloc_tree), rbtree_num(blk->size_idle_tree),
             rbtree_num(blk->pos_idle_tree), blk->pbgn);
}

int kemblk_idleunit_exist (KemBlk * blk, int size)
{
    KemUnit * iter = NULL;
    KemUnit   node;
    void    * ptr = NULL;

    if (!blk || size <= 0) return 0;

    EnterCriticalSection(&blk->idletreeCS);

    node.len = align_size(size, sizeof(void *));
    node.pos = 0;
    ptr = rbtree_get_gemin(blk->size_idle_tree, &node);
    if (ptr) iter = structfrom(ptr, KemUnit, rbnode);

    LeaveCriticalSection(&blk->idletreeCS);

    if (iter) return 1;

    return 0;
}

void * kemblk_idleunit_delete (KemBlk * blk, int size)
{
    KemUnit * iter = NULL;
    KemUnit   node;
    void    * ptr = NULL;

    if (!blk || size <= 0) return NULL;

    EnterCriticalSection(&blk->idletreeCS);

    node.len = align_size(size, sizeof(void *));
    node.pos = 0;
    ptr = rbtree_delete_gemin(blk->size_idle_tree, &node);
    if (ptr) iter = structfrom(ptr, KemUnit, rbnode);

    LeaveCriticalSection(&blk->idletreeCS);

    return iter;
}

void * kemblk_alloc_unit_dbg (void * vblk, int size, char * file, int line)
{
    KemBlk  * blk = (KemBlk *)vblk;
    KemUnit * iter = NULL;
    KemUnit * unit = NULL;
    KemUnit   node;
    void    * ptr = NULL;
    int       len;

    if (!blk || size < 0) // || rbtree_num(blk->size_idle_tree) <= 0)
        return NULL;

    len = align_size(size, sizeof(void *));
    if (len < 8) len = 8;

    EnterCriticalSection(&blk->idletreeCS);

    node.len = len; node.pos = 0;
    ptr = rbtree_delete_gemin(blk->size_idle_tree, &node);
    if (ptr == NULL) {
        LeaveCriticalSection(&blk->idletreeCS);
        return NULL;
    }
    iter = structfrom(ptr, KemUnit, rbnode);

    if (iter->pos >= blk->actsize|| iter->pos < 0) {
        tolog(1, "Panic: kemblk_alloc_unit error pos=%d size=%d len=%d >= actsize=%d %s:%d\n",
              iter->pos, iter->size, iter->len, blk->actsize, file, line);
    }

    if (iter->len <= len + 8) {
        rbtree_delete(blk->pos_idle_tree, (void *)iter->pos);

        LeaveCriticalSection(&blk->idletreeCS);

        iter->size = size;

        EnterCriticalSection(&blk->alloctreeCS);
        rbtree_insert(blk->alloc_tree, (void *)iter->pos, iter, NULL);
        blk->allocnum++;
        LeaveCriticalSection(&blk->alloctreeCS);

        EnterCriticalSection(&blk->memCS);
        blk->allocsize += iter->len;
        blk->restsize -= iter->len;
        blk->stamp = time(0);
        LeaveCriticalSection(&blk->memCS);

#ifdef _KEMDBG
        iter->line = line;
        str_ncpy(iter->file, file, sizeof(iter->file)-1);
#endif  
        return blk->pbgn + iter->pos;

    } else { // len + 8 < iter->len
        /* split the idle unit into 2 parts */
        unit = kemunit_alloc(blk, iter->pos, len, size);

        iter->pos += len;
        iter->len -= len;

        /* insert the remaining split memory unit into the size_idle_tree. */
        rbtree_insert(blk->size_idle_tree, iter, &iter->rbnode[0], NULL);

        LeaveCriticalSection(&blk->idletreeCS);

        /* insert the split memory unit for requesting allocation into the alloc_tree. */
        EnterCriticalSection(&blk->alloctreeCS);
        rbtree_insert(blk->alloc_tree, (void *)unit->pos, unit, NULL);
        blk->allocnum++;
        LeaveCriticalSection(&blk->alloctreeCS);

        EnterCriticalSection(&blk->memCS);
        blk->allocsize += len;
        blk->restsize -= len;
        blk->stamp = time(0);
        LeaveCriticalSection(&blk->memCS);

#ifdef _KEMDBG
        unit->line = line;
        str_ncpy(unit->file, file, sizeof(unit->file)-1);
#endif  
        return blk->pbgn + unit->pos;
    }

    LeaveCriticalSection(&blk->idletreeCS);

    tolog(1, "Panic: AllocUnit %d from KemBlk %p pbgn=%p blksize=%ld allocsize=%ld restsize=%ld "
             "allocnum=%d blkactsize=%ldfailed!\n",
          size, blk, blk->pbgn, blk->size, blk->allocsize, blk->restsize,
          blk->allocnum, blk->actsize);

    return NULL;
}

int kemblk_free_unit_dbg (void * vblk, void * p, char * file, int line)
{
    KemBlk    * blk = (KemBlk *)vblk;
    KemUnit   * unit = NULL;
    KemUnit   * prev = NULL;
    KemUnit   * next = NULL;
    rbtnode_t * rbtn = NULL;
    rbtnode_t * rbtn1 = NULL;
    rbtnode_t * rbtn2 = NULL;
    long        pos = 0;
    int         len = 0;

    if (!blk) return -1;
    if (!p) return -2;

    pos = (uint8 *)p - blk->pbgn;
    if (pos < 0 || pos > blk->actsize) {
        if (blk->allocflag) 
            tolog(1, "Panic: failed to free %p to KemBlk pos=%ld %p pbgn=%p size=%ld actsize=%ld "
                 "allocsize=%ld restsize=%ld allocnum=%d! %s:%d\n",
              p, pos, blk, blk->pbgn, blk->size, blk->actsize, blk->allocsize,
              blk->restsize, blk->allocnum, file, line);
        return -100;
    }

    EnterCriticalSection(&blk->alloctreeCS);
    unit = rbtree_delete(blk->alloc_tree, (void *)pos);
    if (unit) blk->allocnum--;
    LeaveCriticalSection(&blk->alloctreeCS);
    if (!unit) {
        tolog(1, "Panic: FreeUnit pos=%ld not exist! %p pbgn=%p size=%ld allocsize=%ld "
                 "restsize=%ld allocnum=%d! %s:%d\n",
              pos, blk, blk->pbgn, blk->size, blk->allocsize,
              blk->restsize, blk->allocnum, file, line);
        return -200;
    }

    EnterCriticalSection(&blk->memCS);
    blk->allocsize -= unit->len;
    blk->restsize += unit->len;
    blk->stamp = time(0);
    LeaveCriticalSection(&blk->memCS);

    pos = unit->pos;
    len = unit->len;

    /* add to idle tree and merge the contiguous memory unit */

    EnterCriticalSection(&blk->idletreeCS);

    rbtn = rbtree_get_node_pn(blk->pos_idle_tree, (void *)pos, (void **)&rbtn1, (void **)&rbtn2);
    if (!rbtn) {
        if (rbtn1 && (prev = RBTObj(rbtn1)) && prev->pos + prev->len == pos) { //exist an adjacent previous unit
            /* because len of the previous memory unit 'prev' is to be changed due
               to the merging of the current memory unit, it needs to be deleted
               from size_idle_tree. */
            rbtree_delete(blk->size_idle_tree, prev);

            /* if the current unit is closely adjacent to the previous one, 
               merge the current unit to previous one */
            prev->len = pos + len - prev->pos;

            /* reinsert the updated previous memory unit 'prev' into size_idle_tree */
            rbtree_insert(blk->size_idle_tree, prev, &prev->rbnode[0], NULL);

            pos = prev->pos;
            len = prev->len;

            kemunit_free(blk, unit);
            unit = NULL;
        }

        if (rbtn2 && (next = RBTObj(rbtn2)) && pos + len == next->pos) {
            /* because len and pos of the next memory unit 'next' have changed due
               to the merging of the current memory unit, it needs to be deleted
               from size_idle_tree. */
            rbtree_delete(blk->size_idle_tree, next);

            /* if the previous unit is closely adjacent to current unit and next unit */
            if (prev && prev->pos == pos) {
                /* delete the previous memory unit 'prev' from pos_idle_tree */
                rbtree_delete(blk->pos_idle_tree, (void *)prev->pos);

                /* delete the previous memory unit 'prev' from size_idle_tree */
                rbtree_delete(blk->size_idle_tree, prev);

                kemunit_free(blk, prev);
                prev = NULL;
            }

            /* current unit is adjacent to the next unit, merge it into next unit */
            next->len += next->pos - pos;
            next->pos = pos;

            /* reinsert the updated next memory unit 'next' into size_idle_tree */
            rbtree_insert(blk->size_idle_tree, next, &next->rbnode[0], NULL);

            if (unit) {
                kemunit_free(blk, unit);
                unit = NULL;
            }

        } else {
            /* if current unit is not adjacent to the next unit */
            if (unit) {
                /* the current memory unit is not adjacent to any unit in the idle
                   memory unit tree. insert it into the two idle memory unit trees.*/
                rbtree_insert(blk->size_idle_tree, unit, &unit->rbnode[0], NULL);
                rbtree_insert(blk->pos_idle_tree, (void *)unit->pos, unit, NULL);
            }
            LeaveCriticalSection(&blk->idletreeCS);
            return 2;
        }

        LeaveCriticalSection(&blk->idletreeCS);
        return 1;

    } else {
        LeaveCriticalSection(&blk->idletreeCS);

        /* found that the memory unit to be freed already exists in the idle tree. */
        next = RBTObj(rbtn);

        tolog(1, "Panic: reFreeUnit %ld:%d/%d to KemBlk %p pbgn=%p size=%ld actsize=%ld "
                 "allocsize=%ld restsize=%ld allocnum=%d! next=%p unit=%p %s:%d\n",
              unit->pos, unit->len, unit->size, blk, blk->pbgn, blk->size, blk->actsize,
              blk->allocsize, blk->restsize, blk->allocnum, next, unit, file, line);

        if (next != unit) {
            kemunit_free(blk, unit);
            unit = NULL;
        }
    }

    return 0;
}

void * kemblk_realloc_unit_dbg (void * vblk, void * p, int resize, char * file, int line)
{
    KemBlk  * blk = (KemBlk *)vblk;
    KemUnit * unit = NULL;
    KemUnit * next = NULL;
    void    * pnew = NULL;
    long      pos = 0;
    int       len;
    int       relen = 0;
    int       addedlen = 0;

    if (!blk || rbtree_num(blk->pos_idle_tree) <= 0) return NULL;

    if (!p) return kemblk_alloc_unit_dbg(blk, resize, file, line);

    pos = (uint8 *)p - blk->pbgn;
    if (pos < 0 || pos > blk->actsize) return NULL;

    relen = align_size(resize, sizeof(void *));

    EnterCriticalSection(&blk->alloctreeCS);
    unit = rbtree_get(blk->alloc_tree, (void *)pos);
    LeaveCriticalSection(&blk->alloctreeCS);
    if (!unit) {
        return kemblk_alloc_unit_dbg(blk, resize, file, line);
    }

    EnterCriticalSection(&blk->memCS);
    blk->stamp = time(0);
    LeaveCriticalSection(&blk->memCS);

    /* if reallocated memory size is smaller than the original memory length,
       just return without processing */
    if (resize <= unit->len) {
#ifdef _KEMDBG
        unit->line = line;
        str_ncpy(unit->file, file, sizeof(unit->file)-1);
#endif
        unit->size = resize;
        return p;
    }

    pos = unit->pos;
    len = unit->len;
    addedlen = relen - len;

    /* find the next idle memory unit adjacent to the current unit in the pos_idle_tree.
       check if it has enough space */

    EnterCriticalSection(&blk->idletreeCS);

    next = rbtree_get_gemin(blk->pos_idle_tree, (void *)pos);
    if (next && next->len >= addedlen && pos + len == next->pos) {
        /* If the next idle unit exist and is adjacent to the current unit, and the size
           of the next idle unit is big enough that meet the reallocation requirements */

#ifdef _KEMDBG
        unit->line = line;
        str_ncpy(unit->file, file, sizeof(unit->file)-1);
#endif  

        if (next->len <= addedlen + 8) {
            /* if the size of the next idle unit is slightly greater than the newly
               added size, merge this idle unit into the current unit to expand the
               space of the current unit */
            unit->len += next->len;
            unit->size = resize;

            /* delete the next unit from size_idle_tree and pos_idle_tree */
            rbtree_delete(blk->size_idle_tree, next);
            rbtree_delete(blk->pos_idle_tree, (void *)next->pos);

            LeaveCriticalSection(&blk->idletreeCS);

            EnterCriticalSection(&blk->memCS);
            blk->allocsize += next->len;
            blk->restsize -= next->len;
            LeaveCriticalSection(&blk->memCS);

            /* free the next unit */
            kemunit_free(blk, next);
            next = NULL;

            return p;

        } else {
            /* extend the length of the current unit to the idle memory unit, and adjust
               the starting position and length of the idle memory unit. */
            unit->len += addedlen;
            unit->size = resize;

            /* remove the 'next' unit from size_idle_tree due to the change of pos and len */
            rbtree_delete(blk->size_idle_tree, next);

            next->pos += addedlen;
            next->len -= addedlen;

            /* reinsert the 'next' unit into the size_idle_tree */
            rbtree_insert(blk->size_idle_tree, next, &next->rbnode[0], NULL);

            LeaveCriticalSection(&blk->idletreeCS);

            EnterCriticalSection(&blk->memCS);
            blk->allocsize += addedlen;
            blk->restsize -= addedlen;
            LeaveCriticalSection(&blk->memCS);

            return p;
        }
    }

    LeaveCriticalSection(&blk->idletreeCS);

    pnew = kemblk_alloc_unit_dbg(blk, resize, file, line);
    if (!pnew) {
        return NULL;
    }

    /* copy the data from the original memory unit to new-allocated memory unit */
    memcpy(pnew, p, unit->size);

    /* free the original memory unit */
    kemblk_free_unit_dbg (blk, p, file, line);

    return pnew;
}


KemPool * kempool_alloc (long size, int unitnum)
{
    KemPool * mp = NULL;

    mp = koszmalloc(sizeof(*mp));
    if (!mp) return NULL;

    if (unitnum <= 0) unitnum = size/512;
    if (unitnum < 1640) unitnum = 1640;

    mp->alloctype = 1; //osalloc

    InitializeCriticalSection(&mp->mpCS);

    mp->blk_list = arr_osalloc(32);
    mp->sort_blk_list = arr_osalloc(32);

    mp->blksize = align_size(size, sizeof(void *));
    mp->allocnum = 0;
    mp->checktime = 0;

    mp->kemunit_pool = mpool_osalloc();
    mpool_set_allocnum(mp->kemunit_pool, unitnum); //should ensure size of MemCache greater than 128K
    mpool_set_unitsize(mp->kemunit_pool, sizeof(KemUnit));
    mpool_set_initfunc(mp->kemunit_pool, kemunit_init);

    return mp;
}

int kempool_free (KemPool * mp)
{
    void     * blk = NULL;

    if (!mp) return -1;

    EnterCriticalSection(&mp->mpCS);

    while (arr_num(mp->blk_list) > 0) {
        blk = arr_pop(mp->blk_list);
        if (!blk) continue;

        kemblk_free(blk);
    }
    arr_free(mp->blk_list);
    arr_free(mp->sort_blk_list);

    LeaveCriticalSection(&mp->mpCS);

    DeleteCriticalSection(&mp->mpCS);

    mpool_free(mp->kemunit_pool);

    kosfree(mp);

    return 0;
}

long kempool_size (KemPool * mp)
{
    long size = 0;
    int  num = 0;

    if (!mp) return 0;

    size += mpool_size(mp->kemunit_pool);

    num = arr_num(mp->blk_list);
    size += num * (mp->blksize + sizeof(KemBlk));

    return size;
}

void kempool_print (KemPool * mp, frame_t * frm, FILE * fp, int alloclist, int idlelist,
                    int showsize, char * title, int margin)
{
    KemBlk    * blk = NULL;
    char        mar[32] = {0};
    int         i, num;
    KemUnit   * unit = NULL;
    int         j, total;
    time_t      curt = 0;
    rbtnode_t * node = NULL;

    if (!mp) return;

    num = sizeof(mar)/sizeof(char);

    if (margin < 0) margin = 0;
    if (margin >= num) margin = num - 1;

    for (i = 0; i < margin; i++) mar[i] = ' ';
    mar[margin] = '\0';

    num = arr_num(mp->blk_list);
    curt = time(NULL);

    if (frm)
        frame_appendf(frm, "%s%s: BlkNum=%d BlkSize=%ld AllocNum=%d CurT-ChkTime=%lu UnitP[%d/%d/%d]\n",
                      mar, title, num, mp->blksize, mp->allocnum, curt-mp->checktime,
                      mpool_allocated(mp->kemunit_pool), mpool_consumed(mp->kemunit_pool),
                      mpool_remaining(mp->kemunit_pool));

    if (fp)
        fprintf(fp, "%s%s: BlkNum=%d BlkSize=%ld AllocNum=%d CurT-ChkTime=%lu UnitP[%d/%d/%d]\n",
                mar, title, num, mp->blksize, mp->allocnum, curt-mp->checktime,
                mpool_allocated(mp->kemunit_pool), mpool_consumed(mp->kemunit_pool),
                mpool_remaining(mp->kemunit_pool));

    for (i = 0; i < num; i++) {
        blk = arr_value(mp->blk_list, i);
        if (!blk) continue;

        if (frm) {
            frame_appendf(frm, "%s    Blk %d/%d: alloc=%d idle=%d/%d curt-stamp=%lu size=%ld/%ld pbgn=%p"
                               " allocnum=%d allocsize=%ld restsize=%ld\n",
                          mar, i, num, rbtree_num(blk->alloc_tree), rbtree_num(blk->pos_idle_tree),
                          rbtree_num(blk->size_idle_tree), curt-blk->stamp, blk->size, blk->actsize,
                          blk->pbgn, blk->allocnum, blk->allocsize, blk->restsize);
        }

        if (fp) {
            fprintf(fp, "%s    Blk %d/%d: alloc=%d idle=%d/%d curt-stamp=%lu size=%ld/%ld pbgn=%p"
                        " allocnum=%d allocsize=%ld restsize=%ld\n",
                    mar, i, num, rbtree_num(blk->alloc_tree), rbtree_num(blk->pos_idle_tree),
                    rbtree_num(blk->size_idle_tree), curt-blk->stamp, blk->size, blk->actsize,
                    blk->pbgn, blk->allocnum, blk->allocsize, blk->restsize);
        }

        if (alloclist) {
            total = rbtree_num(blk->alloc_tree);
            if (frm) frame_appendf(frm, "    AllocatedUnit: %d\n", total);
            if (fp) fprintf(fp, "    AllocatedUnit: %d\n", total);

            if (total > 0 && frm) frame_appendf(frm, "    ");
            if (total > 0 && fp) fprintf(fp, "    ");

            node = rbtree_min_node(blk->alloc_tree);
            for (j = 0; j < total && node; j++) {
                unit = RBTObj(node);
                if (!unit) continue;
    
                if (frm) {
                    if (j > 0 && j % 4 == 0) frame_appendf(frm, "\n    ");
                    frame_appendf(frm, "  %3d:[%-6ld - %5d/%-5d]", j, unit->pos, unit->len, unit->size);
                }
                if (fp) {
                    if (j > 0 && j % 4 == 0) fprintf(fp, "\n    ");
                    fprintf(fp, "  %3d:[%-6ld - %5d/%-5d]", j, unit->pos, unit->len, unit->size);
                }
    
                node = rbtnode_next(node);
            }
            if (j > 0 && frm) frame_appendf(frm, "\n");
            if (j > 0 && fp) fprintf(fp, "\n");
        }

        if (idlelist) {
            if (showsize)
                total = rbtree_num(blk->size_idle_tree);
            else
                total = rbtree_num(blk->pos_idle_tree);
            if (frm) frame_appendf(frm, "    IdleUnit: %d\n", total);
            if (fp) fprintf(fp, "    IdleUnit: %d\n", total);

            if (total > 0 && frm) frame_appendf(frm, "    ");
            if (total > 0 && fp) fprintf(fp, "    ");

            if (showsize) {
                node = rbtree_min_node(blk->size_idle_tree);
            } else {
                node = rbtree_min_node(blk->pos_idle_tree);
            }
            for (j = 0; j < total && node; j++) {
                if (showsize) unit = structfrom(node, KemUnit, rbnode);
                else unit = RBTObj(node);
                if (!unit) continue;
    
                if (frm) {
                    if (j > 0 && j % 4 == 0) frame_appendf(frm, "\n    ");
                    frame_appendf(frm, "  %3d:[%-6ld - %5d]", j, unit->pos, unit->len);
                }
                if (fp) {
                    if (j > 0 && j % 4 == 0) fprintf(fp, "\n    ");
                    fprintf(fp, " %3d:[%-6ld - %5d]", j, unit->pos, unit->len);
                }

                node = rbtnode_next(node);
            }
            if (j > 0 && frm) frame_appendf(frm, "\n");
            if (j > 0 && fp) fprintf(fp, "\n");
        }
    }
}

int kempool_check (KemPool * mp)
{
    KemBlk   * blk = NULL;
    int        i = 0, num;
    time_t     curt = 0;

    if (!mp) return -1;

    curt = time(0);
    if (curt - mp->checktime < 15) {
        return 0;
    }

    EnterCriticalSection(&mp->mpCS);

    num = arr_num(mp->blk_list);
    for (i = num - 1; i >= 0; i--) {
        blk = arr_value(mp->blk_list, i);
        if (!blk) { arr_delete(mp->blk_list, i); i--; num--; continue; }

        if (curt - blk->stamp < 300) continue;

        if (rbtree_num(blk->alloc_tree) == 0 && rbtree_num(blk->pos_idle_tree) == 1) {
            arr_delete(mp->blk_list, i);
            i--; num--;

            arr_delete_ptr(mp->sort_blk_list, blk);

            kemblk_free(blk);
        }
    }

    mp->checktime = curt;

    LeaveCriticalSection(&mp->mpCS);

    return 0;
}


static void * kem_alloc_one_dbg (void * vmp, int size, void * exclblk, char * file, int line)
{
    KemPool * mp = (KemPool *)vmp;
    KemBlk  * blk = NULL;
    void    * pmem = NULL;
    int       i, num;

    if (!mp) return NULL;

    EnterCriticalSection(&mp->mpCS);

    num = arr_num(mp->blk_list);
    for (i = 0; i < num; i++) {
        blk = arr_value(mp->blk_list, i);
        if (!blk) continue;

        if (exclblk && blk == (KemBlk *)exclblk) continue;

     /* Every time the memory is allocated, multiple memory blocks under KemPool are traversed
      * from the beginning, and protected by a global lock, so the parallel efficiency is low.
      * One of the improved methods is to quickly find a memory unit meeting the requirement from
      * the memory block, delete it, leave the global lock, and complete the memory allocation
      * using this memory unit. But this method has many shortcomings: if the memory unit is the
      * only remaining one in this memory block, which happens to be taken away by other threads,
      * it will cause the system to allocate a new memory block. If similar situations occur
      * frequently, a large number of memory blocks will be allocated and left idle. These wasted
      * memory spaces lead to the reduction of resource utilization efficiency. */ 
        pmem = kemblk_alloc_unit_dbg(blk, size, file, line);
        if (pmem) {
            mp->allocnum++;
            LeaveCriticalSection(&mp->mpCS);
            return pmem;
        }
    }

    if (i >= num) {
        blk = kemblk_alloc(mp, mp->blksize);
        if (!blk) {
            LeaveCriticalSection(&mp->mpCS);

            tolog(1, "Panic: Failed to create a new KemBlk of %d, blksize=%ld\n", num, mp->blksize);
            return NULL;
        }

        arr_push(mp->blk_list, blk);
        arr_insert_by(mp->sort_blk_list, blk, kemblk_cmp_kemblk);
    }

    pmem = kemblk_alloc_unit_dbg(blk, size, file, line);
    if (pmem) {
        mp->allocnum++;
    } else {
        tolog(1, "Panic: failed to alloc %d from KemBlk %p pbgn=%p size=%ld allocsize=%ld "
                 "restsize=%ld allocnum=%d blknum=%d!\n",
              size, blk, blk->pbgn, blk->size, blk->allocsize,
              blk->restsize, blk->allocnum, arr_num(mp->blk_list));
    }

    LeaveCriticalSection(&mp->mpCS);

    return pmem;
}

void * kem_alloc_dbg (void * vmp, int size, char * file, int line)
{
    return kem_alloc_one_dbg(vmp, size, NULL, file, line);
}

void * kem_zalloc_dbg (void * vmp, int size, char * file, int line)
{
    void * p = kem_alloc_one_dbg(vmp, size, NULL, file, line);

    if (p) memset(p, 0, size);

    return p;
}

int kem_free_dbg (void * vmp, void * p, char * file, int line)
{
    KemPool * mp = (KemPool *)vmp;
    KemBlk  * blk = NULL;
    long      pos = 0;

    if (!mp) return -1;
    if (!p) return -2;

    EnterCriticalSection(&mp->mpCS);

    blk = arr_find_by(mp->sort_blk_list, p, kemblk_cmp_p);
    if (!blk || (pos = (uint8 *)p - blk->pbgn) < 0) {
        LeaveCriticalSection(&mp->mpCS);
        tolog(1, "Panic: kem_free failed, p=%p not found KemBlk, %s:%d\n", p, file, line);
        return -100;
    }

    mp->allocnum--;

    LeaveCriticalSection(&mp->mpCS);
    
    kemblk_free_unit_dbg(blk, p, file, line);

    kempool_check(mp);

    return 0;
}

void * kem_realloc_dbg (void * vmp, void * p, int resize, char * file, int line)
{
    KemPool * mp = (KemPool *)vmp;
    KemBlk  * blk = NULL;
    KemUnit * unit = NULL;
    void    * pnew = NULL;
    long      pos = 0;

    if (!mp) return NULL;
    if (!p) return kem_alloc(mp, resize);

    /* find the memory block where the current memory pointer is located. */
    EnterCriticalSection(&mp->mpCS);
    blk = arr_find_by(mp->sort_blk_list, p, kemblk_cmp_p);
    LeaveCriticalSection(&mp->mpCS);
    if (!blk || (pos = (uint8 *)p - blk->pbgn) < 0) {
        tolog(1, "Panic: kem_realloc failed: p=%p realloc-size:%d not found KemBlk\n", p, resize);
        return NULL;
    }

    /* reallocate memory in the current memory block. If the reallocation is
       successful, a new memory pointer is returned. */
    pnew = kemblk_realloc_unit_dbg(blk, p, resize, file, line);
    if (pnew) return pnew;

    /* If memory reallocation in the current memory block fails, get the
       memory unit based on the memory pointer. Then the size of memory pointer will be known */
    EnterCriticalSection(&blk->alloctreeCS);
    unit = rbtree_get(blk->alloc_tree, (void *)pos);
    LeaveCriticalSection(&blk->alloctreeCS);
    if (!unit) {
        tolog(1, "Panic: kem_realloc failed, p=%p realloc-size:%d, pos=%ld "
                 "not found KemUnit\n", p, resize, pos);
        return NULL;
    }

    /* Allocate a new memory unit based on new size in other memory blocks exclusive 
       of the current memory block 'blk' */
    pnew = kem_alloc_one_dbg(mp, resize, blk, file, line);
    if (!pnew) {
        tolog(1, "Panic: kem_realloc %ld:%d/%d to %ld failed, %p\n",
              unit->pos, unit->len, unit->size, resize, p);
        return NULL;
    }

    /* copy the data from the original memory unit to new-allocated memory unit */
    memcpy(pnew, p, unit->size);

    /* free the original memory unit */
    kemblk_free_unit_dbg (blk, p, file, line);

    EnterCriticalSection(&mp->mpCS);
    mp->allocnum--;
    LeaveCriticalSection(&mp->mpCS);

    return pnew;
}

int kem_size (void * vmp, void * p)
{
    KemPool * mp = (KemPool *)vmp;
    KemBlk  * blk = NULL;
    KemUnit * unit = NULL;
    long      pos = 0;

    if (!mp || !p) return -1;

    /* find the memory block where the current memory pointer is located. */
    EnterCriticalSection(&mp->mpCS);
    blk = arr_find_by(mp->sort_blk_list, p, kemblk_cmp_p);
    LeaveCriticalSection(&mp->mpCS);
    if (!blk || (pos = (uint8 *)p - blk->pbgn) < 0) {
        return -2;
    }

    /* obtain memory unit corresponding to the memory pointer */
    EnterCriticalSection(&blk->alloctreeCS);
    unit = rbtree_get(blk->alloc_tree, (void *)pos);
    LeaveCriticalSection(&blk->alloctreeCS);
    if (!unit) return -3;

    return unit->len;
}

void * kem_by_index (void * vmp, int index)
{
    KemPool * mp = (KemPool *)vmp;
    KemBlk  * blk = NULL;
    KemUnit * unit = NULL;
    int       i, num;
    int       iter = 0;
    int       unitnum = 0;
    rbtnode_t * node = NULL;

    if (!mp) return NULL;

    EnterCriticalSection(&mp->mpCS);

    num = arr_num(mp->blk_list);
    for (i = 0; i < num; i++) {
        blk = arr_value(mp->blk_list, i);
        if (!blk) continue;

        unitnum = rbtree_num(blk->alloc_tree);
        if (unitnum <= 0) continue;

        if (index >= iter && index < iter + unitnum)
            break;

        iter += unitnum;
    }

    if (i >= num) {
        LeaveCriticalSection(&mp->mpCS);
        return NULL;
    }

    EnterCriticalSection(&blk->alloctreeCS);
    node = rbtree_min_node(blk->alloc_tree);
    for ( ; iter < index; iter++) {
        node = rbtnode_next(node);
    }
    LeaveCriticalSection(&blk->alloctreeCS);

    if (node && (unit = RBTObj(node))) {
        LeaveCriticalSection(&mp->mpCS);
        return blk->pbgn + unit->pos;
    }

    LeaveCriticalSection(&mp->mpCS);

    return NULL;
}


void * k_mem_alloc_dbg (int size, int alloctype, void * mpool, char * file, int line)
{
    void * pm = NULL;

    if (alloctype == 1) {
        pm = kosmalloc_dbg(size, file, line);
    } else if (alloctype == 2 && mpool) {
        pm = kem_alloc_dbg(mpool, size, file, line);
    } else if (alloctype == 3 && mpool) {
        pm = kemblk_alloc_unit_dbg(mpool, size, file, line);
	if (!pm) {
            pm = kem_alloc_dbg(((KemBlk*)mpool)->kmpool, size, file, line);
	}
    } else {
        pm = kalloc_dbg(size, file, line);
    }
    return pm;
}

void * k_mem_zalloc_dbg (int size, int alloctype, void * mpool, char * file, int line)
{
    void * pm = NULL;

    if (alloctype == 1) {
        pm = koszmalloc_dbg(size, file, line);
    } else if (alloctype == 2 && mpool) {
        pm = kem_zalloc_dbg(mpool, size, file, line);
    } else if (alloctype == 3 && mpool) {
        pm = kemblk_alloc_unit_dbg(mpool, size, file, line);
	if (!pm) {
            pm = kem_alloc_dbg(((KemBlk*)mpool)->kmpool, size, file, line);
	}
	if (pm) memset(pm, 0, size);
    } else {
        pm = kzalloc_dbg(size, file, line);
    }
    return pm;
}

void * k_mem_realloc_dbg (void * opm, int size, int alloctype, void * mpool, char * file, int line)
{
    void    * pm = NULL;
    KemBlk  * blk = NULL;
    KemBlk  * nblk = NULL;
    KemUnit * unit = NULL;
    KemPool * mp = NULL;
    long      pos = 0;

    if (alloctype == 1) {
        pm = kosrealloc_dbg(opm, size, file, line);
    } else if (alloctype == 2 && mpool) {
        pm = kem_realloc_dbg(mpool, opm, size, file, line);
    } else if (alloctype == 3 && mpool) {
        blk = nblk = (KemBlk *)mpool;

        /* obtain memory unit corresponding to the memory pointer */
        pos = (uint8 *)opm - blk->pbgn;
        EnterCriticalSection(&blk->alloctreeCS);
        unit = rbtree_get(blk->alloc_tree, (void *)pos);
        LeaveCriticalSection(&blk->alloctreeCS);
        if (!unit && blk->kmpool) {
            mp = (KemPool *)blk->kmpool;
            EnterCriticalSection(&mp->mpCS);
            nblk = arr_find_by(mp->sort_blk_list, opm, kemblk_cmp_p);
            LeaveCriticalSection(&mp->mpCS);
            if (nblk) {
                pos = (uint8 *)opm - nblk->pbgn;
                EnterCriticalSection(&nblk->alloctreeCS);
                unit = rbtree_get(nblk->alloc_tree, (void *)pos);
                LeaveCriticalSection(&nblk->alloctreeCS);
            }
        }

        pm = kemblk_alloc_unit_dbg(blk, size, file, line);
	if (!pm && blk->kmpool) {
            pm = kem_alloc_dbg(blk->kmpool, size, file, line);
	}
        if (!pm) return NULL;

        if (unit) {
            /* copy the data from the original memory unit to new-allocated memory unit */
            memcpy(pm, opm, unit->size);

            /* free the original memory unit */
            kemblk_free_unit_dbg (nblk, opm, file, line);
        }

    } else {
        pm = krealloc_dbg(opm, size, file, line);
    }
    return pm;

}

void k_mem_free_dbg (void * pm, int alloctype, void * mpool, char * file, int line)
{
    if (alloctype == 1) {
        kosfree_dbg(pm, file, line);
    } else if (alloctype == 2 && mpool) {
        kem_free_dbg(mpool, pm, file, line);
    } else if (alloctype == 3 && mpool) {
        if (kemblk_free_unit_dbg(mpool, pm, file, line) < 0) {
            if (kem_free_dbg(((KemBlk*)mpool)->kmpool, pm, file, line) < 0)
                tolog(1, "Panic: k_mem_free failed, p=%p, %s:%d\n", pm, file, line);
	}
    } else {
        kfree_dbg(pm, file, line);
    }
}

void * k_mem_str_dup_dbg (void * p, int bytelen, int alloctype, void * mpool, char * file, int line)
{
    uint8 * pbyte = (uint8 *)p;
    uint8 * pdup = NULL;

    if (!pbyte) return NULL;

    if (bytelen < 0)
        bytelen = str_len(pbyte);
    if (bytelen <= 0)
        return NULL;

    pdup = k_mem_alloc_dbg(bytelen + 1, alloctype, mpool, file, line);
    if (!pdup)
        return NULL;

    memcpy(pdup, pbyte, bytelen);
    pdup[bytelen] = '\0';

    return pdup;
}

