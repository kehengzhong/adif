/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#include "btype.h"
#include "memory.h"
#include "hashtab.h"
#include "mthread.h"
#include "strutil.h"
#include "filecache.h"

#include "kvpair.h"

typedef struct CommKey_ {
    uint8   * name;
    int       namelen;
} CommStrKey;


#define kvobjgets(vobj, key, keylen, ind, val, signflg, func)             \
    KVPairObj * obj = (KVPairObj *)vobj;                                  \
    char      * p = NULL;                                                 \
    char      * endp = NULL;                                              \
    int         len = 0;                                                  \
    int         ret = 0;                                                  \
    int         sign = 1;                                                 \
                                                                          \
    if (!obj) return -1;                                                  \
                                                                          \
    if (!key) return -2;                                                  \
    if (keylen < 0) keylen = str_len(key);                                \
    if (keylen <= 0) return -3;                                           \
                                                                          \
    ret = kvpair_getP(obj, key, keylen, ind, (void **)&p, &len);          \
    if (ret < 0) return ret;                                              \
                                                                          \
    if (!p || len <= 0)                                                   \
        return -500;                                                      \
                                                                          \
    if (val) {                                                            \
        while (len > 0 && ISSPACE(*p)) {                                  \
            p++; len--;                                                   \
        }                                                                 \
                                                                          \
        if (len <= 0) {                                                   \
            *val = 0;                                                     \
            return -501;                                                  \
        }                                                                 \
                                                                          \
        if (signflg) {                                                    \
            if (*p == '+') { p++; len--; }                                \
            else if (*p == '-') { p++; len--; sign = -1; }                \
        }                                                                 \
                                                                          \
        if (len > 2 && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {     \
            *val = sign * func(p + 2, &endp, 16);                         \
        } else {                                                          \
            *val = sign * func(p, &endp, 10);                             \
        }                                                                 \
                                                                          \
        if (!endp) return ret;                                            \
    }                                                                     \
    return ret;

#define kvobjget(vobj, key, keylen, ind, val, signflg, func)              \
    KVPairObj * obj = (KVPairObj *)vobj;                                  \
    char      * p = NULL;                                                 \
    char      * endp = NULL;                                              \
    int         len = 0;                                                  \
    int         ret = 0;                                                  \
    int         sign = 1;                                                 \
                                                                          \
    if (!obj) return -1;                                                  \
                                                                          \
    if (!key) return -2;                                                  \
    if (keylen < 0) keylen = str_len(key);                                \
    if (keylen <= 0) return -3;                                           \
                                                                          \
    ret = kvpair_getP(obj, key, keylen, ind, (void **)&p, &len);          \
    if (ret < 0) return ret;                                              \
                                                                          \
    if (!p || len <= 0)                                                   \
        return -500;                                                      \
                                                                          \
    if (val) {                                                            \
        while (len > 0 && ISSPACE(*p)) {                                  \
            p++; len--;                                                   \
        }                                                                 \
                                                                          \
        if (len <= 0) {                                                   \
            *val = 0;                                                     \
            return -501;                                                  \
        }                                                                 \
                                                                          \
        if (signflg) {                                                    \
            if (*p == '+') { p++; len--; }                                \
            else if (*p == '-') { p++; len--; sign = -1; }                \
        }                                                                 \
                                                                          \
        if (len > 2 && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {     \
            *val = sign * func(p + 2, &endp, 16);                         \
        } else {                                                          \
            *val = sign * func(p, &endp, 10);                             \
        }                                                                 \
                                                                          \
        if (!endp) return ret;                                            \
                                                                          \
        endp = skipOver(endp, p+len-endp, " \t\f\b\v\r\n,;", 9);          \
        if ( endp < p+len && ( endp+1 >= p+len || ISSPACE(endp[1])) ) {   \
            if (*endp == 'k' || *endp == 'K')                             \
                *val *= 1024;                                             \
            else if (*endp == 'm' || *endp == 'M')                        \
                *val *= 1024*1024;                                        \
            else if (*endp == 'g' || *endp == 'G')                        \
                *val *= 1024*1024*1024;                                   \
        }                                                                 \
    }                                                                     \
    return ret;

#define kvobjgetd(vobj, key, keylen, ind, val, func)             \
    KVPairObj * obj = (KVPairObj *)vobj;                           \
    char      * p = NULL;                                          \
    int         len = 0;                                           \
    int         ret = 0;                                           \
                                                                   \
    if (!obj) return -1;                                           \
                                                                   \
    if (!key) return -2;                                           \
    if (keylen < 0) keylen = str_len(key);                         \
    if (keylen <= 0) return -3;                                    \
                                                                   \
    ret = kvpair_getP(obj, key, keylen, ind, (void **)&p, &len); \
    if (ret < 0) return ret;                                       \
                                                                   \
    if (!p || len <= 0)                                            \
        return -500;                                               \
                                                                   \
    if (val) {                                                     \
        while (len > 0 && ISSPACE(*p)) {                           \
            p++; len--;                                            \
        }                                                          \
                                                                   \
        if (len <= 0) {                                            \
            *val = 0.;                                             \
            return -501;                                           \
        }                                                          \
                                                                   \
        *val = func(p, NULL);                                      \
    }                                                              \
    return ret;


static ulong commstrkey_hash_func (void * vkey) 
{    
    CommStrKey * key = (CommStrKey *)vkey;
     
    if (!key) return 0; 

    return generic_hash(key->name, key->namelen, 0);
}            

static int kvpair_item_cmp_key (void * a, void * b)
{    
    KVPairItem * item = (KVPairItem *)a;
    CommStrKey * key = (CommStrKey *)b; 
    int          len = 0, ret;
 
    if (!item || !key) return -1;

    if (item->namelen != key->namelen) {
        len = (item->namelen > key->namelen) ? key->namelen : item->namelen;
        if (len <= 0) {
            if (item->namelen > 0) return 1;
            return -1;
        }

        ret = str_ncasecmp(item->name, key->name, len);
        if (ret == 0) {
            if (item->namelen > key->namelen) return 1;
            else return -1;

        } else return ret;
    }
 
    len = item->namelen;
    if (len <= 0) return 0;
 
    return str_ncasecmp(item->name, key->name, len);
}


void * kvpair_value_alloc ()
{
    KVPairValue * jval = NULL;

    jval = kzalloc(sizeof(*jval));
    if (!jval) return NULL;

    jval->value = NULL;
    jval->valuelen = 0;

    return jval;
}

int kvpair_value_free (void * vjval)
{
    KVPairValue * jval = (KVPairValue *)vjval;

    if (!jval) return -1;

    if (jval->value) {
        kfree(jval->value);
        jval->value = NULL;
    }

    kfree(jval);
    return 0;
}


void * kvpair_item_alloc ()
{
    KVPairItem * item = NULL;

    item = kzalloc(sizeof(*item));
    if (!item) return NULL;

    item->name = NULL;
    item->namelen = 0;

    item->valnum = 0;
    item->valobj = NULL;

    return item;
}

int kvpair_item_free (void * vitem)
{
    KVPairItem * item = (KVPairItem *)vitem;

    if (!item) return -1;

    if (item->name) {
        kfree(item->name);
        item->name = NULL;
    }

    if (item->valnum > 1) {
        arr_pop_free((arr_t *)item->valobj, kvpair_value_free);

    } else if (item->valnum == 1) {
        kvpair_value_free(item->valobj);
        item->valobj = NULL;
    }

    kfree(item);
    return 0;
}


void * kvpair_init (int htsize, char * sepa, char * kvsep)
{
    KVPairObj * obj = NULL;

    obj = kzalloc(sizeof(*obj));
    if (!obj) return NULL;

    if (htsize <= 0) htsize = 200;
    obj->htsize = htsize;

    InitializeCriticalSection(&obj->objCS);

    obj->objtab = ht_new(obj->htsize, kvpair_item_cmp_key);
    ht_set_hash_func(obj->objtab, commstrkey_hash_func);

    if (sepa) str_cpy(obj->sepa, sepa);
    if (kvsep) str_cpy(obj->kvsep, kvsep);

    return obj;
}

int kvpair_clean (void * vobj)
{
    KVPairObj * obj = (KVPairObj *)vobj;

    if (!obj) return -1;

    DeleteCriticalSection(&obj->objCS);

    ht_free_all(obj->objtab, kvpair_item_free);

    kfree(obj);
    return 0;
}

int kvpair_zero (void * vobj)
{
    KVPairObj * obj = (KVPairObj *)vobj;
 
    if (!obj) return -1;
 
    ht_free_member(obj->objtab, kvpair_item_free);
 
    return 0;
}

void * kvpair_get_item (void * vobj, void * name, int namelen)
{
    KVPairObj  * obj = (KVPairObj *)vobj;
    KVPairItem * item = NULL;
    CommStrKey   key;

    if (!obj) return NULL;

    if (!name) return NULL;
    if (namelen < 0) namelen = str_len(name);
    if (namelen <= 0) return NULL;

    key.name = name;
    key.namelen = namelen;

    EnterCriticalSection(&obj->objCS);
    item = ht_get(obj->objtab, &key);
    LeaveCriticalSection(&obj->objCS);

    return item;
}

int kvpair_add_item (void * vobj, void * name, int namelen, void * vitem)
{
    KVPairObj  * obj = (KVPairObj *)vobj;
    KVPairItem * item = (KVPairItem *)vitem;
    CommStrKey key;
 
    if (!obj) return -1;
    if (!name) return -2;
    if (namelen < 0) namelen = str_len(name);
    if (namelen <= 0) return -3;
 
    key.name = name;
    key.namelen = namelen;
 
    EnterCriticalSection(&obj->objCS);
    ht_set(obj->objtab, &key, item);
    LeaveCriticalSection(&obj->objCS);
 
    return 0;
}

void * kvpair_del_item (void * vobj, void * name, int namelen)
{ 
    KVPairObj  * obj = (KVPairObj *)vobj;
    KVPairItem * item = NULL; 
    CommStrKey key; 
     
    if (!obj) return NULL;
     
    if (!name) return NULL;
    if (namelen < 0) namelen = str_len(name);
    if (namelen <= 0) return NULL; 
     
    key.name = name;
    key.namelen = namelen;
 
    EnterCriticalSection(&obj->objCS);
    item = ht_delete(obj->objtab, &key);
    LeaveCriticalSection(&obj->objCS);
 
    return item;
}

int kvpair_valuenum (void * vobj, void * key, int keylen)
{
    KVPairObj   * obj = (KVPairObj *)vobj;
    KVPairItem  * item = NULL;
 
    if (!obj) return -1;
 
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
 
    item = kvpair_get_item(obj, key, keylen);
    if (!item) return -100;
 
    return item->valnum;
}

int kvpair_num (void * vobj)
{
    KVPairObj * obj = (KVPairObj *)vobj;
 
    if (!obj) return 0;
 
    return ht_num(obj->objtab);
}

 
int kvpair_seq_get (void * vobj, int seq, int index, void ** pval, int * vallen)
{
    KVPairObj   * obj = (KVPairObj *)vobj;
    KVPairItem  * item = NULL;
    KVPairValue * jval = NULL;
 
    if (pval) *pval = NULL;
    if (vallen) *vallen = 0;
 
    if (!obj) return -1;
 
    EnterCriticalSection(&obj->objCS);
    item = ht_value(obj->objtab, seq);
    LeaveCriticalSection(&obj->objCS);

    if (!item || item->valnum <= 0) return -100;
 
    if (index < 0) index = item->valnum - 1;
    if (index >= item->valnum || index < 0)
        return -200;
 
    if (index == 0 && item->valnum == 1) {
        jval = (KVPairValue *)item->valobj;
 
    } else if (item->valnum > 1) {
        jval = arr_value((arr_t *)item->valobj, index);
    }
 
    if (!jval) return -300;
 
    if (pval) *pval = jval->value;
    if (vallen) *vallen = jval->valuelen;
 
    return item->valnum;
}

int kvpair_getP (void * vobj, void * key, int keylen, int index, void ** pval, int * vallen)
{
    KVPairObj   * obj = (KVPairObj *)vobj;
    KVPairItem  * item = NULL;
    KVPairValue * jval = NULL;
 
    if (pval) *pval = NULL;
    if (vallen) *vallen = 0;

    if (!obj) return -1;
 
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
 
    item = kvpair_get_item(obj, key, keylen);
    if (!item || item->valnum <= 0) return -100;

    if (index < 0) index = item->valnum - 1;
    if (index >= item->valnum || index < 0)
        return -200;
 
    if (index == 0 && item->valnum == 1) {
        jval = (KVPairValue *)item->valobj;

    } else if (item->valnum > 1) {
        jval = arr_value((arr_t *)item->valobj, index);
    }

    if (!jval) return -300;
 
    if (pval) *pval = jval->value;
    if (vallen) *vallen = jval->valuelen;
 
    return item->valnum;
}
 
int kvpair_get (void * vobj, void * key, int keylen, int index, void * val, int * vallen)
{
    void      * p = NULL;
    int         len = 0;
    int         ret = 0;
 
    ret = kvpair_getP(vobj, key, keylen, index, &p, &len);
    if (ret <= 0) {
        if (vallen) *vallen = 0;
        return ret;
    }
 
    if (p && val) {
        if (vallen && *vallen < len)
            memcpy(val, p, *vallen);
        else
            memcpy(val, p, len);
    }
 
    if (vallen) *vallen = len;

    return ret;
}


int kvpair_del (void * vobj, void * key, int keylen, int index)
{ 
    KVPairObj   * obj = (KVPairObj *)vobj;
    KVPairItem  * item = NULL; 
    KVPairValue * jval = NULL;
     
    if (!obj) return -1;
     
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3; 
     
    item = kvpair_get_item(obj, key, keylen);
    if (!item) return -100;

    if (index < 0 || item->valnum <= 1) {
        kvpair_del_item(obj, key, keylen);

        kvpair_item_free(item);
        return 0; 
    }

    if (index < 0) index = item->valnum - 1; 
    if (index >= item->valnum || index < 0) return -200;
     
    jval = arr_delete((arr_t *)item->valobj, index);
    kvpair_value_free(jval);

    if (item->valnum == 2) {
        jval = arr_delete((arr_t *)item->valobj, 0);
        arr_free((arr_t *)item->valobj);
        item->valobj = jval;
    }
 
    item->valnum--;

    return item->valnum;
}
 

int kvpair_get_int8 (void * vobj, void * key, int keylen, int index, int8 * val)
{
    kvobjgets(vobj, key, keylen, index, val, 1, strtol);
}
 
int kvpair_get_uint8 (void * vobj, void * key, int keylen, int index, uint8 * val)
{
    kvobjgets(vobj, key, keylen, index, val, 0, strtoul);
}
 
int kvpair_get_int16 (void * vobj, void * key, int keylen, int index, int16 * val)
{    
    kvobjgets(vobj, key, keylen, index, val, 1, strtol);
}
 
int kvpair_get_uint16 (void * vobj, void * key, int keylen, int index, uint16 * val)
{
    kvobjgets(vobj, key, keylen, index, val, 0, strtoul);
}
 
int kvpair_get_int (void * vobj, void * key, int keylen, int index, int * val)
{
    kvobjget(vobj, key, keylen, index, val, 1, strtol);
}

int kvpair_get_uint32 (void * vobj, void * key, int keylen, int index, uint32 * val)
{
    kvobjget(vobj, key, keylen, index, val, 0, strtoul);
}

int kvpair_get_long (void * vobj, void * key, int keylen, int index, long * val)
{
    kvobjget(vobj, key, keylen, index, val, 1, strtol);
}

int kvpair_get_ulong (void * vobj, void * key, int keylen, int index, ulong * val)
{
    kvobjget(vobj, key, keylen, index, val, 0, strtoul);
}

int kvpair_get_int64 (void * vobj, void * key, int keylen, int index, int64 * val)
{
    kvobjget(vobj, key, keylen, index, val, 1, strtoll);
}

int kvpair_get_uint64 (void * vobj, void * key, int keylen, int index, uint64 * val)
{
    kvobjget(vobj, key, keylen, index, val, 0, strtoull);
}
 
int kvpair_get_double (void * vobj, void * key, int keylen, int index, double * val)
{
    kvobjgetd(vobj, key, keylen, index, val, strtod);
}
 
 
int kvpair_add (void * vobj, void * key, int keylen, void * val, int vallen)
{
    KVPairObj   * obj = (KVPairObj *)vobj;
    KVPairItem  * item = NULL;
    KVPairValue * jval = NULL;
    arr_t       * vallist = NULL;

    if (!obj) return -1;

    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;

    if (val && vallen < 0) vallen = str_len(val);

    item = kvpair_get_item(obj, key, keylen);
    if (!item) {
        item = kvpair_item_alloc();
        item->name = str_dup(key, keylen);
        item->namelen = keylen;
        kvpair_add_item(obj, key, keylen, item);
    }

    jval = kvpair_value_alloc();
    if (val && vallen > 0) {
        jval->value = str_dup(val, vallen);
        jval->valuelen = vallen;
    } else {
        jval->value = kzalloc(1);
        jval->valuelen = 0;
    }

    if (item->valnum == 0) {
        item->valobj = jval;

    } else if (item->valnum == 1) {
        vallist = arr_new(4);
        arr_push(vallist, item->valobj);
        arr_push(vallist, jval);
        item->valobj = vallist;

    } else {
        arr_push((arr_t *)item->valobj, jval);
    }

    item->valnum++;

    return item->valnum;
}

int kvpair_add_by_fca (void * vobj, void * fca, long keypos, int keylen, long valpos, int vallen)
{ 
    KVPairObj   * obj = (KVPairObj *)vobj;
    KVPairItem  * item = NULL; 
    KVPairValue * jval = NULL;
    arr_t       * vallist = NULL;
    uint8       * key = NULL;
     
    if (!obj) return -1;
    if (!fca) return -2;
 
    if (keylen <= 0) return -3;
    if (vallen < 0) return -100;
 
    key = kzalloc(keylen + 1);
    file_cache_seek(fca, keypos);
    file_cache_read(fca, key, keylen, 0);

    item = kvpair_get_item(obj, key, keylen);
    if (!item) {
        item = kvpair_item_alloc();
        item->name = key;
        item->namelen = keylen;
        kvpair_add_item(obj, key, keylen, item);
    } else {
        kfree(key);
    }
 
    jval = kvpair_value_alloc();
    jval->value = kzalloc(vallen+1);
    jval->valuelen = vallen;
 
    if (vallen > 0) {
        file_cache_seek(fca, valpos);
        file_cache_read(fca, jval->value, vallen, 0);
    }

    if (item->valnum == 0) {
        item->valobj = jval;

    } else if (item->valnum == 1) {
        vallist = arr_new(4);
        arr_push(vallist, item->valobj);
        arr_push(vallist, jval);
        item->valobj = vallist;

    } else {
        arr_push((arr_t *)item->valobj, jval);
    }
    item->valnum++;
 
    return item->valnum;
}
 

int kvpair_add_int8 (void * vobj, void * key, int keylen, int8 val)
{
    KVPairObj  * obj = (KVPairObj *)vobj;
    char      buf[32];
 
    if (!obj) return -1;
 
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
 
    sprintf(buf, "%d", val);
    return kvpair_add(obj, key, keylen, buf, -1);
}
 
int kvpair_add_uint8 (void * vobj, void * key, int keylen, uint8 val)
{
    KVPairObj  * obj = (KVPairObj *)vobj;
    char      buf[64];
 
    if (!obj) return -1;
 
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
 
    sprintf(buf, "%u", val);
    return kvpair_add(obj, key, keylen, buf, -1);
}
 

int kvpair_add_int16 (void * vobj, void * key, int keylen, int16 val)
{
    KVPairObj  * obj = (KVPairObj *)vobj;
    char      buf[32]; 
     
    if (!obj) return -1;
     
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
     
    sprintf(buf, "%d", val);
    return kvpair_add(obj, key, keylen, buf, -1);
}    
         
int kvpair_add_uint16 (void * vobj, void * key, int keylen, uint16 val)
{        
    KVPairObj  * obj = (KVPairObj *)vobj;
    char      buf[64];
 
    if (!obj) return -1;
 
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
 
    sprintf(buf, "%u", val);
    return kvpair_add(obj, key, keylen, buf, -1);
}
 

int kvpair_add_int (void * vobj, void * key, int keylen, int val)
{
    KVPairObj  * obj = (KVPairObj *)vobj;
    char      buf[32];

    if (!obj) return -1;

    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;

    sprintf(buf, "%d", val);
    return kvpair_add(obj, key, keylen, buf, -1);
}

int kvpair_add_uint32 (void * vobj, void * key, int keylen, uint32 val)
{
    KVPairObj  * obj = (KVPairObj *)vobj;
    char      buf[64];  
 
    if (!obj) return -1; 
 
    if (!key) return -2; 
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3; 
 
    sprintf(buf, "%u", val); 
    return kvpair_add(obj, key, keylen, buf, -1);
}

int kvpair_add_long (void * vobj, void * key, int keylen, long val)
{
    KVPairObj  * obj = (KVPairObj *)vobj;
    char      buf[64];  
 
    if (!obj) return -1; 
 
    if (!key) return -2; 
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3; 
 
    sprintf(buf, "%ld", val); 
    return kvpair_add(obj, key, keylen, buf, -1);
}

int kvpair_add_ulong (void * vobj, void * key, int keylen, ulong val)
{
    KVPairObj  * obj = (KVPairObj *)vobj;
    char      buf[64];
 
    if (!obj) return -1;
 
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
 
    sprintf(buf, "%lu", val);
    return kvpair_add(obj, key, keylen, buf, -1);
}

int kvpair_add_int64 (void * vobj, void * key, int keylen, int64 val)
{
    KVPairObj  * obj = (KVPairObj *)vobj;
    char      buf[64];
 
    if (!obj) return -1;
 
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
 
#if defined(_WIN32) || defined(_WIN64)
    sprintf(buf, "%I64d", val);
#else
    sprintf(buf, "%lld", val);
#endif
    return kvpair_add(obj, key, keylen, buf, -1);
}

int kvpair_add_uint64 (void * vobj, void * key, int keylen, uint64 val)
{
    KVPairObj  * obj = (KVPairObj *)vobj;
    char       buf[64];
 
    if (!obj) return -1;
 
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
 
#if defined(_WIN32) || defined(_WIN64)
    sprintf(buf, "%I64u", val);
#else
    sprintf(buf, "%llu", val);
#endif
    return kvpair_add(obj, key, keylen, buf, -1);
}

int kvpair_add_double (void * vobj, void * key, int keylen, double val)
{
    KVPairObj  * obj = (KVPairObj *)vobj;
    char       buf[64];
 
    if (!obj) return -1;
 
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
 
    sprintf(buf, "%f", val);
    return kvpair_add(obj, key, keylen, buf, -1);
}


int kvpair_decode (void * vobj, void * vbyte, int length)
{
    KVPairObj  * obj = (KVPairObj *)vobj;
    uint8      * pbyte = (uint8 *)vbyte;
    char         sepa[32];
    uint8      * pbgn = NULL;
    uint8      * pend = NULL;

    uint8      * pcolon = NULL;
    uint8      * pcomma = NULL;
    uint8      * poct = NULL;

    uint8      * name = NULL;
    int          namelen = 0;
    uint8      * value = NULL;
    int          valuelen = 0;

    if (!obj) return 0;
    if (!pbyte) return 0;
    if (length < 0) length = str_len(pbyte);
    if (length <= 0) return 0;

    sprintf(sepa, "%s%s", obj->kvsep, obj->sepa);

    pbgn = pbyte; pend = pbgn + length;

    do {
        namelen = valuelen = 0;

        /* fid=ab9282784398292fd98acf&mimeid=4007&fname="king.mp4"&keyid=783 */

        pbgn = skipOver(pbgn, pend-pbgn, sepa, str_len(sepa));
        if (pbgn >= pend) return pbgn - pbyte;

        pcolon = skipQuoteTo(pbgn, pend-pbgn, sepa, str_len(sepa));

        if (*pbgn == '"' || *pbgn == '\'') {
            name = pbgn+1;
            poct = skipTo(pbgn+1, pcolon-pbgn-1, pbgn, 1);
            poct--;
        } else {
            name = pbgn;
            poct = pcolon-1;
        }
        namelen = poct - name + 1;

        if (memchr(obj->kvsep, *pcolon, str_len(obj->kvsep))) {
            poct = pcolon+1;
            if (poct >= pend) return poct - pbyte;

            pcomma = skipQuoteTo(poct, pend-poct, obj->sepa, str_len(obj->sepa));
            if (*poct == '"' || *poct == '\'') {
                value = poct + 1;
                poct = skipTo(poct+1, pcomma-poct-1, poct, 1);
                poct--;
            } else {
                value = poct;
                poct = pcomma - 1;
            }
            valuelen = poct - value + 1;
            pbgn = pcomma;
        } else {
            pbgn = pcolon;
        }

        kvpair_add(obj, name, namelen, value, valuelen);

    } while (pbgn < pend);

    return pbgn-pbyte;
}
 
long kvpair_fca_decode (void * vobj, void * fca, long startpos, long length)
{ 
    KVPairObj  * obj = (KVPairObj *)vobj;
    char         sepa[32];
    long       fsize = 0;
    long       iter = 0;
    long       endpos = 0;
    int        fch = 0;
    uint8      pat[8];

    long       colon = 0;
    long       comma = 0;
    long       pos = 0;

    long       namepos = 0;
    long       namelen = 0;
    long       valuepos = 0;
    long       valuelen = 0;

    if (!obj) return 0;
    if (!fca) return 0;

    fsize = file_cache_filesize(fca);
    if (startpos >= fsize) return 0;
    if (length < 0) length = fsize;

    sprintf(sepa, "%s%s", obj->kvsep, obj->sepa);

    iter = startpos;
    endpos = iter + length;
    if (endpos > fsize) endpos = fsize;
    if (endpos <= startpos) return 0;

    while (iter < endpos) {
        namelen = valuelen = 0;

        iter = file_cache_skip_over(fca, iter, endpos-iter, sepa, str_len(sepa));
        if (iter >= endpos) return iter - startpos;
 
        colon = file_cache_skip_to(fca, iter, endpos-iter, sepa, str_len(sepa));

        if ((fch = file_cache_at(fca, iter)) == '"' || fch == '\'') {
            namepos = iter + 1;
            pat[0] = fch;
            pos = file_cache_skip_to(fca, iter+1, colon-iter-1, pat, 1);
            pos--;
        } else {
            namepos = iter;
            pos = colon - 1;
        }
        namelen = pos - namepos + 1;

        fch = file_cache_at(fca, colon);
        if (memchr(obj->kvsep, fch, str_len(obj->kvsep))) {
            pos = colon + 1;
            if (pos >= endpos) return pos - startpos;

            comma = file_cache_skip_quote_to(fca, pos, endpos-pos, obj->sepa, str_len(obj->sepa));
            fch = file_cache_at(fca, pos);
            if (fch == '"' || fch == '\'') {
                valuepos = pos + 1;
                pat[0] = fch;
                pos = file_cache_skip_to(fca, pos+1, comma-pos-1, pat, 1);
                pos--;
            } else {
                valuepos = pos;
                pos = comma - 1;
            }
            valuelen = pos - valuepos + 1;
            iter = comma;
        } else {
            iter = colon;
        }
        kvpair_add_by_fca(obj, fca, namepos, namelen, valuepos, valuelen);
    }
 
    return iter-startpos;
}
 

int kvpair_value_encode (void * vjval, uint8 * pbyte, int len)
{
    KVPairValue * jval = (KVPairValue *)vjval;
    int           iter = 0;

    if (!jval) return 0;
    if (!pbyte || len <= 0) return 0;

    if (iter + 1 <= len) pbyte[iter] = '"';
    iter++;

    if (jval->value && jval->valuelen > 0) {
        if (iter + jval->valuelen <= len) 
            memcpy(pbyte + iter, jval->value, jval->valuelen);

        iter += jval->valuelen;
    }

    if (iter + 1 <= len) pbyte[iter] = '"';
    iter++;

    return iter;
}

int kvpair_encode (void * vobj, void * vbyte, int len)
{
    KVPairObj   * obj = (KVPairObj *)vobj;
    uint8       * pbyte = (uint8 *)vbyte;
    KVPairItem  * item = NULL;
    KVPairValue * jval = NULL;
    int           iter = 0;
    int           i, j, num;
    uint8         needcomma = 0;

    if (!obj) return 0;

    num = ht_num(obj->objtab);
    for (i = 0; i < num; i++) {
        item = (KVPairItem *)ht_value(obj->objtab, i);

        if (!item || !item->name || item->namelen <= 0) continue;
        if (item->valnum <= 0) continue;

        if (needcomma && str_len(obj->sepa) > 0) {
            if (iter + 1 <= len) pbyte[iter] = obj->sepa[0];
            iter++;
        }

        if (iter + item->namelen <= len)
            memcpy(pbyte+iter, item->name, item->namelen);
        iter += item->namelen;

        if (iter + 1 <= len) pbyte[iter] = obj->kvsep[0];
        iter++;

        if (item->valnum == 1) {
            jval = (KVPairValue *)item->valobj;
            if (jval) iter += kvpair_value_encode(jval, pbyte+iter, len-iter);

        } else {
            for (j = 0; j < item->valnum; j++) {
                if (j > 0) {
                    if (str_len(obj->sepa) > 0) {
                        if (iter + 1 <= len) pbyte[iter] = obj->sepa[0];
                        iter++;
                    }

                    if (iter + item->namelen <= len)
                        memcpy(pbyte+iter, item->name, item->namelen);
                    iter += item->namelen;

                    if (iter+1 <= len) pbyte[iter] = obj->kvsep[0];
                    iter++;
                }

                jval = arr_value((arr_t *)item->valobj, j);
                if (jval) iter += kvpair_value_encode(jval, pbyte+iter, len-iter);
            }
        }

        needcomma = 1;
    }

    return iter;
}

