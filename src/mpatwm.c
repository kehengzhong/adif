/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include "btype.h"
#include <math.h>
#include "memory.h"
#include "dynarr.h"
#include "filecache.h"

#include "mpatwm.h"


typedef struct prefix_item {
    uint32     hash;
    int        index;
    uint16     dualhash;
    uint8      singlehash;
    void     * next;
} PrefixItem;

typedef struct pattern_item {
    int          length;
    uint8        pattern[128];
    PMatchSucc * matchsucc;
    void       * para;
} PatternItem;


typedef struct alphabet_unit {
    uint8      letter;
    uint8      offset;
} AlphaUnit;


typedef struct wu_manber_body {
    int           B;
    int           tabsize;

    int           patslen;
    int           patnum;

    AlphaUnit     alphatab[256];
    int           alphasize;
    int           alphabits;

    arr_t        * patlist;

    uint8       * shifttab;
    void       ** prefixtab;

} WMBody;


int wm_hash_func (void * vbody, uint8 * p, int len)
{
    WMBody * body = (WMBody *)vbody;
    int      value = 0;

    while (len-- > 0) {
        value <<= body->alphabits;
        value += body->alphatab[(int)*p].offset;
        p++;
    }

    return value;
}


int wm_tab_clean (void * vbody)
{
    WMBody     * body = (WMBody *)vbody;
    PrefixItem * iternode = NULL, * next = NULL;
    int          i = 0;
    
    if (!body) return -1;
    
    if (body->shifttab) {
        kfree(body->shifttab);
        body->shifttab = NULL;
    }   

    if (body->prefixtab) {
        for (i=0; i<body->tabsize; i++) {
            if ((iternode = body->prefixtab[i]) != NULL) {
                while (iternode) {
                    next = iternode->next;
                    kfree(iternode);
                    iternode = next;
                }
            }
        }
        kfree(body->prefixtab);
        body->prefixtab = NULL;
    }

    return 0;
}


void * wm_init (int isascii, int ignorecase)
{
    WMBody  * body = NULL;
    int       i;

    body = kzalloc(sizeof(*body));
    if (!body) return NULL;

    body->B = 2;
    body->tabsize = 0;

    body->patslen = 0;
    body->patnum = 0;

    body->patlist = arr_new(16);

    body->alphasize = 0;
    body->alphatab[0].letter = 0;
    body->alphatab[0].offset = body->alphasize++;

    for (i = 1; i < 256; i++) {
        body->alphatab[i].letter = i;

        if (isascii) {
            if (i >= 32 && i < 127) {
                if (ignorecase && i>='a' && i<='z') {
                    body->alphatab[i].letter = body->alphatab[i-'a'+'A'].letter;
                    body->alphatab[i].offset = body->alphatab[i-'a'+'A'].offset;

                } else {
                    body->alphatab[i].offset = body->alphasize++;
                }

            } else
                body->alphatab[i].offset = 0;

        } else {
            if (ignorecase && i>='a' && i<='z') {
                body->alphatab[i].letter = body->alphatab[i-'a'+'A'].letter;
                body->alphatab[i].offset = body->alphatab[i-'a'+'A'].offset;

            } else {
                body->alphatab[i].offset = body->alphasize++;
            }
        }
    }

    if (body->alphasize > 128)     { body->alphasize = 256; body->alphabits = 8; }
    else if (body->alphasize > 64) { body->alphasize = 128; body->alphabits = 7; }
    else if (body->alphasize > 32) { body->alphasize = 64;  body->alphabits = 6; }
    else if (body->alphasize > 16) { body->alphasize = 32;  body->alphabits = 5; }
    else if (body->alphasize > 8)  { body->alphasize = 16;  body->alphabits = 4; }
    else                           { body->alphasize = 8;   body->alphabits = 3; }

    return body;
}

