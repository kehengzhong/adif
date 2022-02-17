/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#ifndef _JSON_H_
#define _JSON_H_

#include "frame.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct json_value {
    uint8              valtype; //0-generic string value, 1-object value
 
    uint8            * value;
    int                valuelen;
 
    void             * jsonobj;
} JsonValue;
 
 
typedef struct json_item {
    uint8              valtype; //0-generic string value, 1-object value
 
    uint8            * name;
    int                namelen;
 
    uint8              arrflag; //0-only one value 1-array multiple value/sub-objs
    int                valnum;
    void             * valobj;  //num=1->JsonValue, num>1->arr_t containing JsonValue array
} JsonItem;
 
void * json_item_alloc ();
int    json_item_free (void * vitem);

typedef struct json_obj {
 
    CRITICAL_SECTION   objCS;
    hashtab_t        * objtab;
    int                bytenum;
 
    uint8              kvsep;
    uint8              itemsep;
 
    uint8              sp[8];
    uint8              kvsp[8];
    uint8              arrsp[8];
    uint8              keyend[8];
    uint8              arrend[8];
    uint8              kvend[8];
 
    unsigned           sptype:2;   //0-standard separator(:,) 1-conf-like separator(=;)
    unsigned           cmtflag:2;  //0-not support comment  1-support comment
    unsigned           sibcoex:2;  //0-same keys exclude  1-same keys coexist
 
    unsigned           splen:4;
    unsigned           kvsplen:4;
    unsigned           arrsplen:4;
    unsigned           keyendlen:4;
    unsigned           arrendlen:4;
    unsigned           kvendlen:4;
 
} JsonObj;

void * json_item_get (void * vobj, void * name, int namelen);
int    json_item_add (void * vobj, void * name, int namelen, void * vitem);
void * json_item_del (void * vobj, void * name, int namelen);


/* sptype: separator type
    0-->  { "key":"value", "name":"data"}
    1-->  { "key"="value"; "name"="data"}
 */
void * json_init  (int sptype, int cmtflag, int sibcoex);
int    json_clean (void * vobj);

int    json_size (void * vobj);

int    json_valuenum (void * vobj, void * key, int keylen);

int    json_num (void * vobj);
int    json_iter (void * vobj, int ind, int valind, void ** pkey, int * keylen, void ** pval, int * vallen, void ** pobj);

       /* "http.server.location[0].errpage.504" : "504.html" */
int    json_mdel (void * vobj, void * key, int keylen);

       /* http.server.location[0].errpage.504 */
int    json_mget_value (void * vobj, void * key, int keylen, void ** pval, int * vallen, void ** pobj);
int    json_mget     (void * vobj, void * key, int keylen, void * val, int * vallen);
int    json_mgetP    (void * vobj, void * key, int keylen, void ** pval, int * vallen);
int    json_mget_obj (void * vobj, void * key, int keylen, void ** pobj);

int    json_mget_int8   (void * vobj, void * key, int keylen, int8 * val);
int    json_mget_uint8  (void * vobj, void * key, int keylen, uint8 * val);
int    json_mget_int16  (void * vobj, void * key, int keylen, int16 * val);
int    json_mget_uint16 (void * vobj, void * key, int keylen, uint16 * val);
int    json_mget_int    (void * vobj, void * key, int keylen, int * val);
int    json_mget_uint32 (void * vobj, void * key, int keylen, uint32 * val);
int    json_mget_long   (void * vobj, void * key, int keylen, long * val);
int    json_mget_ulong  (void * vobj, void * key, int keylen, ulong * val);
int    json_mget_int64  (void * vobj, void * key, int keylen, int64 * val);
int    json_mget_uint64 (void * vobj, void * key, int keylen, uint64 * val);
int    json_mget_double (void * vobj, void * key, int keylen, double * val);

int    json_del     (void * vobj, void * key, int keylen);

int    json_get     (void * vobj, void * key, int keylen, int ind, void * val, int vallen);
int    json_getP    (void * vobj, void * key, int keylen, int ind, void * pval, int * vallen);
int    json_get_obj (void * vobj, void * key, int keylen, int ind, void ** pobj);

int    json_get_int8   (void * vobj, void * key, int keylen, int ind, int8 * val);
int    json_get_uint8  (void * vobj, void * key, int keylen, int ind, uint8 * val);
int    json_get_int16  (void * vobj, void * key, int keylen, int ind, int16 * val);
int    json_get_uint16 (void * vobj, void * key, int keylen, int ind, uint16 * val);
int    json_get_int    (void * vobj, void * key, int keylen, int ind, int * val);
int    json_get_uint32 (void * vobj, void * key, int keylen, int ind, uint32 * val);
int    json_get_long   (void * vobj, void * key, int keylen, int ind, long * val);
int    json_get_ulong  (void * vobj, void * key, int keylen, int ind, ulong * val);
int    json_get_int64  (void * vobj, void * key, int keylen, int ind, int64 * val);
int    json_get_uint64 (void * vobj, void * key, int keylen, int ind, uint64 * val);
int    json_get_double (void * vobj, void * key, int keylen, int ind, double * val);

/* if key exists, it's value will be appended the new content */
int    json_append      (void * vobj, void * key, int keylen, void * val, int vallen, uint8 strip);
int    json_append_file (void * vobj, void * key, int keylen, char * fname, long startpos, long length);

/* if key exists, a new json-value will be created and bound to the key */
int    json_add (void * vobj, void * key, int ken, void * val, int vlen, uint8 isarr, uint8 strip);

#define json_add_str(obj, key, klen, val, vlen, isarr) json_add(obj, key, klen, val, vlen, isarr, 0)

void * json_add_obj (void * vobj, void * key, int keylen, uint8 isarr);

int    json_add_by_fca     (void * obj, void * fca, long pos, int len, long vpos, int vlen, uint8 isarr);
void * json_add_obj_by_fca (void * vobj, void * fca, long keypos, int keylen, uint8 isarr);

int    json_add_int8   (void * vobj, void * key, int keylen, int8 val, uint8 isarr);
int    json_add_uint8  (void * vobj, void * key, int keylen, uint8 val, uint8 isarr);
int    json_add_int16  (void * vobj, void * key, int keylen, int16 val, uint8 isarr);
int    json_add_uint16 (void * vobj, void * key, int keylen, uint16 val, uint8 isarr);
int    json_add_int    (void * vobj, void * key, int keylen, int val, uint8 isarr);
int    json_add_uint32 (void * vobj, void * key, int keylen, uint32 val, uint8 isarr);
int    json_add_long   (void * vobj, void * key, int keylen, long val, uint8 isarr);
int    json_add_ulong  (void * vobj, void * key, int keylen, ulong val, uint8 isarr);
int    json_add_int64  (void * vobj, void * key, int keylen, int64 val, uint8 isarr);
int    json_add_uint64 (void * vobj, void * key, int keylen, uint64 val, uint8 isarr);
int    json_add_double (void * vobj, void * key, int keylen, double val, uint8 isarr);

int    json_decode (void * vobj, void * pjson, int length, int findobjbgn, int strip);
int    json_decode_file (void * vobj, void * fn, int fnlen, int findobjbgn, int strip);

int    json_encode (void * vobj, void * pjson, int len);
int    json_encode2 (void * vobj, frame_p objfrm);

long   json_fca_decode (void * vobj, void * fca, long startpos, long length);

#ifdef __cplusplus
}
#endif

#endif

