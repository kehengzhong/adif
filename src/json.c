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
#include "patmat.h"
#include "fileop.h"
#include "nativefile.h"

#include <fcntl.h>
#ifdef UNIX
#include <sys/mman.h>
#endif

#include "json.h"


#define objgets(vobj, key, keylen, val, mget, ind, signflg, func)         \
    JsonObj  * obj = (JsonObj *)vobj;                                     \
    char     * p = NULL;                                                  \
    char     * endp = NULL;                                               \
    int        len = 0;                                                   \
    int        ret = 0;                                                   \
    int        sign = 1;                                                  \
                                                                          \
    if (!obj) return -1;                                                  \
                                                                          \
    if (!key) return -2;                                                  \
    if (keylen < 0) keylen = str_len(key);                                \
    if (keylen <= 0) return -3;                                           \
                                                                          \
    if (mget)                                                             \
        ret = json_mgetP(obj, key, keylen, (void **)&p, &len);            \
    else                                                                  \
        ret = json_getP(obj, key, keylen, ind, &p, &len);                 \
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


#define objget(vobj, key, keylen, val, mget, ind, signflg, func)          \
    JsonObj  * obj = (JsonObj *)vobj;                                     \
    char     * p = NULL;                                                  \
    char     * endp = NULL;                                               \
    int        len = 0;                                                   \
    int        ret = 0;                                                   \
    int        sign = 1;                                                  \
                                                                          \
    if (!obj) return -1;                                                  \
                                                                          \
    if (!key) return -2;                                                  \
    if (keylen < 0) keylen = str_len(key);                                \
    if (keylen <= 0) return -3;                                           \
                                                                          \
    if (mget)                                                             \
        ret = json_mgetP(obj, key, keylen, (void **)&p, &len);            \
    else                                                                  \
        ret = json_getP(obj, key, keylen, ind, &p, &len);                 \
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

#define objgetd(vobj, key, keylen, val, mget, ind, func)       \
    JsonObj  * obj = (JsonObj *)vobj;                          \
    char     * p = NULL;                                       \
    int        len = 0;                                        \
    int        ret = 0;                                        \
                                                               \
    if (!obj) return -1;                                       \
                                                               \
    if (!key) return -2;                                       \
    if (keylen < 0) keylen = str_len(key);                     \
    if (keylen <= 0) return -3;                                \
                                                               \
    if (mget)                                                  \
        ret = json_mgetP(obj, key, keylen, (void **)&p, &len); \
    else                                                       \
        ret = json_getP(obj, key, keylen, ind, &p, &len);      \
    if (ret < 0) return ret;                                   \
                                                               \
    if (!p || len <= 0)                                        \
        return -500;                                           \
                                                               \
    if (val) {                                                 \
        while (len > 0 && ISSPACE(*p)) {                       \
            p++; len--;                                        \
        }                                                      \
                                                               \
        if (len <= 0) {                                        \
            *val = 0.;                                         \
            return -501;                                       \
        }                                                      \
                                                               \
        *val = func(p, NULL);                                  \
    }                                                          \
    return ret;


static int json_item_cmp_key (void * a, void * b)
{    
    JsonItem * item = (JsonItem *)a;
    ckstr_t  * key = (ckstr_t *)b; 
    int        len = 0, ret;
 
    if (!item || !key) return -1;

    if (item->namelen != key->len) {

        len = (item->namelen > key->len) ? key->len : item->namelen;

        if (len <= 0) {
            if (item->namelen > 0) return 1;
            return -1;
        }

        ret = str_ncasecmp(item->name, key->p, len);
        if (ret == 0) {
            if (item->namelen > key->len) return 1;

            else return -1;

        } else return ret;
    }
 
    len = item->namelen;
    if (len <= 0) return 0;
 
    return str_ncasecmp(item->name, key->p, len);
}


void * json_value_alloc ()
{
    JsonValue * jval = NULL;

    jval = kzalloc(sizeof(*jval));
    if (!jval) return NULL;

    jval->valtype = 0;

    jval->value = NULL;
    jval->valuelen = 0;
    jval->jsonobj = NULL;

    return jval;
}

int json_value_free (void * vjval)
{
    JsonValue * jval = (JsonValue *)vjval;

    if (!jval) return -1;

    if (jval->jsonobj) {
        json_clean(jval->jsonobj);
        jval->jsonobj = NULL;
    }

    if (jval->value) {
        kfree(jval->value);
        jval->value = NULL;
    }

    kfree(jval);
    return 0;
}


void * json_item_alloc ()
{
    JsonItem * item = NULL;

    item = kzalloc(sizeof(*item));
    if (!item) return NULL;

    item->valtype = 0;

    item->name = NULL;
    item->namelen = 0;

    item->arrflag = 0;
    item->valnum = 0;
    item->valobj = NULL;

    return item;
}

int json_item_free (void * vitem)
{
    JsonItem * item = (JsonItem *)vitem;

    if (!item) return -1;

    if (item->name) {
        kfree(item->name);
        item->name = NULL;
    }

    if (item->valnum > 1) {
        arr_pop_free((arr_t *)item->valobj, json_value_free);

    } else if (item->valnum == 1) {
        json_value_free(item->valobj);
        item->valobj = NULL;
    }

    kfree(item);
    return 0;
}


void * json_init (int sptype, int cmtflag, int sibcoex)
{
    JsonObj * obj = NULL;

    obj = kzalloc(sizeof(*obj));
    if (!obj) return NULL;

    InitializeCriticalSection(&obj->objCS);

    obj->objtab = ht_new(200, json_item_cmp_key);
    ht_set_hash_func(obj->objtab, ckstr_string_hash);

    obj->bytenum = 0;

    obj->sptype = (sptype == 0 ? 0 : 1);
    obj->cmtflag = cmtflag;
    obj->sibcoex = sibcoex;

    str_cpy(obj->sp, " \t\r\n\f\v");
    obj->splen = 6;
 
    str_cpy(obj->arrsp, ", \t\r\n\f\v");
    obj->arrsplen = 7;
 
    if (obj->sptype == 0) {
        str_cpy(obj->kvsp, ", \t\r\n\f\v");
        obj->kvsplen = 7;
 
        obj->kvsep = ':';
        obj->itemsep = ',';
 
        str_cpy(obj->keyend, ":,}");
        obj->keyendlen = 3;

        str_cpy(obj->arrend, ",]}");
        obj->arrendlen = 3;

        str_cpy(obj->kvend, ",}");
        obj->kvendlen = 2;

    } else {
        str_cpy(obj->kvsp, "; \t\r\n\f\v");
        obj->kvsplen = 7;
 
        obj->kvsep = '=';
        obj->itemsep = ';';
 
        str_cpy(obj->keyend, "=;}");
        obj->keyendlen = 3;
 
        str_cpy(obj->arrend, ",];}");
        obj->arrendlen = 4;
         
        str_cpy(obj->kvend, ";}");
        obj->kvendlen = 2;
    }

    return obj;
}

int json_clean (void * vobj)
{
    JsonObj * obj = (JsonObj *)vobj;

    if (!obj) return -1;

    DeleteCriticalSection(&obj->objCS);

    ht_free_all(obj->objtab, json_item_free);

    kfree(obj);
    return 0;
}

int json_size (void * vobj)
{
    JsonObj   * obj = (JsonObj *)vobj;
    JsonObj   * subobj = NULL;
    int         size = 0;

    JsonItem  * item = NULL;
    JsonValue * jval = NULL;
    int         i, j, num;

    if (!obj) return 0;

    size = obj->bytenum;

    num = ht_num(obj->objtab);

    for (i = 0; i < num; i++) {

        item = (JsonItem *)ht_value(obj->objtab, i);
        if (!item || item->valnum <= 0) continue;

        if (item->valnum == 1) {

            jval = (JsonValue *)item->valobj;
            if (!jval || jval->valtype == 0) continue;

            subobj = jval->jsonobj;
            if (subobj) size += json_size(subobj);

        } else {

            for (j = 0; j < item->valnum; j++) {
                jval = arr_value((arr_t *)item->valobj, j);
                if (!jval || jval->valtype == 0) continue;

                subobj = jval->jsonobj;
                if (subobj) size += json_size(subobj);
            }
        }
    }

    //size = (size + 1024)/1024 * 1024;
    return size;
}


int json_valuenum (void * vobj, void * key, int keylen)
{
    JsonObj   * obj = (JsonObj *)vobj;
    JsonItem  * item = NULL;
 
    if (!obj) return -1;
 
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
 
    item = json_item_get(obj, key, keylen);
    if (!item) return -100;
 
    return item->valnum;
}

int json_num (void * vobj)
{
    JsonObj * obj = (JsonObj *)vobj;

    if (!obj) return 0;

    return ht_num(obj->objtab);
}

void * json_item_get (void * vobj, void * name, int namelen)
{
    JsonObj    * obj = (JsonObj *)vobj;
    JsonItem   * item = NULL;
    ckstr_t      key;

    if (!obj) return NULL;

    if (!name) return NULL;
    if (namelen < 0) namelen = str_len(name);
    if (namelen <= 0) return NULL;

    key.p = name;
    key.len = namelen;

    EnterCriticalSection(&obj->objCS);
    item = ht_get(obj->objtab, &key);
    LeaveCriticalSection(&obj->objCS);

    return item;
}

