/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#include "btype.h"
#include "memory.h"
#include "mpool.h"
#include "strutil.h"
#include "dynarr.h"
#include "hashtab.h"
#include "wordlib.h"


typedef struct worditem_ {
    int        word;

    uint8      phraselen;
    uint8      serial;
    uint8      phrase_end;
    uint8      alloc;

    arr_t    * sublist;
    void     * parent;
    void     * varpara;
    void     * varfree;
} WordItem;

typedef void WordItem_FREE (void * );


void * word_item_alloc ();
int    word_item_free (void * vwi);
int    word_item_recursive_free (void * vwi);
 
static int word_item_cmp_item (void * a, void * b)
{
    WordItem * awi = (WordItem *)a;
    WordItem * bwi = (WordItem *)b;

    return awi->word - bwi->word;
}

static int word_item_cmp_word (void * vwi, void * vword)
{
    WordItem * wi = (WordItem *)vwi;
    int        word = *(int *)vword;

    return wi->word - word;
}

static ulong word_item_hash_func (void * key)
{
    int word = *(int *)key;

    return (ulong)word;
}

 
void * word_item_alloc ()
{
    WordItem * wi = NULL;
 
    wi = kzalloc(sizeof(*wi));
    if (!wi) return NULL;
 
    wi->alloc = 1;
    return wi;
}
 
int word_item_free (void * vwi)
{
    WordItem      * wi = (WordItem *)vwi;
    WordItem_FREE * freefunc = NULL;
 
    if (!wi) return -1;
 
    if (wi->sublist) {
        arr_free(wi->sublist);
        wi->sublist = NULL;
    }

    if (wi->varpara && wi->varfree) {
        freefunc = (WordItem_FREE *)wi->varfree;
        freefunc(wi->varpara);
    }

    if (wi->alloc) kfree(wi);
 
    return 0;
}
 
int word_item_recursive_free (void * vwi)
{
    WordItem * wi = (WordItem *)vwi;
    WordItem * subwi = NULL;
 
    if (!wi) return -1;
 
    while (arr_num(wi->sublist) > 0) {
        subwi = arr_pop(wi->sublist);
        word_item_recursive_free(subwi);
    }

    if (arr_num(wi->sublist) <= 0) {
        word_item_free(wi);
    }
 
    return 0;
}
 
int word_item_init (void * vwi)
{
    WordItem * wi = (WordItem *)vwi;
 
    if (!wi) return -1;
 
    wi->word = 0;
    wi->phraselen = 0;
    wi->serial = 0;
    wi->phrase_end = 0;

    if (wi->sublist) wi->sublist = arr_new(4);
    arr_zero(wi->sublist);

    wi->parent = NULL;
    wi->varpara = NULL;
    wi->varfree = NULL;
 
    return 0;
}

void * word_item_fetch (void * vwlib) 
{
    WordLib  * wlib = (WordLib *)vwlib;
    WordItem * wi = NULL;

    if (!wlib) return NULL;

    wi = mpool_fetch(wlib->itempool);
    if (!wi) wi = word_item_alloc();

    return wi;
}

int word_item_recycle (void * vwlib, void * vwi)
{
    WordLib  * wlib = (WordLib *)vwlib;
    WordItem * wi = (WordItem *)vwi;
    WordItem_FREE * freefunc = NULL;
 
    if (!wlib) return -1;
    if (!wi) return -2;

    if (wi->alloc) return word_item_free(wi);

    if (wi->varpara && wi->varfree) {
        freefunc = (WordItem_FREE *)wi->varfree;
        freefunc(wi->varpara);
    }

    mpool_recycle(wlib->itempool, wi);
    return 0;
}




void * word_lib_create (char * wdir, ulong tabsize, uint8 charset)
{
    WordLib * wlib = NULL;
 
    wlib = kzalloc(sizeof(*wlib));
    if (!wlib) return NULL;
 
    if (wdir)
        strcpy(wlib->word_dir, wdir);

    if (tabsize < 100) tabsize = 100;
    wlib->tabsize = tabsize;

    wlib->charset = charset;
 
    wlib->subtab = ht_new(tabsize, word_item_cmp_word);
    ht_set_hash_func (wlib->subtab, word_item_hash_func);

    wlib->itempool = mpool_alloc();
    mpool_set_unitsize(wlib->itempool, sizeof(WordItem));
    mpool_set_allocnum(wlib->itempool, 1024);
    mpool_set_initfunc(wlib->itempool, word_item_init);
    mpool_set_freefunc(wlib->itempool, word_item_free);

    return wlib;
}
 