int wm_free (void * vbody)
{
    WMBody * body = (WMBody *)vbody;

    if (!body) return -1;

    wm_tab_clean(body);

    if (body->patlist) {
        while (arr_num(body->patlist)> 0) 
            kfree(arr_pop(body->patlist));
        arr_free(body->patlist);
        body->patlist = NULL;
    }

    kfree(body);
    return 0;
}

int wm_reset (void * vbody)
{
    WMBody * body = (WMBody *)vbody;
     
    if (!body) return -1;
     
    wm_tab_clean(body);
     
    if (body->patlist == NULL)
        body->patlist = arr_new(16);

    while (arr_num(body->patlist)> 0)
        kfree(arr_pop(body->patlist));

    arr_zero(body->patlist); 
 
    return 0;
}


int wm_pattern_add (void * vbody, void * vpat, int patlen, void * matchfunc, void * para)
{
    uint8       * pat = (uint8 *)vpat;
    WMBody      * body = (WMBody *)vbody;
    PatternItem * patitem = NULL;

    if (!body) return -1;

    if (!pat) return -2;
    if (patlen < 0) patlen = strlen((char *)pat);
    if (patlen <= 0) return -3;

    patitem = kzalloc(sizeof(*patitem));
    if (!patitem) return -100;

    if (patlen > sizeof(patitem->pattern))
        patlen = sizeof(patitem->pattern);

    patitem->length = patlen;
    memcpy(patitem->pattern, pat, patlen);
    patitem->matchsucc = matchfunc;
    patitem->para = para;

    arr_push(body->patlist, patitem);

    if (body->patslen == 0 || body->patslen > patlen)
        body->patslen = patlen;

    return 0;
}

int wm_adjust_shifttab (void * vbody, uint8 * suffb, int L)
{
    WMBody  * body = (WMBody *)vbody;
    int       prefhash = 0;
    int       suffhash = 0;
    int       hash = 0;
    int       max_prefix_hash = 0;
    int       i = 0;
    uint8     prefch[8];

    if (!body) return -1;
    if (!suffb || L < 1) return -2;

    suffhash = wm_hash_func(body, suffb, L);

    max_prefix_hash = pow(body->alphasize, body->B - L);

    for (i = 0; i < max_prefix_hash; i++) {
        if (body->B - L == 1) {
            prefch[0] = i;
            prefhash = wm_hash_func(body, prefch, 1);
            hash = (prefhash << (body->alphabits * L)) + suffhash;

        } else if (body->B - L == 2) {
            prefch[0] = i/body->alphasize;
            prefch[1] = i%body->alphasize;
            prefhash = wm_hash_func(body, prefch, 2);
            hash = (prefhash << (body->alphabits * L)) + suffhash;

        } else
            break;

        body->shifttab[hash] = min(body->patslen - L, body->shifttab[hash]);
    }

    return 0;
}


int wm_pattern_precalc (void * vbody)
{
    WMBody      * body = (WMBody *)vbody;
    int           tabsize = 0;
    int           i, j, k;
    int           hash = 0;
    PatternItem * patitem = NULL;
    uint8       * pbyte = NULL;
    int           shiftlen = 0;
    PrefixItem  * prenode = NULL;
    PrefixItem  * iternode = NULL;

    if (!body) return -1;

    body->patnum = arr_num(body->patlist);
    if (body->patnum > 4096) body->B = 3;

    tabsize = pow(body->alphasize, body->B);

    if (tabsize != body->tabsize) {
        wm_tab_clean(body);

        body->tabsize = tabsize;

        body->shifttab = kalloc(body->tabsize);
        body->prefixtab = (void **)kzalloc(body->tabsize * sizeof(void *));
    }

    for (i = 0; i < body->tabsize; i++) {
        //body->shifttab[i] = body->patslen - body->B + 1;
        body->shifttab[i] = body->patslen;
    }

    for (i = 0; i < body->patnum; i++) {
        patitem = arr_value(body->patlist, i);
        if (!patitem) continue;

        pbyte = patitem->pattern;

        for (k = 0; k < body->B - 1; k++)
            wm_adjust_shifttab(body, pbyte, body->B - 1 - k);

        for (j = body->patslen; j >= body->B; j--) {
            hash = wm_hash_func(body, pbyte + j - body->B, body->B);

            shiftlen = body->patslen - j;

            body->shifttab[hash] = min(body->shifttab[hash], shiftlen);

            if (0 == shiftlen) {
                prenode = kzalloc(sizeof(*prenode));
                prenode->index = i;
                prenode->next = NULL;
                prenode->hash = wm_hash_func(body, pbyte, body->B);

                if ((iternode = body->prefixtab[hash]) == NULL) {
                    body->prefixtab[hash] = prenode;

                } else {
                    while (iternode->next != NULL)
                        iternode = iternode->next;

                    iternode->next = prenode;
                }
            }
        }
    }

    return 0;
}