int json_item_add (void * vobj, void * name, int namelen, void * vitem)
{
    JsonObj  * obj = (JsonObj *)vobj;
    JsonItem * item = (JsonItem *)vitem;
    ckstr_t    key;
 
    if (!obj) return -1;
    if (!name) return -2;
    if (namelen < 0) namelen = str_len(name);
    if (namelen <= 0) return -3;
 
    key.p = name;
    key.len = namelen;
 
    EnterCriticalSection(&obj->objCS);
    ht_set(obj->objtab, &key, item);
    LeaveCriticalSection(&obj->objCS);
 
    return 0;
}

void * json_item_del (void * vobj, void * name, int namelen)
{
    JsonObj    * obj = (JsonObj *)vobj;
    JsonItem   * item = NULL;
    ckstr_t      key;
 
    if (!obj) return NULL;
 
    if (!name) return NULL;
    if (namelen < 0) namelen = str_len(name);
    if (namelen <= 0) return NULL;
 
    key.p = name;
    key.len = namelen;
 
    EnterCriticalSection(&obj->objCS);
    item = ht_delete(obj->objtab, &key);
    LeaveCriticalSection(&obj->objCS);
 
    return item;
}


int json_iter (void * vobj, int ind, int valind, void ** pkey, int * keylen, void ** pval, int * vallen, void ** pobj)
{
    JsonObj   * obj = (JsonObj *)vobj;
    JsonItem  * item = NULL;
    JsonValue * jval = NULL;
    int         num;
 
    if (pkey) *pkey = NULL;
    if (keylen) *keylen = 0;

    if (pval) *pval = NULL;
    if (vallen) *vallen = 0;

    if (!obj) return -1;

    num = ht_num(obj->objtab);

    if (ind < 0 || ind >= num) return -100;

    item = ht_value(obj->objtab, ind);
    if (!item) return -200;

    if (pkey) *pkey = item->name;
    if (keylen) *keylen = item->namelen;

    if (item->valnum == 1) {
        jval = (JsonValue *)item->valobj;
 
    } else if (item->valnum > 1) {
        if (valind < 0 || valind >= item->valnum)
            return -300;

        jval = arr_value((arr_t *)item->valobj, valind);

    } else {
        return -301;
    }

    if (jval->valtype == 0) { //generic string
        if (pval) *pval = jval->value;
        if (vallen) *vallen = jval->valuelen;
        if (pobj) *pobj = NULL;
 
    } else { //object
        if (pval) *pval = NULL;
        if (vallen) *vallen = 0;
        if (pobj) *pobj = jval->jsonobj;
    }

    return item->valnum;
}


/* "http.server.location[0].errpage.504" : "504.html" */
int json_mdel (void * vobj, void * key, int keylen)
{
    JsonObj   * obj = (JsonObj *)vobj;
    JsonItem  * item = NULL;
    JsonValue * jval = NULL;
 
    JsonObj   * subobj = NULL;
    char      * plist[32];
    int         plen[32];
    char      * name = NULL;
    int         i, num = 0;
    char      * sublist[3];
    int         sublen[3];
    int         subnum = 0;
    int         index = 0;
 
    if (!obj) return -1;
 
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
 
    num = string_tokenize(key, keylen, ".", 1, (void **)plist, plen, 32);
 
    for (i = 0, subobj = obj; i < num && subobj; i++) {
        name = plist[i]; keylen = plen[i];
 
        if (plen[i] >= 2) {
            if ((name[0] == '"' || name[0] == '\'') && name[keylen - 1] == name[0]) {
                name++; keylen -= 2;
            }
        }
 
        subnum = string_tokenize(name, keylen, "[]", 2, (void **)sublist, sublen, 3);
 
        if (subnum > 1) index = atoi(sublist[1]);
        else index = 0;
 
        item = json_item_get(subobj, sublist[0], sublen[0]);
        if (!item) return -100;

        if (i == num - 1) { //last sub-item is to be deleted
            if (subnum <= 1 || (sublist[1][0] < '0' || sublist[1][0] > '9') || item->valnum <= 1) {
                /* delete all locations
                   1. json_del(obj, "http.server.location", -1) or 
                   2. json_del(obj, "http.server.location[]", -1) or
                   3. not more than 1 item value */
                item = json_item_del(subobj, sublist[0], sublen[0]);
                json_item_free(item);
                return 1;
            }

            if (index < 0) index = item->valnum - 1;
            if (index >= 0 && index < item->valnum && item->valnum > 1) {
                jval = arr_delete((arr_t *)item->valobj, index);
                json_value_free(jval);
                return 1;
            }

            return 0;
        }

        if (item->valnum <= 0)
            return -101;
 
        if (index < 0) index = item->valnum - 1;
        if (index >= item->valnum || index < 0)
            return -200;
 
        if (index == 0 && item->valnum == 1) {
            jval = (JsonValue *)item->valobj;
 
        } else if (item->valnum > 1) {
            jval = arr_value((arr_t *)item->valobj, index);
        }
 
        if (!jval) return -300;
 
        if (jval->valtype == 0) { //generic string
            break;
        } else { //object
            subobj = jval->jsonobj;
        }
    }
 
    return -400;
}


/* "http.server.location[0].errpage.504" : "504.html" */
int json_mget_value (void * vobj, void * key, int keylen, void ** pval, int * vallen, void ** pobj)
{
    JsonObj   * obj = (JsonObj *)vobj;
    JsonItem  * item = NULL;
    JsonValue * jval = NULL;
 
    JsonObj   * subobj = NULL;
    char      * plist[32];
    int         plen[32];
    char      * name = NULL;
    int         i, num = 0;
    void      * sublist[3];
    int         sublen[3];
    int         subnum = 0;
    int         index = 0;

    if (pval) *pval = NULL;
    if (vallen) *vallen = 0;

    if (!obj) return -1;
 
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
 
    num = string_tokenize(key, keylen, ".", 1, (void **)plist, plen, 32);

    for (i = 0, subobj = obj; i < num && subobj; i++) {
        name = plist[i]; keylen = plen[i];

        if (plen[i] >= 2) {
            if ((name[0] == '"' || name[0] == '\'') && name[keylen - 1] == name[0]) {
                name++; keylen -= 2;
            }
        }

        subnum = string_tokenize(name, keylen, "[]", 2, sublist, sublen, 3);

        if (subnum > 1) index = atoi(sublist[1]);
        else index = 0;

        item = json_item_get(subobj, sublist[0], sublen[0]);
        if (!item || item->valnum <= 0)
            return -100;
     
        if (index < 0) index = item->valnum - 1;
        if (index >= item->valnum || index < 0)
            return -200;
     
        if (index == 0 && item->valnum == 1) {
            jval = (JsonValue *)item->valobj;
     
        } else if (item->valnum > 1) {
            jval = arr_value((arr_t *)item->valobj, index);
        }
     
        if (!jval) return -300;
     
        if (jval->valtype == 0) { //generic string
            break;
        } else { //object
            subobj = jval->jsonobj;
        }
    }

    if (!jval) return -400;

    if (jval->valtype == 0) { //generic string
        if (pval) *pval = jval->value;
        if (vallen) *vallen = jval->valuelen;
        if (pobj) *pobj = NULL;

    } else { //object
        if (pval) *pval = NULL;
        if (vallen) *vallen = 0;
        if (pobj) *pobj = jval->jsonobj;
    }

    return item->valnum;
}

/* http.server.location[0].errpage.504 */
int json_mget (void * vobj, void * key, int keylen, void * val, int * vallen)
{
    void      * p = NULL;
    int         len = 0;
    int         ret = 0;

    ret = json_mget_value(vobj, key, keylen, &p, &len, NULL);
    if (ret <= 0) {
        if (vallen) *vallen = 0;
        return ret;
    }

    if (vallen && *vallen > len)
        *vallen = len;

    if (p)
        memcpy(val, p, *vallen);

    return ret;
}
 
int json_mgetP (void * vobj, void * key, int keylen, void ** pval, int * vallen)
{
    void      * p = NULL;
    int         len = 0;
    int         ret = 0;
 
    if (pval) *pval = NULL;
    if (vallen) *vallen = 0;

    ret = json_mget_value(vobj, key, keylen, &p, &len, NULL);
    if (ret <= 0) {
        if (vallen) *vallen = 0;
        if (pval) *pval = NULL;
        return ret;
    }
 
    if (vallen) *vallen = len;
    if (pval) *pval = p;
 
    return ret;
}

int json_mget_obj (void * jobj, void * key, int keylen, void ** pobj)
{
    void      * tmp = NULL;
    int         ret = 0;
 
    if (pobj) *pobj = NULL;

    ret = json_mget_value(jobj, key, keylen, NULL, NULL, &tmp);
    if (ret <= 0) {
        if (pobj) *pobj = NULL;
        return ret;
    }
 
    if (pobj) *pobj = tmp;
 
    return ret;
}
 
int json_mget_int8 (void * vobj, void * key, int keylen, int8 * val)
{
    objgets(vobj, key, keylen, val, 1, 0, 1, strtol);
}
 
