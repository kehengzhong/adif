/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include "btype.h"
#include "memory.h"
#include "vstar.h"
#include "nativefile.h"
#include "mthread.h"
#include "fragpack.h"

int fragitem_cmp_offset (void * a, void * b)
{
    FragItem * pl = (FragItem *)a;
    int64      pos = *(int64 *)b;
 
    if (pl->offset > pos) return 1;

    if (pl->offset < pos) {
        if (pl->offset + pl->length > pos) return 0;
        else return -1;
    }

    return 0;
}

int fragitem_cmp_by_offset (void * a, void * b)
{
    FragItem * pla = (FragItem *)a;
    FragItem * plb = (FragItem *)b;
 
    if (pla->offset < plb->offset) return -1;
    if (pla->offset > plb->offset) return 1;
    return 0;
}

void * frag_pack_alloc ()
{
    FragPack * frag = NULL;

    frag = kalloc(sizeof(*frag));
    if (!frag) return NULL;

    frag->complete = 0;
    frag->length = 0;
    frag->rcvlen = 0;

    InitializeCriticalSection(&frag->packCS);

    frag->pack_var = vstar_new(sizeof(FragItem), 4, NULL);

    return frag;
}

void frag_pack_free (void * vfrag)
{
    FragPack * frag = (FragPack *)vfrag;

    if (!frag) return;

    DeleteCriticalSection(&frag->packCS);

    vstar_free(frag->pack_var);

    kfree(frag);
}
 
int frag_pack_zero (void * vfrag)
{
    FragPack * frag = (FragPack *)vfrag;

    if (!frag) return -1;

    EnterCriticalSection(&frag->packCS);

    frag->complete = 0;
    frag->length = 0;
    frag->rcvlen = 0;

    vstar_zero(frag->pack_var);

    LeaveCriticalSection(&frag->packCS);

    return 0;
}

void frag_pack_set_length (void * vfrag, int64 length)
{
    FragPack * frag = (FragPack *)vfrag;
    FragItem * item = NULL;

    if (!frag) return;

    if (frag->length == length) return;

    EnterCriticalSection(&frag->packCS);

    frag->length = length;

    frag->complete = 0;
    if (length > 0 && vstar_num(frag->pack_var) == 1) {
        item = vstar_get(frag->pack_var, 0);
        if (item && item->offset == 0) {
            if (frag->length > 0 && item->length == frag->length)
                frag->complete = 1;
        }
    }

    LeaveCriticalSection(&frag->packCS);
}

int frag_pack_complete (void * vfrag)
{
    FragPack * frag = (FragPack *)vfrag;
    FragItem * item = NULL;

    if (!frag) return 0;

    EnterCriticalSection(&frag->packCS);

    frag->complete = 0;
    if (vstar_num(frag->pack_var) == 1) {
        item = vstar_get(frag->pack_var, 0);
        if (item && item->offset == 0) {
            if (frag->length > 0 && item->length == frag->length)
                frag->complete = 1;
        }
    }

    LeaveCriticalSection(&frag->packCS);

    return frag->complete;
}
 
int64 frag_pack_length (void * vfrag)
{
    FragPack * frag = (FragPack *)vfrag;

    if (!frag) return 0;

    return frag->length;
}

int64 frag_pack_rcvlen (void * vfrag, int * fragnum)
{
    FragPack * frag = (FragPack *)vfrag;
 
    if (!frag) return 0;
 
    if (fragnum)
        *fragnum = vstar_num(frag->pack_var);

    return frag->rcvlen;
}

int64 frag_pack_curlen (void * vfrag)
{
    FragPack * frag = (FragPack *)vfrag;
    FragItem * item = NULL;

    if (!frag) return 0;

    EnterCriticalSection(&frag->packCS);
    item = vstar_last(frag->pack_var);
    LeaveCriticalSection(&frag->packCS);

    if (item) {
        return item->offset + item->length;
    }

    return 0;
}

int frag_pack_read (void * vfrag, void * hfile, int64 pos)
{
    FragPack * frag = (FragPack *)vfrag;
    FragItem * item = NULL;
    int        i, num = 0;
    int        len = 0;
    void     * p = NULL;

    if (!frag) return -1;
    if (!hfile) return -2;

    EnterCriticalSection(&frag->packCS);

    native_file_seek(hfile, pos);

    native_file_read(hfile, &frag->length, 8);

    native_file_read(hfile, &len, 4);
    if ((len % sizeof(FragItem)) != 0) {
        LeaveCriticalSection(&frag->packCS);
        return -100;
    }

    if (len <= 0 || len >= 16*1024*1024) {
        LeaveCriticalSection(&frag->packCS);
        return -101;
    }

    p = kalloc(len);
    native_file_read(hfile, p, len);

    vstar_zero(frag->pack_var);
    frag->rcvlen = 0;

    num = len/sizeof(FragItem);
    for (i = 0; i < num; i++) {
        item = (FragItem *)((uint8 *)p + i * sizeof(FragItem));
        frag->rcvlen += item->length;

        vstar_push(frag->pack_var, item);
    }
    kfree(p);

    LeaveCriticalSection(&frag->packCS);

    frag_pack_complete(frag);

    return 0;
}