long wm_bytes_search (void * vbody, void * vbyte, long bytelen, void ** foundobj, int foundexit)
{
    WMBody      * body = (WMBody *)vbody;
    uint8       * pbyte = (uint8 *)vbyte;
    long          iter = 0;
    int           hash1 = 0;
    int           hash2 = 0;
    int           shift = 0;
    PrefixItem  * prenode = NULL;
    PatternItem * patitem = NULL;
    uint8       * target = NULL;
    uint8       * pattern = NULL;
    int           i = 0;
    int           matchnum = 0;
    uint8         found = 0;
    int           skipnum = 0;
    int           maxlen = 0;

    if (!body) return -1;

    if (foundobj) *foundobj = NULL;

    if (bytelen < body->patslen) return -100;

    iter = body->patslen;
    while (iter <= bytelen) {
        hash1 = wm_hash_func(body, pbyte + iter - body->B, body->B);

        shift = body->shifttab[hash1];
        if (shift > 0) {
            iter += shift;

        } else {
            hash2 = wm_hash_func(body, pbyte + iter - body->patslen, body->B);

            prenode = body->prefixtab[hash1];

            for (found=0, skipnum=0; prenode != NULL; prenode = prenode->next) {

                if (hash2 == prenode->hash) {
                    /* prefix block of the pattern has the same hash value with 
                     * the prefix block of text matching window
                     * now compare the text of matching window with the pattern */

                    patitem = arr_value(body->patlist, prenode->index);
                    if (!patitem) continue;

                    pattern = patitem->pattern + body->B;
                    target = pbyte + iter - body->patslen + body->B;

                    for (i = patitem->length - body->B; i > 0; i--) {
                        if (body->alphatab[(int)*pattern].letter == body->alphatab[(int)*target].letter) { 
                            pattern++;
                            target++;

                        } else
                            break;
                    }

                    if (i == 0) {
                        //found one match
                        matchnum++; found = 1;

                        if (maxlen < patitem->length) {
                            maxlen = patitem->length;

                            if (foundobj) *foundobj = patitem->para;
                        }

                        if (patitem->matchsucc) {
                            (*patitem->matchsucc)(patitem->para, 0,
                                       pbyte, bytelen, iter - body->patslen,
                                       &skipnum, patitem->pattern, patitem->length,
                                       NULL, 0, NULL);

                            if (skipnum > 0) break;
                        }
                    }
                }
            }

            if (found && foundexit)
                return iter-body->patslen;

            if (found && skipnum > 0) {
                iter += skipnum;
            } else {
                hash1 = wm_hash_func(body, pbyte + iter - body->B + 1, body->B);
                shift = body->shifttab[hash1];
                iter += shift + 1;
            }
        }
    }

    if (foundexit && matchnum <= 0)
        return -200;

    return iter;
}