int json_mget_uint8 (void * vobj, void * key, int keylen, uint8 * val)
{
    objgets(vobj, key, keylen, val, 1, 0, 0, strtoul);
}
 

int json_mget_int16 (void * vobj, void * key, int keylen, int16 * val)
{    
    objgets(vobj, key, keylen, val, 1, 0, 1, strtol);
}

int json_mget_uint16 (void * vobj, void * key, int keylen, uint16 * val)
{
    objgets(vobj, key, keylen, val, 1, 0, 0, strtoul);
}
 
int json_mget_int (void * vobj, void * key, int keylen, int * val)
{
    objget(vobj, key, keylen, val, 1, 0, 1, strtol);
}

int json_mget_uint32 (void * vobj, void * key, int keylen, uint32 * val)
{
    objget(vobj, key, keylen, val, 1, 0, 0, strtoul);
}

int json_mget_long (void * vobj, void * key, int keylen, long * val)
{
    objget(vobj, key, keylen, val, 1, 0, 1, strtol);
}

int json_mget_ulong (void * vobj, void * key, int keylen, ulong * val)
{
    objget(vobj, key, keylen, val, 1, 0, 0, strtoul);
}

int json_mget_int64 (void * vobj, void * key, int keylen, int64 * val)
{
    objget(vobj, key, keylen, val, 1, 0, 1, strtoll);
}

int json_mget_uint64 (void * vobj, void * key, int keylen, uint64 * val)
{
    objget(vobj, key, keylen, val, 1, 0, 0, strtoull);
}
 
int json_mget_double (void * vobj, void * key, int keylen, double * val)
{
    objgetd(vobj, key, keylen, val, 1, 0, strtod);
}


int json_del (void * vobj, void * key, int keylen)
{
    JsonObj   * obj = (JsonObj *)vobj;
    JsonItem  * item = NULL;
 
    if (!obj) return -1;
 
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
 
    item = json_item_del(obj, key, keylen);
    if (!item) return -100;
 
    json_item_free(item);

    return 0;
}


int json_get_value (void * vobj, void * key, int keylen, int index, void ** pval, int * vallen, void ** pobj)
{
    JsonObj   * obj = (JsonObj *)vobj;
    JsonItem  * item = NULL;
    JsonValue * jval = NULL;
 
    if (pval) *pval = NULL;
    if (vallen) *vallen = 0;

    if (!obj) return -1;
 
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
 
    item = json_item_get(obj, key, keylen);
    if (!item || item->valnum <= 0)
        return -100;

    if (index < 0) index = item->valnum - 1;
    if (index >= item->valnum || index < 0)
        return -200;
 
    if (index == 0 && item->valnum == 1) {
        jval = (JsonValue *)item->valobj;

    } else if (item->valnum > 1) {
        jval = arr_value((arr_t *)item->valobj, index);
    }

    if (!jval) return -300;
 
    if (jval->valtype == 0) { //generic string
        if (pval) *pval = jval->value;
        if (vallen) *vallen = jval->valuelen;
        if (pobj) *pobj = NULL;
 
    } else { //object
        if (pval) *pval = NULL;
        if (vallen) *vallen = 0;
        if (pobj) *pobj = jval->jsonobj;
    }
 
    return item->valnum;
}
 
int json_getP (void * vobj, void * key, int keylen, int index, void * pval, int * vallen)
{
    void  ** ppval = NULL;
    void   * p = NULL;
    int      len = 0;
    int      ret = 0;
 
    if (pval) ppval = (void **)pval;

    ret = json_get_value(vobj, key, keylen, index, &p, &len, NULL);
    if (ret <= 0) {
        if (vallen) *vallen = 0;
        if (ppval) *ppval = p;
        return ret;
    }
 
    if (vallen) *vallen = len;
    if (ppval) *ppval = p;
 
    return ret;
}

int json_get (void * vobj, void * key, int keylen, int index, void * val, int vallen)
{
    void      * p = NULL;
    int         len = 0;
    int         ret = 0;
 
    ret = json_get_value(vobj, key, keylen, index, &p, &len, NULL);
    if (ret <= 0) {
        return ret;
    }

    if (p && val) {
        str_secpy(val, vallen, p, len);
    }
 
    return ret;
}

int json_get_obj (void * vobj, void * key, int keylen, int index, void ** pobj)
{
    void      * obj = NULL;
    int         ret = 0;
 
    ret = json_get_value(vobj, key, keylen, index, NULL, NULL, &obj);
    if (ret <= 0) {
        if (pobj) *pobj = NULL;
        return ret;
    }
 
    if (pobj) *pobj = obj;
 
    return ret;
}

 
int json_get_int8 (void * vobj, void * key, int keylen, int index, int8 * val)
{
    objgets(vobj, key, keylen, val, 0, index, 1, strtol);
}
 
int json_get_uint8 (void * vobj, void * key, int keylen, int index, uint8 * val)
{
    objgets(vobj, key, keylen, val, 0, index, 0, strtoul);
}
 

int json_get_int16 (void * vobj, void * key, int keylen, int index, int16 * val)
{    
    objgets(vobj, key, keylen, val, 0, index, 1, strtol);
}

int json_get_uint16 (void * vobj, void * key, int keylen, int index, uint16 * val)
{
    objgets(vobj, key, keylen, val, 0, index, 0, strtoul);
}
 
int json_get_int (void * vobj, void * key, int keylen, int index, int * val)
{
    objget(vobj, key, keylen, val, 0, index, 1, strtol);
}

int json_get_uint32 (void * vobj, void * key, int keylen, int index, uint32 * val)
{
    objget(vobj, key, keylen, val, 0, index, 0, strtoul);
}

int json_get_long (void * vobj, void * key, int keylen, int index, long * val)
{
    objget(vobj, key, keylen, val, 0, index, 1, strtol);
}

int json_get_ulong (void * vobj, void * key, int keylen, int index, ulong * val)
{
    objget(vobj, key, keylen, val, 0, index, 0, strtoul);
}

int json_get_int64 (void * vobj, void * key, int keylen, int index, int64 * val)
{
    objget(vobj, key, keylen, val, 0, index, 1, strtoll);
}

int json_get_uint64 (void * vobj, void * key, int keylen, int index, uint64 * val)
{
    objget(vobj, key, keylen, val, 0, index, 0, strtoull);
}
 
int json_get_double (void * vobj, void * key, int keylen, int index, double * val)
{
    objgetd(vobj, key, keylen, val, 0, index, strtod);
}

 
int json_add (void * vobj, void * key, int keylen, void * val, int vallen, uint8 isarr, uint8 strip)
{
    JsonObj   * obj = (JsonObj *)vobj;
    JsonItem  * item = NULL;
    JsonValue * jval = NULL;
    arr_t     * vallist = NULL;
    uint8       alloc = 0;

    if (!obj) return -1;

    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;

    if (val && vallen < 0) vallen = str_len(val);
    if (vallen < 0) vallen = 0;

    if (strip) {
        key = json_strip_dup(key, keylen);
        keylen = str_len(key);
        alloc = 1;
    }

    item = json_item_get(obj, key, keylen);
    if (!item) {
        item = json_item_alloc();
        if (alloc) {
            item->name = key;
            item->namelen = keylen;
        } else {
            item->name = str_dup(key, keylen);
            item->namelen = keylen;
        }
        json_item_add(obj, key, keylen, item);

        obj->bytenum += keylen + 3;
    }
    item->arrflag = isarr;

    jval = json_value_alloc();

    jval->value = strip ? json_strip_dup(val, vallen) : str_dup(val, vallen);
    jval->valuelen = jval->value ? str_len(jval->value) : 0;

    obj->bytenum += vallen + 3;

    if (item->arrflag) {
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

    } else {
        if (item->valnum == 1) {
            json_value_free(item->valobj); 

        } else if (item->valnum > 1) { 
            arr_pop_free((arr_t *)item->valobj, json_value_free); 
        }

        item->valobj = jval; 
        item->valnum = 1;
    }

    return item->valnum;
}