int word_lib_clean (void *vwlib)
{
    WordLib   * wlib = (WordLib *)vwlib;
    WordItem  * wi = NULL;
    int         i, num;
 
    if (!wlib) return -1;
 
    if (wlib->subtab) {
        num = ht_num(wlib->subtab);
        for (i = 0; i < num; i++) {
            wi = ht_value(wlib->subtab, i);
            word_item_recursive_free(wi);
        }
        ht_free(wlib->subtab);
        wlib->subtab = NULL;
    }

    if (wlib->itempool) {
        mpool_free(wlib->itempool);
        wlib->itempool = NULL;
    }

    kfree(wlib);
    return 0;
}
 
 
int word_lib_loadfile (void * vwlib, char * file)
{
    WordLib  * wlib = (WordLib *)vwlib;
    FILE     * fp = NULL;
    char       buf[512];
    char     * p = NULL;
    int        ret = 0;
 
    if (!wlib) return -1;
    if (!file) return -2;
 
    fp = fopen(file, "r");
    if (!fp) return -10;
 
    while (!feof(fp)) {
        fgets(buf, sizeof(buf), fp);
        p = str_trim(buf);
        if ((ret = strlen(p)) <= 0) continue;
        if (*p == ';' || *p == '#') continue;
 
        word_lib_add(wlib, p, ret, NULL, NULL);
    }
    fclose(fp);
 
    return 0;
}
 
int word_lib_getword (void * vwlib, void * vbyte, int bytelen, int * word)
{
    WordLib * wlib = (WordLib *)vwlib;
    uint8   * pbyte = (uint8 *)vbyte;
    int       ret = 0;
    uint8   * pend = NULL;
    int       i, j, bytenum = 0;
 
    if (!pbyte || bytelen <= 0) return 0;
    pend = pbyte + bytelen;
 
    if (wlib->charset == WL_ASCII) {   
        if (*pbyte >= 'A' && *pbyte <= 'Z') *word = *pbyte-'A'+'a';
        else *word = *pbyte;
        pbyte++; ret++;
    } else if (wlib->charset == WL_GBK) {   
        *word = *pbyte++; ret++;
        if (pbyte >= pend) return ret;

        if (*word >= 0x80) {
            *word *= 256; *word += *pbyte++; ret++;
        }
    } else if (wlib->charset == WL_UNICODE) {
        *word = *pbyte++; ret++;
        if (pbyte >= pend) return ret;
        *word *= 256; *word += *pbyte++; ret++;
    } else if (wlib->charset == WL_UTF8) {
        i = j = 0;
        if (pbyte[0] <= 0x7F) bytenum = 1;
        else if (pbyte[0] >= 0xC0 && pbyte[0] <= 0xDF) bytenum = 2;
        else if (pbyte[0] >= 0xE0 && pbyte[0] <= 0xEF) bytenum = 3;
        else if (pbyte[0] >= 0xF0 && pbyte[0] <= 0xF7) bytenum = 4;
        else if (pbyte[0] >= 0xF8 && pbyte[0] <= 0xFB) bytenum = 5;
        else if (pbyte[0] >= 0xFC && pbyte[0] <= 0xFD) bytenum = 6;
        else bytenum = 0;

        if (bytenum >= 2)
            *word = pbyte[0] & (0x7F >> bytenum);
        else
            *word = pbyte[0];
        ret++;
        for (i=1, j=0; i<bytelen && j<bytenum-1; j++, i++, ret++) {
            if (pbyte[i] < 0x80 || pbyte[i] > 0xBF) break;
            *word = (*word << 6) + (pbyte[i] & 0x3F);
        }
    } else {
        *word = *pbyte++; ret++;
    }
 
    return ret;
}
 