int wm_filecache_hash_func (void * vbody, void * fca, long pos, int len)
{
    WMBody * body = (WMBody *)vbody;
    int      value = 0;
    int      i, fchar = 0;
 
    for (i = 0; i < len; i++) {
        value <<= body->alphabits;

        fchar = file_cache_at(fca, i + pos);
        if (fchar < 0) break;

        value += body->alphatab[fchar].offset;
    }
 
    return value;
}

long wm_filecache_search (void * vbody, void * fca, long pos, void ** foundobj, int foundexit)
{ 
    WMBody      * body = (WMBody *)vbody;
    long          filesize;

    long          iter = 0; 
    int           hash1 = 0;
    int           hash2 = 0;
    int           shift = 0;
    PrefixItem  * prenode = NULL;
    PatternItem * patitem = NULL;
    uint8       * pattern = NULL;
    int           i = 0; 
    int           fch = 0;
    int           matchnum = 0;
    uint8         found = 0;
    int           skipnum = 0;
    int           maxlen = 0;
     
    if (!body) return -1;
    if (!fca) return -2;

    if (foundobj) *foundobj = NULL;

    filesize = file_cache_filesize(fca);
    if (filesize < body->patslen) return -100;

    for (iter = pos + body->patslen; iter <= filesize; ) {
        hash1 = wm_filecache_hash_func(body, fca, iter - body->B, body->B);
         
        shift = body->shifttab[hash1];
        if (shift > 0) {
            iter += shift;

        } else {

            hash2 = wm_filecache_hash_func(body, fca, iter - body->patslen, body->B);
            prenode = body->prefixtab[hash1];

            for (found=0, skipnum=0; prenode != NULL; prenode = prenode->next) {

                if (hash2 == prenode->hash) {
                    /* prefix block of the pattern has the same hash value with
                       the prefix block of text matching window
                       now compare the text of matching window with the pattern */
 
                    patitem = arr_value(body->patlist, prenode->index);
                    if (!patitem) continue;
 
                    pattern = patitem->pattern + body->B;
 
                    for (i = 0; i < patitem->length - body->B; i++) {
                        fch = file_cache_at(fca, iter - body->patslen + body->B + i);
                        if (fch < 0) break;

                        if (body->alphatab[(int)*pattern].letter == body->alphatab[fch].letter) {
                            pattern++;

                        } else
                            break;
                    }

                    if (i >= patitem->length - body->B) {
                        //found one match
                        matchnum++; found=1;
 
                        if (maxlen < patitem->length) {
                            maxlen = patitem->length;
                            if (foundobj) *foundobj = patitem->para;
                        }

                        if (patitem->matchsucc) {
                            (*patitem->matchsucc)(patitem->para, 1,
                                       fca, filesize, iter-body->patslen,
                                       &skipnum, patitem->pattern, patitem->length,
                                       NULL, 0, NULL);

                            if (skipnum > 0) break;
                        }
                    }
                }
            }
 
            if (found && foundexit)
                return iter-body->patslen;

            if (found && skipnum > 0) {
                iter += skipnum;

            } else {
                hash1 = wm_filecache_hash_func(body, fca, iter - body->B + 1, body->B);
                shift = body->shifttab[hash1];
                iter += shift + 1;
            }
        }
    }
 
    if (foundexit && matchnum <= 0)
        return -200;

    return iter;
}
 

long wm_file_search (void * vbody, char * file) 
{  
    WMBody      * body = (WMBody *)vbody;
    long          filesize;  
    long          pos = 0;
    void        * fca = NULL;
 
    if (!body) return -1;
    if (!file) return -2;
 
    fca = file_cache_init(10, 8192);
    if (!fca) return -100;
 
    file_cache_set_prefix_ratio(fca, (float)0.6);
    if (file_cache_setfile(fca, file, 0) < 0) {
        file_cache_clean(fca);
        return -200;
    }
 
    filesize = file_cache_filesize(fca);
    if (filesize <= 0) return -101;
 
    pos = wm_filecache_search(body, fca, 0, NULL, 0);

    file_cache_clean(fca);
    return pos;
}
 