int json_append (void * vobj, void * key, int keylen, void * val, int vallen, uint8 strip)
{
    JsonObj   * obj = (JsonObj *)vobj;
    JsonItem  * item = NULL;
    JsonValue * jval = NULL;
    arr_t     * vallist = NULL;
    int         i, num = 0;
    int         bytenum = 0;
    uint8       alloc = 0;
 
    if (!obj) return -1;
 
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
 
    if (val && vallen < 0) vallen = str_len(val);
    if (vallen <= 0) return 0;
 
    if (strip) {
        key = json_strip_dup(key, keylen);
        keylen = str_len(key);
        alloc = 1;
    }

    item = json_item_get(obj, key, keylen);
    if (!item) {
        item = json_item_alloc();

        if (alloc) {
            item->name = key;
            item->namelen = keylen;
        } else {
            item->name = str_dup(key, keylen);
            item->namelen = keylen;
        }

        json_item_add(obj, key, keylen, item);
 
        obj->bytenum += keylen + 3;
    }
 
    if (item->valnum < 1) {
        jval = json_value_alloc();

        jval->value = strip ? json_strip_dup(val, vallen) : str_dup(val, vallen);
        jval->valuelen = jval->value ? str_len(jval->value) : 0;

        obj->bytenum += vallen + 3;

        item->valobj = jval;
        item->valnum = 1;

    } else if (item->valnum == 1) {
        jval = item->valobj;
        if (!jval) jval = item->valobj = json_value_alloc();

        obj->bytenum += vallen + 3;
        jval->value = krealloc(jval->value, jval->valuelen + vallen + 1);

        if (jval->value) {
            if (strip) {
                bytenum = json_strip(val, vallen, jval->value + jval->valuelen, vallen);
            } else {
                bytenum = vallen;
                memcpy(jval->value + jval->valuelen, val, vallen);
            }

            jval->valuelen += bytenum;
        }

    } else {
        vallist = (arr_t *)item->valobj;

        num = arr_num(vallist);
        for (i = 0; i < num; i++) {
            jval = arr_value(vallist, i);
            if (!jval) continue;

            obj->bytenum += vallen + 3;
            jval->value = krealloc(jval->value, jval->valuelen + vallen + 1);

            if (jval->value) {
                if (strip) {
                    bytenum = json_strip(val, vallen, jval->value + jval->valuelen, vallen);
                } else {
                    bytenum = vallen;
                    memcpy(jval->value + jval->valuelen, val, vallen);
                }
                jval->valuelen += bytenum;
            }
        }
    }
 
    return item->valnum;
}

int json_append_file (void * vobj, void * key, int keylen, char * fname, long startpos, long length)
{
    JsonObj   * obj = (JsonObj *)vobj;
    JsonItem  * item = NULL;
    JsonValue * jval = NULL;
    arr_t     * vallist = NULL;
    long        fsize = 0;
    FILE      * fp = NULL;
    int         i, num = 0, tmplen = 0;
 
    if (!obj) return -1;
 
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
 
    if ((fsize = file_size(fname)) <= 0)
        return -100;

    if (startpos < 0) startpos = 0;
    if (startpos >= fsize) return -101;

    if (length < 0 || length > fsize - startpos)
        length = fsize - startpos;
    if (length <= 0) return -102;

    fp = fopen(fname, "rb");
    if (!fp) return -200;

    item = json_item_get(obj, key, keylen);
    if (!item) {
        item = json_item_alloc();
        item->name = str_dup(key, keylen);
        item->namelen = keylen;
        json_item_add(obj, key, keylen, item);
 
        obj->bytenum += keylen+3;
    }
 
    if (item->valnum < 1) {
        jval = json_value_alloc();

        jval->value = kalloc(length+1);

        file_read(fp, jval->value, length);

        /* strip the escaped char */
        jval->valuelen = json_strip(jval->value, length, jval->value, length);
        jval->value[jval->valuelen] = '\0';

        obj->bytenum += length+3;
 
        item->valobj = jval;
        item->valnum = 1;

    } else if (item->valnum == 1) {
        jval = item->valobj;
        if (!jval) jval = item->valobj = json_value_alloc();
 
        obj->bytenum += length+3;
        jval->value = krealloc(jval->value, jval->valuelen + length + 1);
 
        if (jval->value) {
            file_read(fp, jval->value + jval->valuelen, length);

            /* strip the escaped char */
            tmplen = json_strip(jval->value + jval->valuelen, length,
                                   jval->value + jval->valuelen, length);
            jval->valuelen += tmplen;

            jval->value[jval->valuelen] = '\0';
        }

    } else {
        vallist = (arr_t *)item->valobj;

        num = arr_num(vallist);
        for (i=0; i<num; i++) {
            jval = arr_value(vallist, i);
            if (!jval) continue;
 
            obj->bytenum += length+3;
            jval->value = krealloc(jval->value, jval->valuelen + length + 1);
 
            if (jval->value) {
                file_read(fp, jval->value + jval->valuelen, length);

                /* strip the escaped char */
                tmplen = json_strip(jval->value + jval->valuelen, length,
                                       jval->value + jval->valuelen, length);
                jval->valuelen += tmplen;

                jval->value[jval->valuelen] = '\0';
            }
        }
    }
 
    fclose(fp);
    return item->valnum;
}


int json_add_by_fca (void * vobj, void * fca, long keypos, int keylen, long valpos, int vallen, uint8 isarr)
{ 
    JsonObj   * obj = (JsonObj *)vobj;
    JsonItem  * item = NULL; 
    JsonValue * jval = NULL;
    arr_t     * vallist = NULL;
    uint8     * key = NULL;
     
    if (!obj) return -1;
    if (!fca) return -2;
 
    if (keylen <= 0) return -3;
    if (vallen <= 0) return -100;
 
    key = kzalloc(keylen + 1);

    file_cache_seek(fca, keypos);
    file_cache_read(fca, key, keylen, 0);

    item = json_item_get(obj, key, keylen);
    if (!item) {
        item = json_item_alloc();

        item->name = key;
        item->namelen = keylen;

        json_item_add(obj, key, keylen, item);

    } else {
        kfree(key);
    }

    item->arrflag = isarr;
 
    jval = json_value_alloc();

    jval->value = kzalloc(vallen+1);
    jval->valuelen = vallen;
 
    file_cache_seek(fca, valpos);
    file_cache_read(fca, jval->value, vallen, 0);

    jval->valuelen = json_strip(jval->value, vallen, jval->value, vallen);
    jval->value[jval->valuelen] = '\0';

    if (item->arrflag) {
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

    } else {
        if (item->valnum == 1) {
            json_value_free(item->valobj);
        } else if (item->valnum > 1) {
            arr_pop_free((arr_t *)item->valobj, json_value_free);
        }

        item->valobj = jval;
        item->valnum = 1;
    }
 
    return item->valnum;
}
 
 
void * json_add_obj (void * vobj, void * key, int keylen, uint8 isarr)
{
    JsonObj   * obj = (JsonObj *)vobj;
    JsonItem  * item = NULL;
    JsonValue * jval = NULL;
    arr_t     * vallist = NULL;

    if (!obj) return NULL;

    if (!key) return NULL;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return NULL;

    item = json_item_get(obj, key, keylen);
    if (!item) {
        item = json_item_alloc();
        item->name = str_dup(key, keylen);
        item->namelen = keylen;

        item->valtype = 1;
        json_item_add(obj, key, keylen, item);
    }
    item->arrflag = isarr;

    jval = json_value_alloc();
    jval->valtype = 1;
    jval->jsonobj = json_init(obj->sptype, obj->cmtflag, obj->sibcoex);

    if (item->arrflag) {
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
    } else {
        if (item->valnum == 1) {
            json_value_free(item->valobj); 
        } else if (item->valnum > 1) { 
            arr_pop_free((arr_t *)item->valobj, json_value_free); 
        }
        item->valobj = jval; 
        item->valnum = 1;
    }

    return jval->jsonobj;
}

void * json_add_obj_by_fca (void * vobj, void * fca, long keypos, int keylen, uint8 isarr)
{    
    JsonObj   * obj = (JsonObj *)vobj;
    JsonItem  * item = NULL; 
    JsonValue * jval = NULL;
    arr_t     * vallist = NULL;
    uint8     * key = NULL;
         
    if (!obj) return NULL;
    if (!fca) return NULL;
         
    if (keylen <= 0) return NULL;
     
    key = kzalloc(keylen + 1);
    file_cache_seek(fca, keypos);
    file_cache_read(fca, key, keylen, 0);

    item = json_item_get(obj, key, keylen);
    if (!item) {
        item = json_item_alloc();
        item->name = key;
        item->namelen = keylen;
     
        item->valtype = 1;
        json_item_add(obj, key, keylen, item);
    } else {
        kfree(key);
    }
    item->arrflag = isarr;
 
    jval = json_value_alloc();
    jval->valtype = 1;
    jval->jsonobj = json_init(obj->sptype, obj->cmtflag, obj->sibcoex);
 
    if (item->arrflag) {
        if (item->valnum == 0) item->valobj = jval;
        else if (item->valnum == 1) {
            vallist = arr_new(4);
            arr_push(vallist, item->valobj);
            arr_push(vallist, jval);
            item->valobj = vallist;
        } else {
            arr_push((arr_t *)item->valobj, jval);
        }
        item->valnum++;
    } else {
        if (item->valnum == 1) json_value_free(item->valobj);
        else if (item->valnum > 1) {
            arr_pop_free((arr_t *)item->valobj, json_value_free);
        }
        item->valobj = jval;
        item->valnum = 1;
    }
 
    return jval->jsonobj;
}

 
int json_add_int8 (void * vobj, void * key, int keylen, int8 val, uint8 isarr)
{
    JsonObj  * obj = (JsonObj *)vobj;
    char       buf[32];
 
    if (!obj) return -1;
 
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
 
    sprintf(buf, "%d", val);
    return json_add(obj, key, keylen, buf, -1, isarr, 0);
}
 
int json_add_uint8 (void * vobj, void * key, int keylen, uint8 val, uint8 isarr)
{
    JsonObj  * obj = (JsonObj *)vobj;
    char       buf[64];
 
    if (!obj) return -1;
 
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
 
    sprintf(buf, "%u", val);
    return json_add(obj, key, keylen, buf, -1, isarr, 0);
}
 