int frag_pack_write (void * vfrag, void * hfile, int64 pos)
{
    FragPack * frag = (FragPack *)vfrag;
    int        len = 0;
 
    if (!frag) return -1;
    if (!hfile) return -2;
 
    EnterCriticalSection(&frag->packCS);

    len = vstar_len(frag->pack_var);

    native_file_seek(hfile, pos);
    native_file_write(hfile, &frag->length, 8);
    native_file_write(hfile, &len, 4);
    native_file_write(hfile, vstar_ptr(frag->pack_var), len);

    LeaveCriticalSection(&frag->packCS);

    return len + 12;
}
 
int frag_pack_add (void * vfrag, int64 pos, int64 len)
{
    FragPack * frag = (FragPack *)vfrag;
    FragItem   sitem = {0};
    FragItem * item = NULL;
    FragItem * next = NULL;
    int        found = 0;
    int        loc = 0;
    int64      rcvlen = 0;

    if (!frag) return -1;

    if (frag->complete) return 0;

    if (frag->length > 0) {
        if (pos > frag->length) pos = frag->length;
        if (pos + len > frag->length)
            len = frag->length - pos;
    }

    sitem.offset = pos;
    sitem.length = len;

    EnterCriticalSection(&frag->packCS);

    loc = vstar_findloc_by(frag->pack_var, &pos, fragitem_cmp_offset, &found);
    if (!found) {
        item = NULL;
        if (loc > 0) 
            item = vstar_get(frag->pack_var, loc-1);
        if (item && item->offset + item->length == pos) {
            sitem.offset = item->offset;
            sitem.length = item->length;

            item->length = pos + len - item->offset;
            goto loopcheck;
        }

        next = vstar_get(frag->pack_var, loc);
        if (!next || pos + len < next->offset) {
            /* -...  !     !      |----------| ... 
                    pos pos+len  next     length    */
            vstar_insert(frag->pack_var, &sitem, loc);

            rcvlen = len;
            frag->rcvlen += rcvlen;

            goto end_add;
        }

        if (pos + len <= next->offset + next->length) {
            /* -...  !    |-------!------| ...
                    pos  next  pos+len  length    */
            rcvlen = next->offset - pos;
            frag->rcvlen += rcvlen;

            next->length += next->offset - pos;
            next->offset = pos;

            goto end_add;
        } 

        /* -...  !    |-------|    |--------!-------| ...
                pos  next       next+1   pos+len    */
        rcvlen = next->offset - pos;
        frag->rcvlen += rcvlen;

        sitem.offset = next->offset;
        sitem.length = next->length;

        item = next;
        item->offset = pos;
        item->length = len;
        loc++;

    } else { //found one item
        /* -... |-----!----------!-----|        |-----------| ... 
               item  pos                       next     */
        item = vstar_get(frag->pack_var, loc);
        if (pos + len <= item->offset + item->length) {
            LeaveCriticalSection(&frag->packCS);
            return 0;
        }

        /* -... |-----!------|        |--------!-----| ... 
               item  pos             next   pos+len         */
        sitem.offset = item->offset;
        sitem.length = item->length;

        item->length = pos + len - item->offset;
        loc++;
    }

loopcheck:
    while (loc <= vstar_num(frag->pack_var)) {
        next = vstar_get(frag->pack_var, loc);
        if (next && item->offset + item->length >= next->offset) {
            /* -... |-----!----|     |----!--------| ... 
                   item  pos       next pos+len    */
            rcvlen = next->offset - (sitem.offset + sitem.length);
            frag->rcvlen += rcvlen;

            if (item->offset + item->length <= next->offset + next->length) {
                item->length = next->offset - item->offset + next->length;
                vstar_delete(frag->pack_var, loc);
                break;
 
            } else {
                /* -... |-----!----|     |--------|    |-------!---|  ... 
                       item  pos       next           next  pos+len    */
                sitem.offset = next->offset;
                sitem.length = next->length;

                vstar_delete(frag->pack_var, loc);
            }
 
        } else {
            rcvlen = (item->offset + item->length) - (sitem.offset + sitem.length);
            frag->rcvlen += rcvlen;
            break;
        }
    }

end_add:
    if (vstar_num(frag->pack_var) == 1) {
        item = vstar_get(frag->pack_var, 0);
        if (item && item->offset == 0) {
            if (frag->length > 0 && item->length == frag->length)
                frag->complete = 1;
        }
    }

    LeaveCriticalSection(&frag->packCS);
    return 1;
}