long wm_match_replace (void * vbody, void * fca, long pos, uint8 * pdst, int dstlen, int * cpbytes)
{
    WMBody      * body = (WMBody *)vbody;
    long          filesize;
    long          iter = 0;
    long          i = 0;
    int           cpnum = 0;
    int           repnum = 0;

    int           hash1 = 0; 
    int           hash2 = 0;
    int           shift = 0; 
    PrefixItem  * prenode = NULL;
    PatternItem * patitem = NULL;
    uint8       * pattern = NULL;
    int           fch = 0;
    int           matchnum = 0;
    uint8         found = 0;
    int           skipnum = 0;
 
    if (!body) return -1;
    if (!fca) return -2;
 
    filesize = file_cache_filesize(fca);

    if (filesize < body->patslen || pos + body->patslen > filesize) {
        for (i = pos; i < filesize; i++)
            pdst[cpnum++] = file_cache_at(fca, i);

        if (cpbytes) *cpbytes = cpnum;

        return filesize;
    }
 
    for (iter = pos + body->patslen; iter <= filesize && cpnum < dstlen; ) {

        hash1 = wm_filecache_hash_func(body, fca, iter - body->B, body->B);
 
        shift = body->shifttab[hash1];
        if (shift > 0) {
            if (cpnum + shift > dstlen) {
                if (cpbytes) *cpbytes = cpnum;

                return iter-body->patslen;
            }

            for (i = 0; i < shift; i++)
                pdst[cpnum++] = file_cache_at(fca, iter-body->patslen+i);

            iter += shift;

        } else {
            hash2 = wm_filecache_hash_func(body, fca, iter - body->patslen, body->B);
 
            prenode = body->prefixtab[hash1];
            for (found = 0, skipnum = 0; prenode != NULL; prenode = prenode->next) {

                if (hash2 == prenode->hash) {
                    /* prefix block of the pattern has the same hash value with
                     * the prefix block of text matching window
                     * now compare the text of matching window with the pattern */
 
                    patitem = arr_value(body->patlist, prenode->index);
                    if (!patitem) continue;
 
                    pattern = patitem->pattern + body->B;
 
                    for (i = 0; i < patitem->length - body->B; i++) {
                        fch = file_cache_at(fca, iter - body->patslen + body->B + i);
                        if (fch < 0) break;
 
                        if (body->alphatab[(int)*pattern].letter == body->alphatab[fch].letter) {
                            pattern++;

                        } else
                            break;
                    }

                    if (i >= patitem->length - body->B) {
                        //found one match
                        matchnum++; found=1;
 
                        if (patitem->matchsucc) {
                            repnum = 0;
                            (*patitem->matchsucc)(patitem->para, 1,
                                         fca, filesize, iter-body->patslen,
                                         &skipnum, patitem->pattern, patitem->length,
                                         pdst + cpnum, dstlen - cpnum, &repnum);

                            if (cpnum + repnum > dstlen) {
                                if (cpbytes) *cpbytes = cpnum;

                                return iter - body->patslen;
                            }

                            cpnum += repnum;

                            if (skipnum > 0) break;
                        }
                    }
                }
            }
 
            if (found && skipnum > 0) {
                iter += skipnum;

            } else {
                hash1 = wm_filecache_hash_func(body, fca, iter - body->B + 1, body->B);
                shift = body->shifttab[hash1];

                if (cpnum + shift + 1 > dstlen) {
                    if (cpbytes) *cpbytes = cpnum;

                    return iter-body->patslen;
                }

                for (i = 0; i < shift+1; i++)
                    pdst[cpnum++] = file_cache_at(fca, iter-body->patslen+i);

                iter += shift + 1;
            }
        }
    }
 
    for (i = iter - body->patslen; cpnum < dstlen && i < filesize; i++)
        pdst[cpnum++] = file_cache_at(fca, i);

    if (cpbytes) *cpbytes = cpnum;

    return i;
}