int word_lib_add (void * vwlib, void * vbyte, int bytelen, void * varpara, void * varfree)
{
    WordLib     * wlib = (WordLib *)vwlib;
    uint8       * pbyte = (uint8 *)vbyte;
    WordItem    * wi = NULL;
    WordItem    * subwi = NULL;
    WordItem      item;
    uint8       * p = pbyte;
    uint8       * pend = NULL;
    int           wordlen = 0;
    int           serial = 0;
    int           phraselen = 0;
 
    if (!wlib) return -1;
    if (!pbyte) return -2;
    if (bytelen < 0) bytelen = str_len(pbyte);
    if (bytelen <= 0) return -3;
 
    pend = pbyte + bytelen;
 
    memset(&item, 0, sizeof(item));
    wordlen = word_lib_getword(wlib, p, pend-p, &item.word); 
    p += wordlen; phraselen += wordlen;
 
    wi = ht_get(wlib->subtab, &item.word);
    if (!wi) {
        wi = word_item_fetch(wlib);
        wi->word = item.word;
        wi->serial = ++serial;
        wi->phraselen = phraselen;
        wi->parent = wlib;
        ht_set(wlib->subtab, &item.word, wi);
    }
 
    for (; p < pend; ) {
        memset(&item, 0, sizeof(item));
        wordlen = word_lib_getword(wlib, p, pend-p, &item.word); 
        p += wordlen; phraselen += wordlen;
 
        if (!wi->sublist) wi->sublist = arr_new(4);
        subwi = arr_find_by(wi->sublist, &item, word_item_cmp_item);
        if (!subwi) {
            subwi = word_item_fetch(wlib);
            subwi->word = item.word;
            subwi->serial = ++serial;
            subwi->phraselen = phraselen;
            subwi->parent = wi;
            arr_insert_by(wi->sublist, subwi, word_item_cmp_item);
        }
 
        wi = subwi;
    }
 
    wi->phrase_end = 1;
    wi->varpara = varpara;
    wi->varfree = varfree;
 
    return 0;
}
 
 
int word_lib_get (void * vwlib, void * vbyte, int bytelen, void ** pvar)
{
    WordLib     * wlib = (WordLib *)vwlib;
    uint8       * pbyte = (uint8 *)vbyte;
    WordItem    * wi = NULL;
    WordItem    * subwi = NULL;
    WordItem      item;
    uint8       * p = pbyte;
    uint8       * pend = NULL;
    int           wordlen = 0;
 
    if (!wlib) return 0;
    if (!pbyte) return 0;
    if (bytelen < 0) bytelen = str_len(pbyte);
    if (bytelen <= 0) return 0;

    pend = pbyte + bytelen;
 
    memset(&item, 0, sizeof(item));
    wordlen = word_lib_getword(wlib, p, pend-p, &item.word); p += wordlen;
 
    wi = ht_get(wlib->subtab, &item.word);
    if (!wi) return 0;
 
    for (; p < pend && arr_num(wi->sublist) > 0; ) {
        memset(&item, 0, sizeof(item));
        wordlen = word_lib_getword(wlib, p, pend-p, &item.word); p += wordlen;
 
        subwi = arr_find_by(wi->sublist, &item, word_item_cmp_item);
        if (!subwi) break;
 
        wi = subwi;
    }
 
    while (wi && wi->serial > 1 && !wi->phrase_end) {
        wi = (WordItem *)wi->parent;
    }
 
    if (wi && wi->phrase_end) {
        if (pvar) *pvar = wi->varpara;
        return wi->phraselen;
    }
 
    return 0;
}
 
 
int word_lib_del (void * vwlib, void * vbyte, int bytelen)
{
    WordLib     * wlib = (WordLib *)vwlib;
    uint8       * pbyte = (uint8 *)vbyte;
    WordItem    * wi = NULL;
    WordItem    * subwi = NULL;
    WordItem      item;
    uint8       * p = pbyte;
    uint8       * pend = NULL;
    int           wordlen = 0;
 
    if (!wlib) return -1;
    if (!pbyte) return -2;
    if (bytelen < 0) bytelen = str_len(pbyte);
    if (bytelen <= 0) return -3;
 
    pend = pbyte + bytelen;
 
    memset(&item, 0, sizeof(item));
    wordlen = word_lib_getword(wlib, p, pend-p, &item.word); p+=wordlen;
 
    wi = ht_get(wlib->subtab, &item.word);
    if (!wi) return -100;
 
    for (; p < pend && arr_num(wi->sublist) > 0; ) {
        memset(&item, 0, sizeof(item));
        wordlen = word_lib_getword(wlib, p, pend-p, &item.word); p+=wordlen;
 
        subwi = arr_find_by(wi->sublist, &item, word_item_cmp_item);
        if (!subwi) break;
 
        wi = subwi;
    }
    if (p < pend) return -10; //not found all words
 
    while (wi && wi->serial > 1) {
        subwi = wi;
        wi = (WordItem *)subwi->parent;

        if (arr_num(subwi->sublist) <= 0) {
            arr_delete_by(wi->sublist, subwi, word_item_cmp_item);
            word_item_recycle(wlib, subwi);
        }
    }

    if (wi && wi->serial == 1) {
        if (arr_num(wi->sublist) <= 0) {
            ht_delete(wlib->subtab, &wi->word);
            word_item_recycle(wlib, wi);
        }
    }
 
    return 0;
}
 

int word_lib_fwmaxmatch (void * vwlib, void * vbyte, int len, void ** pres, int * reslen, void ** pvar)
{
    WordLib     * wlib = (WordLib *)vwlib;
    uint8       * pbyte = (uint8 *)vbyte;
    int           i;
    int           ret = 0;

    if (pres) *pres = NULL;
    if (reslen) *reslen = 0;

    if (!wlib) return -1;
    if (!pbyte) return -2;
    if (len < 0) len = str_len(pbyte);
    if (len <= 0) return -3;

    for (i = 0; i < len; i++) {
        ret = word_lib_get(wlib, pbyte+i, len-i, pvar);
        if (ret > 0) { //matched one words or phrase
            if (pres) *pres = pbyte + i;
            if (reslen) *reslen = ret;
            return i;
        }
    }

    return -100;
}