int frag_pack_del (void * vfrag, int64 pos, int64 len)
{
    FragPack * frag = (FragPack *)vfrag;
    FragItem   sitem = {0};
    FragItem * item = NULL;
    int        found = 0;
    int        loc = 0;
 
    if (!frag) return -1;
 
    if (frag->length > 0) {
        if (pos > frag->length) pos = frag->length;
        if (pos + len > frag->length)
            len = frag->length - pos;
    }

    EnterCriticalSection(&frag->packCS);
 
    loc = vstar_findloc_by(frag->pack_var, &pos, fragitem_cmp_offset, &found);
    if (found) {
        item = vstar_get(frag->pack_var, loc);
        if (item) {
            if (pos == item->offset) {
                if (pos + len >= item->offset + item->length) {
                    /*     pos          pos+len                                 
                       -... |----------|  !    |--------|    |-----------|  ... 
                           item               next           next             */
                    frag->rcvlen -= item->length;
                    vstar_delete(frag->pack_var, loc);
                } else {
                    /*     pos   pos+len                                 
                       -... |------!---|       |--------|    |-----------|  ... 
                           item               next           next             */
                    item->length -= len;
                    item->offset = pos + len;

                    frag->rcvlen -= len;
                    LeaveCriticalSection(&frag->packCS);
                    return 1;
                }
            } else {
                if (pos + len >= item->offset + item->length) {
                    /*       pos         pos+len                                 
                       -... |--!--------|   !    |--------|    |-----------|  ... 
                           item                 next           next             */
                    frag->rcvlen -= item->length - (pos - item->offset);
                    item->length = pos - item->offset;
                    loc++;
                } else {
                    /*       pos   pos+len                                 
                       -... |--!-----!--|       |--------|    |-----------|  ... 
                           item                 next           next             */
                    frag->rcvlen -= len;

                    sitem.offset = pos + len;
                    sitem.length = (item->offset + item->length) - (pos + len);
                    vstar_insert(frag->pack_var, &sitem, loc + 1);

                    item->length = pos - item->offset;
                    LeaveCriticalSection(&frag->packCS);
                    return 1;
                }
            }
        }
    }

    while (loc <= vstar_num(frag->pack_var)) {
        item = vstar_get(frag->pack_var, loc);
        if (!item || pos + len <= item->offset) break;

        if (pos + len >= item->offset + item->length) {
            /*       pos                      pos+len                                 
              -... |--!--------|     |--------|  !   |-----------|  ... 
                                    item            next             */
            frag->rcvlen -= item->length;
            vstar_delete(frag->pack_var, loc);

        } else {
            /*       pos                pos+len                                 
              -... |--!--------|     |-----!---|      |-----------|  ... 
                                    item            next             */
            frag->rcvlen -= (pos + len) - item->offset;
            item->length = (item->offset + item->length) - (pos + len);
            item->offset = pos + len;
            break;
        }
    }

    LeaveCriticalSection(&frag->packCS);

    return 1;
}

int frag_pack_get (void * vfrag, int64 pos, int64 * actpos, int64 * actlen)
{
    FragPack * frag = (FragPack *)vfrag;
    FragItem * item = NULL;
    int        found = 0;
    int        loc = 0;
 
    if (actpos) *actpos = 0;
    if (actlen) *actlen = 0;

    if (!frag) return -1;
 
    EnterCriticalSection(&frag->packCS);
 
    loc = vstar_findloc_by(frag->pack_var, &pos, fragitem_cmp_offset, &found);
    if (!found) {
        item = vstar_get(frag->pack_var, loc);
        if (item) {
            if (actpos) *actpos = item->offset;
            if (actlen) *actlen = item->length;

            LeaveCriticalSection(&frag->packCS);
            return 0;
        }

        LeaveCriticalSection(&frag->packCS);
        return -100;
    }

    item = vstar_get(frag->pack_var, loc);
    if (item) {
        if (actpos) *actpos = item->offset;
        if (actlen) *actlen = item->length;

        LeaveCriticalSection(&frag->packCS);
        return 1;
    }

    LeaveCriticalSection(&frag->packCS);

    return 0;
}
 