int json_add_int16 (void * vobj, void * key, int keylen, int16 val, uint8 isarr)
{
    JsonObj  * obj = (JsonObj *)vobj;
    char       buf[32]; 
     
    if (!obj) return -1;
     
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
     
    sprintf(buf, "%d", val);
    return json_add(obj, key, keylen, buf, -1, isarr, 0);
}    

int json_add_uint16 (void * vobj, void * key, int keylen, uint16 val, uint8 isarr)
{        
    JsonObj  * obj = (JsonObj *)vobj;
    char       buf[64];
 
    if (!obj) return -1;
 
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
 
    sprintf(buf, "%u", val);
    return json_add(obj, key, keylen, buf, -1, isarr, 0);
}
 

int json_add_int (void * vobj, void * key, int keylen, int val, uint8 isarr)
{
    JsonObj  * obj = (JsonObj *)vobj;
    char       buf[32];

    if (!obj) return -1;

    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;

    sprintf(buf, "%d", val);
    return json_add(obj, key, keylen, buf, -1, isarr, 0);
}

int json_add_uint32 (void * vobj, void * key, int keylen, uint32 val, uint8 isarr)
{
    JsonObj  * obj = (JsonObj *)vobj;
    char       buf[64];  
 
    if (!obj) return -1; 
 
    if (!key) return -2; 
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3; 
 
    sprintf(buf, "%u", val); 
    return json_add(obj, key, keylen, buf, -1, isarr, 0);
}

int json_add_long (void * vobj, void * key, int keylen, long val, uint8 isarr)
{
    JsonObj  * obj = (JsonObj *)vobj;
    char       buf[64];  
 
    if (!obj) return -1; 
 
    if (!key) return -2; 
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3; 
 
    sprintf(buf, "%ld", val); 
    return json_add(obj, key, keylen, buf, -1, isarr, 0);
}

int json_add_ulong (void * vobj, void * key, int keylen, ulong val, uint8 isarr)
{
    JsonObj  * obj = (JsonObj *)vobj;
    char       buf[64];
 
    if (!obj) return -1;
 
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
 
    sprintf(buf, "%lu", val);
    return json_add(obj, key, keylen, buf, -1, isarr, 0);
}

int json_add_int64 (void * vobj, void * key, int keylen, int64 val, uint8 isarr)
{
    JsonObj  * obj = (JsonObj *)vobj;
    char       buf[64];
 
    if (!obj) return -1;
 
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
 
#if defined(_WIN32) || defined(_WIN64)
    sprintf(buf, "%I64d", val);
#else
    sprintf(buf, "%lld", val);
#endif
    return json_add(obj, key, keylen, buf, -1, isarr, 0);
}

int json_add_uint64 (void * vobj, void * key, int keylen, uint64 val, uint8 isarr)
{
    JsonObj  * obj = (JsonObj *)vobj;
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
    return json_add(obj, key, keylen, buf, -1, isarr, 0);
}

int json_add_double (void * vobj, void * key, int keylen, double val, uint8 isarr)
{
    JsonObj  * obj = (JsonObj *)vobj;
    char       buf[64];
 
    if (!obj) return -1;
 
    if (!key) return -2;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return -3;
 
    sprintf(buf, "%f", val);
    return json_add(obj, key, keylen, buf, -1, isarr, 0);
}

int reverse_find_brace_peer (uint8 * pbgn, uint8 * pbra)
{
    int  revbra = 0;
    int  bra = 0;

    if (pbra < pbgn) return 0;

    for ( ; pbra >= pbgn; pbra--) {
        if (*pbra == '}') revbra++;
        else if (*pbra == '{') bra++;
    }

    return bra - revbra;
}

uint8 * seek_closed_peer (uint8 * pbgn, int len, int leftch, int rightch)
{
    uint8 * pend = NULL;
    int     bra = 0;
    int     revbra = 0;
 
    if (len <= 0) return pbgn;

    pend = pbgn + len;
 
    if (*pbgn != leftch) return pbgn;

    for ( ; pbgn < pend; pbgn++) {
        if (*pbgn == rightch) {
            revbra++;
            if (revbra == bra)
                return pbgn;

        } else if (*pbgn == leftch) {
            bra++;
        }
    }
 
    return pbgn;
}

uint8 * find_closed_tag (uint8 * pbgn, uint8 * pend, char * tagname)
{
    uint8  * pkvend = NULL;
    char     headtag[64];
    char     tailtag[64];
    int      taglen = 0;

    if (!tagname || (taglen = str_len(tagname)) <= 0)
        return NULL;

    if (!pbgn || !pend) return NULL;

    if (pend < pbgn + 2 * taglen + 5) return NULL;

    snprintf(headtag, sizeof(headtag)-1, "<%s>", tagname);
    snprintf(tailtag, sizeof(tailtag)-1, "</%s>", tagname);

    /* "cache_file" = <script> if()... else {..}  return ... </script>; */

    if (str_ncasecmp(pbgn, headtag, taglen + 2) != 0)
        return NULL;

    pkvend = sun_find_string(pbgn+8, pend-pbgn-8, tailtag, taglen + 3, NULL);
    if (pkvend == NULL)
        return NULL;

    return pkvend + taglen + 3;
}