long wm_bytes_replace (void * vbody, uint8 * pbyte, long bytelen, uint8 * pdst, int dstlen, int * cpbytes)
{        
    WMBody      * body = (WMBody *)vbody;
    long          iter = 0;
    long          i = 0;
    int           cpnum = 0; 
    int           repnum = 0; 
 
    int           hash1 = 0;  
    int           hash2 = 0;
    int           shift = 0;   
    PrefixItem  * prenode = NULL; 
    PatternItem * patitem = NULL; 
    uint8       * pattern = NULL;
    int           fch = 0;
    int           matchnum = 0; 
    uint8         found = 0;
    int           skipnum = 0;
 
    if (!body) return -1;
    if (!pbyte) return -2;
 
    if (bytelen < body->patslen) {
        for (i = 0; i < bytelen && i < dstlen; i++)
            pdst[cpnum++] = pbyte[i];

        if (cpbytes) *cpbytes = cpnum;

        return i;
    }
 
    for (iter = body->patslen; iter <= bytelen && cpnum < dstlen; ) {

        hash1 = wm_hash_func(body, pbyte + iter - body->B, body->B);
 
        shift = body->shifttab[hash1];
        if (shift > 0) {
            if (cpnum + shift > dstlen) {
                if (cpbytes) *cpbytes = cpnum;
                return iter-body->patslen;
            }
 
            for (i = 0; i < shift; i++)
                pdst[cpnum++] = pbyte[iter - body->patslen + i];

            iter += shift;

        } else {
            hash2 = wm_hash_func(body, pbyte + iter - body->patslen, body->B);
 
            prenode = body->prefixtab[hash1];

            for (found = 0, skipnum = 0; prenode != NULL; prenode = prenode->next) {

                if (hash2 == prenode->hash) {
                    /* prefix block of the pattern has the same hash value with
                     * the prefix block of text matching window
                     * now compare the text of matching window with the pattern */
 
                    patitem = arr_value(body->patlist, prenode->index);
                    if (!patitem) continue;
 
                    pattern = patitem->pattern + body->B;
 
                    for (i = 0; i < patitem->length - body->B; i++) {
                        fch = pbyte[iter - body->patslen + body->B + i];
 
                        if (body->alphatab[(int)*pattern].letter == body->alphatab[fch].letter) {
                            pattern++;

                        } else 
                            break;
                    }

                    if (i >= patitem->length - body->B) {
                        //found one match
                        matchnum++; found=1;
 
                        if (patitem->matchsucc) {
                            repnum = 0;
                            (*patitem->matchsucc)(patitem->para, 0,
                                           pbyte, bytelen, iter - body->patslen,
                                           &skipnum, patitem->pattern, patitem->length,
                                           pdst + cpnum, dstlen - cpnum, &repnum);
 
                            if (cpnum + repnum > dstlen) {
                                if (cpbytes) *cpbytes = cpnum;

                                return iter - body->patslen;
                            }

                            cpnum += repnum;
 
                            if (skipnum > 0) break;
                        }
                    }
                }
            }
 
            if (found && skipnum > 0) {
                iter += skipnum;

            } else {
                hash1 = wm_hash_func(body, pbyte + iter - body->B + 1, body->B);
                shift = body->shifttab[hash1];
 
                if (cpnum + shift + 1 > dstlen) {
                    if (cpbytes) *cpbytes = cpnum;
                    return iter-body->patslen;
                }
 
                for (i = 0; i < shift+1; i++)
                    pdst[cpnum++] = pbyte[iter-body->patslen+i];

                iter += shift + 1;
            }
        }
    }
 
    for (i = iter - body->patslen; cpnum < dstlen && i < bytelen; i++)
        pdst[cpnum++] = pbyte[i];
 
    if (cpbytes) *cpbytes = cpnum;

    return i;
}
 