int frag_pack_gap (void * vfrag, int64 pos, int64 * gappos, int64 * gaplen)
{
    FragPack * frag = (FragPack *)vfrag;
    FragItem * prev = NULL;
    FragItem * next = NULL;
    int64      offset = 0;
    int64      length = 0;
    int        found = 0;
    int        loc = 0;
 
    if (gappos) *gappos = 0;
    if (gaplen) *gaplen = 0;

    if (!frag) return -1;
 
    EnterCriticalSection(&frag->packCS);
 
    loc = vstar_findloc_by(frag->pack_var, &pos, fragitem_cmp_offset, &found);
    if (!found) {
        next = vstar_get(frag->pack_var, loc);
        if (loc > 0)
            prev = vstar_get(frag->pack_var, loc - 1);

    } else {
        prev = vstar_get(frag->pack_var, loc);
        next = vstar_get(frag->pack_var, loc + 1);
    }

    if (prev) {
        offset = prev->offset + prev->length;
    } else {
        offset = 0;
    }

    if (next) {
        length = next->offset - offset;
    } else {
        if (frag->length > 0)
            length = frag->length - offset;
        else
            length = -1;
    }
    
    LeaveCriticalSection(&frag->packCS);

    if (gappos) *gappos = offset;
    if (gaplen) *gaplen = length;

    if (length > 0) return 1;
    return 0;
}

/* check if the given range pos:length is contained.
   return value: 0 - completely not contained
                 1 - right-side partial contained,   !    |--!-------|
                 2 - left-side partial contained,    |------!---|    !
                 3 - completely contained
 */
int frag_pack_contain (void * vfrag, int64 pos, int64 length, int64 * datapos,
                       int64 * datalen, int64 * gappos, int64 * gaplen)
{
    FragPack * frag = (FragPack *)vfrag;
    FragItem * item = NULL;
    FragItem * next = NULL;
    int        found = 0;
    int        loc = 0;
 
    if (datapos) *datapos = pos;
    if (datalen) *datalen = 0;
    if (gappos) *gappos = pos;
    if (gaplen) *gaplen = -1;
 
    if (!frag) return -1;
 
    if (frag->complete) {
        if (datalen) *datalen = frag->length - pos;
        if (gappos) *gappos = frag->length;
        if (gaplen) *gaplen = 0;
        return 3;
    }

    if (pos >= 0) {
        if (length < 0 && frag->length > 0)  //[200, -1]
            length = frag->length - pos;
        else if (length < 0 && frag->length <= 0)  //[200, -1]
            length = 0x7FFFFFFFFFFFFFFF - pos;
    } else {
        if (length > 0 && frag->length > 0)  //[-1, 1000]
            pos = frag->length - length;
    }

    if (pos < 0 || length < 0) return -10;

    EnterCriticalSection(&frag->packCS);

    loc = vstar_findloc_by(frag->pack_var, &pos, fragitem_cmp_offset, &found);
    if (!found) {
        item = vstar_get(frag->pack_var, loc);
        if (item) {
            if (gaplen) *gaplen = item->offset - pos;
        } else {
            if (gaplen) *gaplen = frag->length - pos;
        }

        if (item && pos + length >= item->offset) {
            /* !    |--!-------| */
            if (datapos) *datapos = item->offset;
            if (datalen) *datalen = pos + length - item->offset;

            LeaveCriticalSection(&frag->packCS);
            return 1;  //right-side partial contained
        }

        LeaveCriticalSection(&frag->packCS);
        return 0;
 
    } else {
        item = vstar_get(frag->pack_var, loc);
        if (item) {
            if (gappos) *gappos = item->offset + item->length;

            next = vstar_get(frag->pack_var, loc+1);
            if (next) {
                if (gaplen) *gaplen = next->offset - item->offset - item->length;
            } else {
                if (gaplen) *gaplen = frag->length - item->offset - item->length;
            }

            if (pos + length > item->offset + item->length) {
                /* |------!---|    ! */
                if (datapos) *datapos = pos;
                if (datalen) *datalen = item->offset + item->length - pos;

                LeaveCriticalSection(&frag->packCS);
                return 2;  //left-side partial contained

            } else {
                if (datapos) *datapos = pos;
                if (datalen) *datalen = length;

                LeaveCriticalSection(&frag->packCS);
                return 3;  //completely contained
            }
        }
    }

    LeaveCriticalSection(&frag->packCS);

    return 0;
}

void frag_pack_print (void * vfrag, FILE * fp)
{
    FragPack * frag = (FragPack *)vfrag;
    FragItem * item = NULL;
    int        i, num;

    if (!frag) return;

    if (!fp) fp = stdout;

    num = vstar_num(frag->pack_var);

    fprintf(fp, "FragLength: %lld  RcvLen: %lld  Complete: %d  FragNum: %d\n",
            frag->length, frag->rcvlen, frag->complete, num);

    for (i = 1; i <= num; i++) {
        item = vstar_get(frag->pack_var, i-1);
        if (!item) continue;

        if (i % 6 == 0) fprintf(fp, "\n");

        fprintf(fp, "  [%-2d %lld-%lld]", i, item->offset, item->length);
    }
    fprintf(fp, "\n");
}