int json_decode (void * vobj, void * vjson, int length, int findobjbgn, int strip)
{
    JsonObj  * obj = (JsonObj *)vobj;
    uint8    * pjson = (uint8 *)vjson;
    JsonObj  * subobj = NULL;
    uint8    * pbgn = NULL;
    uint8    * pend = NULL;

    uint8    * pkvsep = NULL;
    uint8    * pkvend = NULL;
    uint8    * poct = NULL;

    uint8    * name = NULL;
    int        namelen = 0;
    uint8    * value = NULL;
    int        valuelen = 0;

    if (!obj) return 0;
    if (!pjson) return 0;
    if (length < 0) length = str_len(pjson);
    if (length <= 0) return 0;

    pbgn = pjson; pend = pbgn + length;

    if (findobjbgn) {
        pbgn = skipQuoteTo(pbgn, pend-pbgn, "{", 1);
        if (pbgn >= pend) return pbgn - pjson;
        pbgn += 1;
    }

    do {
        pbgn = skipOver(pbgn, pend-pbgn, obj->kvsp, obj->kvsplen);
        if (pbgn >= pend) return pbgn - pjson;
        if (*pbgn == '}') return pbgn - pjson + 1;

        if (obj->cmtflag) {
            if (*pbgn == '#') { //comment to end of the line
                pbgn++;
    
                //find the \r\n
                poct = skipTo(pbgn, pend-pbgn, "\r\n", 2);
                if (obj->cmtflag > 1)
                    json_add(obj, "cmt#", 4, pbgn, poct - pbgn, 1, 0);
    
                pbgn = poct;
                continue;
    
            } else if (pbgn[0] == '/' && pbgn[1] == '*') {
                pbgn += 2;
    
                //find the comment end * and  /
                for (poct = pbgn; poct < pend; ) {
                    poct = skipTo(poct, pend-poct, "*", 1);
                    if (poct < pend - 1 && poct[1] != '/') {
                        poct++;
                        continue;
    
                    } else break;
                }
    
                if (poct >= pend - 1) {
                    if (obj->cmtflag > 1)
                        json_add(obj, "cmt*", 4, pbgn, poct - pbgn, 1, 0);
                    pbgn = poct;
    
                } else if (poct[0] == '*' && poct[1] == '/') {
                    if (obj->cmtflag > 1)
                        json_add(obj, "cmt*", 4, pbgn, poct - pbgn, 1, 0);
                    pbgn = poct + 2;
                }
                continue;
            }
        } //end of if (obj->cmtflag)

        if (*pbgn == '<' && (pkvend = find_closed_tag(pbgn, pend, "script")) != NULL) { //<script>
            /* <script>
                 if ($query[fid]) return $query[fid]$req_file_ext;
               </script> */

            name = (uint8 *)"script";
            namelen = 6;

            value = skipOver(pbgn+8, pkvend-pbgn-8, " \t\r\n\f\v", 6);
            poct = rskipOver(pkvend-10, pkvend-9-value, " \t\t\n\f\v", 6);
            valuelen = poct + 1 - value;

            if (valuelen > 0)
                json_add(obj, name, namelen, value, valuelen, 1, strip);

            pbgn = pkvend;
            continue; 
        }

        if (*pbgn == '<' && (pkvend = find_closed_tag(pbgn, pend, "reply_script")) != NULL) { //<reply_script>
            /* <reply_script>
                 if ($query[fid]) return $query[fid]$req_file_ext;
               </reply_script> */
 
            name = (uint8 *)"reply_script";
            namelen = 12;
 
            value = skipOver(pbgn+namelen+2, pkvend-pbgn-namelen-2, " \t\r\n\f\v", 6);
            poct = rskipOver(pkvend-namelen-4, pkvend-namelen-3-value, " \t\t\n\f\v", 6);
            valuelen = poct + 1 - value;
 
            if (valuelen > 0)
                json_add(obj, name, namelen, value, valuelen, 1, strip);
 
            pbgn = pkvend;
            continue;
        }

        /* search the separator between key and value */
        pkvsep = skipQuoteTo(pbgn, pend-pbgn, obj->keyend, obj->keyendlen);

        if (pkvsep >= pend) return pkvsep - pjson;
        if (*pkvsep == '}') return pkvsep - pjson + 1;

        if (*pkvsep == obj->kvsep) {
            if (*pbgn == '"' || *pbgn == '\'') {
                name = pbgn + 1;
                poct = skipEscTo(pbgn+1, pkvsep-pbgn-1, pbgn, 1);
                poct--;

            } else {
                name = pbgn;
                poct = rskipOver(pkvsep-1, pkvsep-pbgn, obj->sp, obj->splen);
            }

            namelen = poct - name + 1;

            poct = skipOver(pkvsep+1, pend-pkvsep-1, obj->sp, obj->splen);
            if (*poct == '}') return poct - pjson + 1;

            if (*poct == '{') { //sub-obj

                /* script = { if (...) ... else ... } 
                   here we name 'script' as a constant defining the script codes */
                if (namelen == 6 && str_ncasecmp(name, "script", 6) == 0) {
                    //pbgn = seek_closed_peer(poct, pend-poct, '{', '}');
                    pbgn = skipToPeer(poct, pend-poct, '{', '}');

                    value = skipOver(poct+1, pbgn-poct-1, " \t\r\n\f\v", 6);
                    poct = rskipOver(pbgn-1, pbgn-value, " \t\r\n\f\v", 6);
                    valuelen = poct + 1 - value;

                    if (valuelen > 0)
                        json_add(obj, name, namelen, value, valuelen, 1, strip);

                    if (pbgn < pend && *pbgn == '}') {
                        pbgn++;
                    }

                    continue;
                } 

                /* reply_script = { if (...) ... else ... }
                   here we name 'reply_script' as a constant defining the script codes */
                if (namelen == 12 && str_ncasecmp(name, "reply_script", 12) == 0) {
                    pbgn = skipToPeer(poct, pend-poct, '{', '}');
 
                    value = skipOver(poct+1, pbgn-poct-1, " \t\r\n\f\v", 6);
                    poct = rskipOver(pbgn-1, pbgn-value, " \t\r\n\f\v", 6);
                    valuelen = poct + 1 - value;
 
                    if (valuelen > 0)
                        json_add(obj, name, namelen, value, valuelen, 1, strip);
 
                    if (pbgn < pend && *pbgn == '}') {
                        pbgn++;
                    }
 
                    continue;
                }

                if (obj->sibcoex)
                    subobj = json_add_obj(obj, name, namelen, 2);
                else
                    subobj = json_add_obj(obj, name, namelen, 0);

                pbgn = poct + json_decode(subobj, poct, pend-poct, 1, strip);

            } else if (*poct == '[') { //array
                poct++;

                do {
                    pbgn = skipOver(poct, pend-poct, obj->arrsp, obj->arrsplen);
                    if (pbgn >= pend) return pbgn - pjson;
                    if (*pbgn == '}') return pbgn - pjson + 1;

                    if (*pbgn == ']') { pbgn++; break; }

                    if (*pbgn == '{') {
                        subobj = json_add_obj(obj, name, namelen, 1);
                        poct = pbgn + json_decode(subobj, pbgn, pend-pbgn, 1, strip);

                    } else {
                        pkvend = skipQuoteTo(pbgn, pend-pbgn, obj->arrend, obj->arrendlen);
                        if (*pbgn == '"' || *pbgn == '\'') {
                            value = pbgn + 1;
                            poct = skipEscTo(pbgn+1, pkvend-pbgn-1, pbgn, 1);
                            poct--;

                        } else {
                            value = pbgn;
                            poct = rskipOver(pkvend-1, pkvend-pbgn, obj->sp, obj->splen);
                        }

                        valuelen = poct - value + 1;
                        json_add(obj, name, namelen, value, valuelen, 1, strip);

                        poct = pkvend;
                    }
                } while (pbgn < pend);

            } else if (*poct == '<' && (pkvend = find_closed_tag(poct, pend, "script")) != NULL) { //<script>
                /* "cache_file" = <script> if ($query[fid]) return $query[fid]$req_file_ext; </script> */

                value = poct;
                valuelen = pkvend - value;
                json_add(obj, name, namelen, value, valuelen, 1, strip);

                pbgn = pkvend;

            } else { //generic string value
                pkvend = skipQuoteTo(poct, pend-poct, obj->kvend, obj->kvendlen);

                /* eg. "cache_file" = "${root}${req_path}${base_name}.ext"; */
                while (pkvend < pend && *pkvend == '}') {
                    if (reverse_find_brace_peer(poct, pkvend) >= 0)
                        pkvend = skipQuoteTo(pkvend+1, pend-pkvend-1, obj->kvend, obj->kvendlen);
                    else
                        break;
                }

                if (*poct == '"' || *poct == '\'') {
                    value = poct + 1;
                    poct = skipEscTo(poct+1, pkvend-poct-1, poct, 1);
                    poct--;

                } else {
                    value = poct;
                    poct = rskipOver(pkvend-1, pkvend-poct, obj->sp, obj->splen);
                }

                valuelen = poct - value + 1;

                if (obj->sibcoex)
                    json_add(obj, name, namelen, value, valuelen, 2, strip);
                else
                    json_add(obj, name, namelen, value, valuelen, 0, strip);

                pbgn = pkvend;
            }

        } else if (*pkvsep == obj->itemsep) {
            /* key has no corresponding value followed */
            if (*pbgn == '"' || *pbgn == '\'') {
                name = pbgn + 1;
                poct = skipEscTo(pbgn+1, pkvsep-pbgn-1, pbgn, 1);
                poct--;
 
            } else {
                name = pbgn;
                poct = rskipOver(pkvsep-1, pkvsep-pbgn, obj->sp, obj->splen);
            }
 
            namelen = poct - name + 1;

            /* include cnf/mime.types; will load and parse the conf file */

            if (str_ncasecmp(name, "include", 7) == 0 && ISSPACE(name[7])) {
                pbgn = skipOver(name + 7, poct + 1 - name - 7, obj->sp, obj->splen);
                if (pbgn > poct) continue;

                json_decode_file(obj, pbgn,  poct + 1 - pbgn, 0, strip);

            } else {
                if (obj->sibcoex)
                    json_add(obj, name, namelen, NULL, 0, 2, strip);
                else
                    json_add(obj, name, namelen, NULL, 0, 0, strip);
            }
            
            pbgn = pkvsep + 1;
        }

    } while (pbgn < pend);

    return pbgn-pjson;
}
 
int json_decode_file (void * vobj, void * fn, int fnlen, int findobjbgn, int strip)
{
#ifdef UNIX
    JsonObj     * obj = (JsonObj *)vobj;
    char          fname[512];
    struct stat   st;
    int           fd;
    void        * pbyte = NULL;
    int           ret = 0;

    if (!obj) return -1;
    if (!fn || fnlen <= 0) return -2;

    str_secpy(fname, sizeof(fname)-1, fn, fnlen);
    if (file_stat(fname, &st) < 0)
        return -100;

    fd = open(fname, O_RDONLY);
    if (fd < 0) {
        return -200;
    }
 
    pbyte = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (!pbyte) {
        close(fd);
        return -300;
    }
 
    ret = json_decode(obj, pbyte, st.st_size, findobjbgn, strip);
 
    munmap(pbyte, st.st_size);
    close(fd);

    return ret;
#endif

#if defined(_WIN32) || defined(_WIN64)
    JsonObj     * obj = (JsonObj *)vobj;
    char          fname[512];
    void        * hfile = NULL;
    HANDLE        hmap;
    void        * pmap;
    void        * pbyte = NULL;
    int64         maplen = 0;
    int64         mapoff = 0;
    int           ret = 0;

    if (!obj) return -1;
    if (!fn || fnlen <= 0) return -2;

    str_secpy(fname, sizeof(fname)-1, fn, fnlen);

    hfile = native_file_open(fname, NF_READ);
    if (!hfile) {
        return -200;
    }
 
    pbyte = file_mmap(NULL, native_file_handle(hfile), 0,
                      native_file_size(hfile), NULL,
                      &hmap, &pmap, &maplen, &mapoff);
    if (!pbyte) {
        native_file_close(hfile);
        return -300;
    }
 
    ret = json_decode(obj, pbyte, native_file_size(hfile), findobjbgn, strip);
 
    file_munmap(hmap, pmap);
    native_file_close(hfile);

    return ret;
#endif
}

#if 0
int json_decode_chunk (void * vobj, void * chunk, int64 pos, int64 length, int findobjbgn, int strip)
{
    JsonObj  * obj = (JsonObj *)vobj;
    JsonObj  * subobj = NULL;

    int64      bgnpos = 0;
    int64      endpos = 0;
    int64      kvseqpos = 0;
    int64      kvendpos = 0;
    int64      octpos = 0;

    uint8    * name = NULL;
    int        namelen = 0;
    uint8    * value = NULL;
    int        valuelen = 0;
 
    if (!obj) return 0;
    if (!chunk) return 0;
    if (pos < chunk_startpos(chunk, 0)) pos = chunk_startpos(chunk, 0);
    if (length < 0) length = chunk_size(chunk, 0) - pos;
    if (length <= 0) return 0;
 
    bgnpos = pos; endpos = bgnpos + length;
 
    if (findobjbgn) {
        bgnpos = chunk_skip_quote_to(chunk, bgnpos, endpos-bgnpos, "{", 1);
        if (bgnpos >= endpos) return bgnpos - pos;
        pbgn += 1;
    }
 
    do {
        bgnpos = chunk_skip_over(chunk, bgnpos, endpos-bgnpos, obj->kvsp, obj->kvsplen);
        if (bgnpos >= endpos) return bgnpos - pos;
        if (chunk_at(chunk, bgnpos, NULL) == '}') return bgnpos - pos + 1;
 
        if (obj->cmtflag) {
            if (chunk_at(chunk, bgnpos, NULL) == '#') { //comment to end of the line
                bgnpos++;
 
                //find the \r\n
                octpos = chunk_skip_to(chunk, bgnpos, endpos-bgnpos, "\r\n", 2);
                if (obj->cmtflag > 1)
                    json_add(obj, "cmt#", 4, pbgn, poct - pbgn, 1, 0);
 
                pbgn = poct;
                continue;
 
            } else if (pbgn[0] == '/' && pbgn[1] == '*') {
                pbgn += 2;
 
                //find the comment end * and  /
                for (poct = pbgn; poct < pend; ) {
                    poct = skipTo(poct, pend-poct, "*", 1);
                    if (poct < pend - 1 && poct[1] != '/') {
                        poct++;
                        continue;
 
                    } else break;
                }
 
                if (poct >= pend - 1) {
                    if (obj->cmtflag > 1)
                        json_add(obj, "cmt*", 4, pbgn, poct - pbgn, 1, 0);
                    pbgn = poct;
 
                } else if (poct[0] == '*' && poct[1] == '/') {
                    if (obj->cmtflag > 1)
                        json_add(obj, "cmt*", 4, pbgn, poct - pbgn, 1, 0);
                    pbgn = poct + 2;
                }
                continue;
            }
        } //end of if (obj->cmtflag)
 
        if (*pbgn == '<' && (pkvend = find_closed_tag(pbgn, pend, "script")) != NULL) { //<script>
            /* <script>
                 if ($query[fid]) return $query[fid]$req_file_ext;
               </script> */
 
            name = (uint8 *)"script";
            namelen = 6;
 
            value = skipOver(pbgn+8, pkvend-pbgn-8, " \t\r\n\f\v", 6);
            poct = rskipOver(pkvend-10, pkvend-9-value, " \t\t\n\f\v", 6);
            valuelen = poct + 1 - value;
 
            if (valuelen > 0)
                json_add(obj, name, namelen, value, valuelen, 1, strip);
 
            pbgn = pkvend;
            continue;
        }
 
        /* search the separator between key and value */
        pkvsep = skipQuoteTo(pbgn, pend-pbgn, obj->keyend, obj->keyendlen);
 
        if (pkvsep >= pend) return pkvsep - pjson;
        if (*pkvsep == '}') return pkvsep - pjson + 1;
 
        if (*pkvsep == obj->kvsep) {
            if (*pbgn == '"' || *pbgn == '\'') {
                name = pbgn + 1;
                poct = skipEscTo(pbgn+1, pkvsep-pbgn-1, pbgn, 1);
                poct--;
 
            } else {
                name = pbgn;
                poct = rskipOver(pkvsep-1, pkvsep-pbgn, obj->sp, obj->splen);
            }
 
            namelen = poct - name + 1;
 
            poct = skipOver(pkvsep+1, pend-pkvsep-1, obj->sp, obj->splen);
            if (*poct == '}') return poct - pjson + 1;
 
            if (*poct == '{') { //sub-obj
 
                /* script = { if (...) ... else ... }
                   here we name 'script' as a constant defining the script codes */
                if (namelen == 6 && str_ncasecmp(name, "script", 6) == 0) {
                    //pbgn = seek_closed_peer(poct, pend-poct, '{', '}');
                    pbgn = skipToPeer(poct, pend-poct, '{', '}');
 
                    value = skipOver(poct+1, pbgn-poct-1, " \t\r\n\f\v", 6);
                    poct = rskipOver(pbgn-1, pbgn-value, " \t\r\n\f\v", 6);
                    valuelen = poct + 1 - value;
 
                    if (valuelen > 0)
                        json_add(obj, name, namelen, value, valuelen, 1, strip);
 
                    if (pbgn < pend && *pbgn == '}') {
                        pbgn++;
                    }
 
                    continue;
                }
 
                subobj = json_add_obj(obj, name, namelen, 1);
                pbgn = poct + json_decode(subobj, poct, pend-poct, 1, strip);
 
            } else if (*poct == '[') { //array
                poct++;
 
                do {
                    pbgn = skipOver(poct, pend-poct, obj->arrsp, obj->arrsplen);
                    if (pbgn >= pend) return pbgn - pjson;
                    if (*pbgn == '}') return pbgn - pjson + 1;
 
                    if (*pbgn == ']') { pbgn++; break; }
 
                    if (*pbgn == '{') {
                        subobj = json_add_obj(obj, name, namelen, 1);
                        poct = pbgn + json_decode(subobj, pbgn, pend-pbgn, 1, strip);
 
                    } else {
                        pkvend = skipQuoteTo(pbgn, pend-pbgn, obj->arrend, obj->arrendlen);
                        if (*pbgn == '"' || *pbgn == '\'') {
                            value = pbgn + 1;
                            poct = skipEscTo(pbgn+1, pkvend-pbgn-1, pbgn, 1);
                            poct--;
 
                        } else {
                            value = pbgn;
                            poct = rskipOver(pkvend-1, pkvend-pbgn, obj->sp, obj->splen);
                        }
 
                        valuelen = poct - value + 1;
                        json_add(obj, name, namelen, value, valuelen, 1, strip);
 
                        poct = pkvend;
                    }
                } while (pbgn < pend);
 
            } else if (*poct == '<' && (pkvend = find_closed_tag(poct, pend, "script")) != NULL) { //<script>
                /* "cache_file" = <script> if ($query[fid]) return $query[fid]$req_file_ext; </script> */
 
                value = poct;
                valuelen = pkvend - value;
                json_add(obj, name, namelen, value, valuelen, 1, strip);
 
                pbgn = pkvend;
 
            } else { //generic string value
                pkvend = skipQuoteTo(poct, pend-poct, obj->kvend, obj->kvendlen);
 
                /* eg. "cache_file" = "${root}${req_path}${base_name}.ext"; */
                while (pkvend < pend && *pkvend == '}') {
                    if (reverse_find_brace_peer(poct, pkvend) >= 0)
                        pkvend = skipQuoteTo(pkvend+1, pend-pkvend-1, obj->kvend, obj->kvendlen);
                    else
                        break;
                }
 
                if (*poct == '"' || *poct == '\'') {
                    value = poct + 1;
                    poct = skipEscTo(poct+1, pkvend-poct-1, poct, 1);
                    poct--;
 
                } else {
                    value = poct;
                    poct = rskipOver(pkvend-1, pkvend-poct, obj->sp, obj->splen);
                }
 
                valuelen = poct - value + 1;
                json_add(obj, name, namelen, value, valuelen, 1, strip);
 
                pbgn = pkvend;
            }
 
        } else if (*pkvsep == obj->itemsep) {
            /* key has no corresponding value followed */
            if (*pbgn == '"' || *pbgn == '\'') {
                name = pbgn + 1;
                poct = skipEscTo(pbgn+1, pkvsep-pbgn-1, pbgn, 1);
                poct--;
 
            } else {
                name = pbgn;
                poct = rskipOver(pkvsep-1, pkvsep-pbgn, obj->sp, obj->splen);
            }
 
            namelen = poct - name + 1;
 
            /* include cnf/mime.types; will load and parse the conf file */
 
            if (str_ncasecmp(name, "include", 7) == 0 && ISSPACE(name[7])) {
                pbgn = skipOver(name + 7, poct + 1 - name - 7, obj->sp, obj->splen);
                if (pbgn > poct) continue;
 
                json_decode_file(obj, pbgn,  poct + 1 - pbgn, 0, strip);
 
            } else {
                json_add(obj, name, namelen, NULL, 0, 1, strip);
            }
 
            pbgn = pkvsep + 1;
        }
 
    } while (pbgn < pend);
 
    return pbgn-pjson;
}
#endif

long json_fca_decode (void * vobj, void * fca, long startpos, long length)
{ 
    JsonObj  * obj = (JsonObj *)vobj;
    JsonObj  * subobj = NULL;  
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

    iter = startpos;
    endpos = iter + length;
    if (endpos > fsize) endpos = fsize;
    if (endpos <= startpos) return 0;

    iter = file_cache_skip_to(fca, iter, endpos-iter, "{", 1);
    if (iter >= endpos) return iter - startpos;

    iter += 1;
    while (iter < endpos) {
        iter = file_cache_skip_over(fca, iter, endpos-iter, ", \t\r\n", 5);
        if (iter >= endpos) return iter - startpos;
        if (file_cache_at(fca, iter) == '}') return iter - startpos + 1;
 
        colon = file_cache_skip_quote_to(fca, iter, endpos-iter, ":}", 2);
        if (colon >= endpos) return colon - startpos;
        if ((fch = file_cache_at(fca, colon)) == '}') return colon - startpos + 1;
        if (fch == ':') {
            if ((fch = file_cache_at(fca, iter)) == '"' || fch == '\'') {
                namepos = iter + 1;
                pat[0] = fch;
                pos = file_cache_skip_esc_to(fca, iter+1, colon-iter-1, pat, 1);
                pos--;
            } else {
                namepos = iter;
                pos = file_cache_rskip_over(fca, colon-1, colon-iter, " \t\r\n", 4);
            }
            namelen = pos - namepos + 1;

            pos = file_cache_skip_over(fca, colon+1, endpos-colon-1, " \t\r\n", 4);
            if ((fch = file_cache_at(fca, pos)) == '}') return pos - startpos + 1;

            if (fch == '{') { //sub-obj
                subobj = json_add_obj_by_fca(obj, fca, namepos, namelen, 0);
                iter = pos + json_fca_decode(subobj, fca, pos, endpos-pos);

            } else if (fch == '[') { //array
                pos++;
                do {
                    iter = file_cache_skip_over(fca, pos, endpos-pos, ", \t\r\n", 5);
                    if (iter >= endpos) return iter - startpos;
                    if ((fch = file_cache_at(fca, iter)) == '}') return iter - startpos + 1;

                    if (fch == ']') { iter++; break; }

                    if (fch == '{') {
                        subobj = json_add_obj_by_fca(obj, fca, namepos, namelen, 1);
                        pos = iter + json_fca_decode(subobj, fca, iter, endpos-iter);
                    } else {
                        comma = file_cache_skip_quote_to(fca, iter, endpos-iter, ",]}", 3);
                        if (fch == '"' || fch == '\'') {
                            valuepos = iter + 1;
                            pat[0] = fch;
                            pos = file_cache_skip_esc_to(fca, iter+1, comma-iter-1, pat, 1);
                            pos--;
                        } else {
                            valuepos = iter;
                            pos = file_cache_rskip_over(fca, comma-1, comma-iter, " \t\r\n", 4);
                        }
                        valuelen = pos - valuepos + 1;
                        json_add_by_fca(obj, fca, namepos, namelen, valuepos, valuelen, 1);

                        pos = comma;
                    }
                } while (pos < endpos);

            } else { //generic string value
                comma = file_cache_skip_quote_to(fca, pos, endpos-pos, ",}", 2);
                if (fch == '"' || fch == '\'') {
                    valuepos = pos + 1;
                    pat[0] = fch;
                    pos = file_cache_skip_esc_to(fca, pos+1, comma-pos-1, pat, 1);
                    pos--;
                } else {
                    valuepos = pos;
                    pos = file_cache_rskip_over(fca, comma-1, comma-pos, " \t\r\n", 4);
                }
                valuelen = pos - valuepos + 1;
                json_add_by_fca(obj, fca, namepos, namelen, valuepos, valuelen, 0);

                iter = comma;
            }
        }
    }
 
    return iter-startpos;
}
 


int json_value_encode (void * vjval, void * vjson, int len)
{
    JsonValue * jval = (JsonValue *)vjval;
    char      * pjson = (char *)vjson;
    int         iter = 0;
    JsonObj   * subobj = NULL;

    if (!jval) return 0;
    if (!pjson || len <= 0) return 0;

    if (jval->valtype == 0) {
        if (pjson && iter + 1 <= len) pjson[iter] = '"';
        iter++;

        if (jval->value && jval->valuelen > 0) {
            iter += json_escape(jval->value, jval->valuelen, pjson + iter, len - iter);
        }

        if (iter + 1 <= len) pjson[iter] = '"';
        iter++;

    } else {
        subobj = jval->jsonobj;
        iter += json_encode(subobj, pjson+iter, len-iter);
    }

    return iter;
}

int json_encode (void * vobj, void * vjson, int len)
{
    JsonObj   * obj = (JsonObj *)vobj;
    char      * pjson = (char *)vjson;
    JsonItem  * item = NULL;
    JsonValue * jval = NULL;
    int         iter = 0;
    int         i, num;
    int         j;
    uint8       needcomma = 0;

    if (!obj) return 0;

    if (iter + 1 <= len) pjson[iter] = '{';
    iter++;

    num = ht_num(obj->objtab);

    for (i = 0; i < num; i++) {

        item = (JsonItem *)ht_value(obj->objtab, i);
        if (!item || !item->name || item->namelen <= 0) continue;
        if (item->valnum <= 0) continue;

        if (needcomma) {
            //if (iter + 2 <= len) memcpy(pjson+iter, ", ", 2);
            if (iter + 2 <= len) { pjson[iter] = obj->itemsep; pjson[iter+1] = ' '; }
            iter += 2;
        }

        if (iter + 1 <= len) pjson[iter] = '"';
        iter++;

        if (iter + item->namelen <= len)
            memcpy(pjson+iter, item->name, item->namelen);
        iter += item->namelen;

        if (iter + 1 <= len) pjson[iter] = '"';
        iter++;
        //if (iter + 1 <= len) pjson[iter] = ':';
        if (iter + 1 <= len) pjson[iter] = obj->kvsep;
        iter++;

        if (item->arrflag == 1) {
            if (iter+1 <= len) pjson[iter] = '[';
            iter++;
        } else if (item->arrflag == 2 && item->valnum > 1) {
            if (iter+1 <= len) pjson[iter] = '[';
            iter++;
        }

        if (item->valnum == 1) {
            jval = (JsonValue *)item->valobj;
            if (jval) iter += json_value_encode(jval, pjson+iter, len-iter);

        } else {
            for (j = 0; j < item->valnum; j++) {
                if (j > 0) {
                    if (iter + 2 <= len) memcpy(pjson+iter, ", ", 2);
                    iter += 2;
                }

                jval = arr_value((arr_t *)item->valobj, j);
                if (jval) iter += json_value_encode(jval, pjson+iter, len-iter);
            }
        }

        if (item->arrflag == 1) {
            if (iter+1 <= len) pjson[iter] = ']';
            iter++;
        } else if (item->arrflag == 2 && item->valnum > 1) {
            if (iter+1 <= len) pjson[iter] = ']';
            iter++;
        }

        needcomma = 1;
    }

    if (iter+1 <= len) pjson[iter] = '}';
    iter++;

    return iter;
}


int json_value_encode2 (void * vjval, frame_p objfrm)
{ 
    JsonValue * jval = (JsonValue *)vjval;
    int         iter = 0; 
    JsonObj   * subobj = NULL;
     
    if (!jval) return 0;
 
    if (jval->valtype == 0) {
        frame_put_last(objfrm, '"'); iter++;

        if (jval->value && jval->valuelen > 0) {
            iter += frame_json_escape(jval->value, jval->valuelen, objfrm);
        }

        frame_put_last(objfrm, '"'); iter++;

    } else {
        subobj = jval->jsonobj;
        iter += json_encode2(subobj, objfrm);
    }
 
    return iter;
}

int json_encode2 (void * vobj, frame_p objfrm)
{        
    JsonObj   * obj = (JsonObj *)vobj;
    JsonItem  * item = NULL; 
    JsonValue * jval = NULL; 
    int         iter = 0;
    int         i, num; 
    int         j;
    uint8       needcomma = 0;
 
    if (!obj) return 0;
 
    frame_put_last(objfrm, '{'); iter++;
 
    num = ht_num(obj->objtab);

    for (i = 0; i < num; i++) {

        item = (JsonItem *)ht_value(obj->objtab, i);
        if (!item || !item->name || item->namelen <= 0) continue;
        if (item->valnum <= 0) continue;
 
        if (needcomma) {
            frame_put_last(objfrm, obj->itemsep);
            frame_put_last(objfrm, ' ');
            iter += 2;
        }
 
        frame_put_last(objfrm, '"'); iter++;
        frame_put_nlast(objfrm, item->name, item->namelen); iter += item->namelen;
        frame_put_last(objfrm, '"'); iter++;
        frame_put_last(objfrm, obj->kvsep); iter++;

        if (item->arrflag == 1) {
            frame_put_last(objfrm, '['); iter++;
        } else if (item->arrflag == 2 && item->valnum > 1) {
            frame_put_last(objfrm, '['); iter++;
        }

        if (item->valnum == 1) {
            jval = (JsonValue *)item->valobj;
            if (jval) iter += json_value_encode2(jval, objfrm);

        } else {
            for (j = 0; j < item->valnum; j++) {
                if (j > 0) { frame_append(objfrm, ", "); iter += 2; }
 
                jval = arr_value((arr_t *)item->valobj, j);
                if (jval) iter += json_value_encode2(jval, objfrm);
            }
        }

        if (item->arrflag == 1) {
            frame_put_last(objfrm, ']'); iter++;
        } else if (item->arrflag == 2 && item->valnum > 1) {
            frame_put_last(objfrm, ']'); iter++;
        }
 
        needcomma = 1;
    }
 
    frame_put_last(objfrm, '}'); iter++;

    return iter;
}

