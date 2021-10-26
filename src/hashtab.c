/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include "btype.h"
#include "memory.h"
#include "strutil.h"
#include "hashtab.h"
#include <math.h>

#define HASH_SHIFT  6
#define HASH_VALUE_BITS  32 
static long s_mask = ~0U << (HASH_VALUE_BITS - HASH_SHIFT);

static ulong find_a_prime (ulong baseNum)
{
    ulong num = 0, i = 0;

    num = baseNum;
    if ((num % 2) == 0) num++;

    for (i = 3; i < (unsigned long)((1 + (long)(sqrt((double)num)))); i += 2) {
        if ((num % i) == 0) {
            i = 1;
            num += 2;
        }
    }

    return num;
}

ulong generic_hash (void * key, int keylen, ulong seed)
{
    uint8  * p = NULL;
    ulong    hash;
    int      i;

    if (!key) return seed;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return seed;

    hash = seed ^ keylen;

    p = (uint8 *)key;
    for (i = 0; i < keylen; i++) {
        hash = (hash & s_mask) ^ (hash << HASH_SHIFT) ^ p[i];
    }

    return hash;
}

ulong string_hash (void * key, int keylen, ulong seed)
{
    uint8  * p = NULL;
    ulong    hash;
    int      i;
 
    if (!key) return seed;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return seed;
 
    hash = seed ^ keylen;
 
    p = (uint8 *)key;
    for (i = 0; i < keylen; i++) {
        hash = (hash & s_mask) ^ (hash << HASH_SHIFT) ^ adf_tolower(p[i]);
    }
 
    return hash;
}

ulong ckstr_generic_hash (void * vkey)
{
    ckstr_t * key = (ckstr_t *)vkey;

    if (!key) return 0;

    return generic_hash(key->p, key->len, 0);
}

ulong ckstr_string_hash (void * vkey)
{
    ckstr_t * key = (ckstr_t *)vkey;
 
    if (!key) return 0;
 
    return string_hash(key->p, key->len, 0);
}

ulong fake_hash (void * key, int keylen, ulong seed)
{
    uint8  * p = NULL;
    ulong    hash;
    int      i;
 
    if (!key) return seed;
    if (keylen < 0) keylen = str_len(key);
    if (keylen <= 0) return seed;
 
    hash = seed ^ keylen;
 
    p = (uint8 *)key;
    for (i = 0; i < keylen; i++) {
        hash ^= hash >> 3;
        hash ^= hash << 5;
        hash ^= adf_tolower(p[i]);
    }
 
    return hash;
}
 
uint32 murmur_hash2 (void * key, int len, uint32 seed)
{
    uint8 * data = (uint8 *)key;
    uint32  h, k;
 
    if (!key) return seed;
    if (len < 0) len = str_len(key);
    if (len <= 0) return seed;
 
    h = seed ^ len;
 
    while (len >= 4) {
        k  = data[0];
        k |= data[1] << 8;
        k |= data[2] << 16;
        k |= data[3] << 24;
 
        k *= 0x5bd1e995;
        k ^= k >> 24;
        k *= 0x5bd1e995;
 
        h *= 0x5bd1e995;
        h ^= k;
 
        data += 4;
        len -= 4;
    }
 
    switch (len) {
    case 3:
        h ^= data[2] << 16;
        /* fall through */
    case 2:
        h ^= data[1] << 8;
        /* fall through */
    case 1:
        h ^= data[0];
        h *= 0x5bd1e995;
    }
 
    h ^= h >> 13;
    h *= 0x5bd1e995;
    h ^= h >> 15;
 
    return h;
}

uint64 murmur_hash2_64 (void * key, int len, uint64 seed )
{
    const uint64 * data = NULL;
    const uint64 * end = NULL;
    const uint8  * data2 = NULL;
    const uint64   m = 0xc6a4a7935bd1e995ULL;
    const int      r = 47;
    uint64         k = 0; 
    uint64         h = 0;
 
    if (!key) return seed;
    if (len < 0) len = str_len(key);
    if (len <= 0) return seed;
 
    h = seed ^ (len * m);
 
    data = (const uint64 *)key;
    end = data + (len/8);
 
    while(data != end) {
        k = *data++;
   
        k *= m;
        k ^= k >> r;
        k *= m;
   
        h ^= k;
        h *= m;
    }
   
    data2 = (const uint8 *)data;
   
    switch(len & 7) {
    case 7: h ^= (uint64)(data2[6]) << 48;
    case 6: h ^= (uint64)(data2[5]) << 40;
    case 5: h ^= (uint64)(data2[4]) << 32;
    case 4: h ^= (uint64)(data2[3]) << 24;
    case 3: h ^= (uint64)(data2[2]) << 16;
    case 2: h ^= (uint64)(data2[1]) << 8;
    case 1: h ^= (uint64)(data2[0]);
            h *= m;
    };
   
    h ^= h >> r;
    h *= m;
    h ^= h >> r;
   
    return h;
}


static ulong hash_key (void * str)
{
    return generic_hash(str, -1, 0);
}

static ulong hash_string (void * str)
{
    return string_hash(str, -1, 0);
}

hashtab_t * ht_only_new (int num, HashTabCmp * cmp)
{
    hashtab_t * ret = NULL;
    int  i = 0;

    ret = kzalloc(sizeof(*ret));
    if (ret == NULL)
        return NULL;

    ret->num_requested = num;
    ret->len = find_a_prime (num);
    ret->num = 0;
    ret->cmp = cmp;
    ret->hashFunc = (HashFunc *)hash_string;

    ret->linear = 0;
    ret->nodelist = NULL;

    ret->ptab = kzalloc(ret->len * sizeof(hashnode_t));
    if (ret->ptab == NULL) {
        kfree(ret);
        return NULL;
    }
    for (i = 0; i < ret->len; i++) {
        ret->ptab[i].dptr = NULL;
        ret->ptab[i].count = 0;
        memset(&ret->ptab[i], 0, sizeof(ret->ptab[i]));
    }

    return ret;
}
 
hashtab_t * ht_new (int num, HashTabCmp * cmp)
{
    hashtab_t * ht = NULL;

    ht = ht_only_new(num, cmp);
    if (ht) {
        ht->linear = 1;
        ht->nodelist = arr_new(4);
    }

    return ht;
}

void ht_set_generic_hash (hashtab_t * ht)
{
    if (!ht) return;
 
    ht->hashFunc = hash_key;
}

void ht_set_hash_func (hashtab_t * ht, HashFunc * hashfunc)
{
    if (!ht || !hashfunc) return;

    ht->hashFunc = hashfunc;
}


void ht_free (hashtab_t * ht)
{
    int i;

    if (!ht) return;

    for (i = 0; i < ht->len; i++) {
        if (ht->ptab[i].count > 1) {
            arr_free((arr_t *)ht->ptab[i].dptr);
        }
    }

    arr_free(ht->nodelist);
    kfree(ht->ptab);
    kfree(ht);
}


void ht_free_all (hashtab_t * ht, void * vfunc)
{
    HashTabFree * func = (HashTabFree *)vfunc;
    int i = 0;

    if (!ht) return;

    if (func == NULL) {
        ht_free(ht);
        return;
    }

    for (i = 0; i < ht->len; i++) {
        if (ht->ptab[i].count == 1) {
            (*func)(ht->ptab[i].dptr);
        } else if (ht->ptab[i].count > 1) {
            arr_pop_free ((arr_t *)ht->ptab[i].dptr, func);
        }
    }

    arr_free(ht->nodelist);
    kfree(ht->ptab);
    kfree(ht);
}

void ht_free_member (hashtab_t * ht, void * vfunc)
{ 
    HashTabFree * func = (HashTabFree *)vfunc;
    int i = 0;
     
    if (!ht) return;
     
    if (!func) {
        ht_zero(ht); 
        return; 
    }
     
    for (i = 0; i < ht->len; i++) {
        if (ht->ptab[i].count == 1) {
            (*func)(ht->ptab[i].dptr);  
        } else if (ht->ptab[i].count > 1) {
            arr_pop_free((arr_t *)ht->ptab[i].dptr, func);
        }    

        ht->ptab[i].dptr = NULL;
        ht->ptab[i].count = 0;
    }    
     
    if (ht->linear) arr_zero(ht->nodelist);
    ht->num = 0;

#ifdef _DEBUG
    memset(&ht->collide_tab, 0, sizeof(ht->collide_tab));
#endif
}
 
void ht_zero (hashtab_t * ht)
{
    int i = 0;

    if (!ht) return;

    for (i = 0; i < ht->len; i++) {
        if (ht->ptab[i].count > 1) {
            arr_free((arr_t *)ht->ptab[i].dptr);
        }
        ht->ptab[i].dptr = NULL;
        ht->ptab[i].count = 0;
    }
    ht->num = 0;

    if (ht->linear) arr_zero(ht->nodelist);

#ifdef _DEBUG
    memset(&ht->collide_tab, 0, sizeof(ht->collide_tab));
#endif
}


int ht_num (hashtab_t * ht)
{
    if (!ht) return 0;

    return ht->num;
}


void * ht_get (hashtab_t * ht, void * key)
{
    ulong hash = 0;

    if (!ht || !key) return NULL;

    hash = (*ht->hashFunc)(key);
    hash %= ht->len;

    switch (ht->ptab[hash].count) {
    case 0:
        return NULL;

    case 1:
        if ((*ht->cmp)(ht->ptab[hash].dptr, key) == 0) {
            return ht->ptab[hash].dptr;
        } else
            return NULL;

    default:
        return arr_search(ht->ptab[hash].dptr, key, ht->cmp);
    }

    return NULL;
}


int ht_sort (hashtab_t * ht, HashTabCmp * cmp)
{
    if (!ht) return -1;

    if (ht->linear)
        arr_sort_by(ht->nodelist, cmp);

    return 0;
}


void * ht_value (hashtab_t * ht, int index)
{
    int  i = 0;
    int  num = 0;

    if (!ht) return NULL;

    if (index < 0 || index >= ht->num) return NULL;

    if (ht->linear)
        return arr_value(ht->nodelist, index);

    for (num = 0, i = 0; i < ht->len; i++) {
        switch(ht->ptab[i].count) {
        case 0: 
            continue;
        case 1:
            if (index == num) return ht->ptab[i].dptr;
            if (index < num) return NULL;
            num += 1;
            break;
        default:
            if (index >= num + ht->ptab[i].count) {
                num += ht->ptab[i].count;
                continue;
            } else {
                return arr_value(ht->ptab[i].dptr, index - num);
            }
        }
    }

    return NULL;
}


void ht_traverse (hashtab_t * ht, void * usrInfo, void (*check)(void *, void *))
{
    int i = 0, j = 0;

    if (!ht || !check) return;

    for (i = 0; i < ht->len; i++) {
        switch (ht->ptab[i].count) {
        case 0: 
            break;
        case 1:
            (*check)(usrInfo, ht->ptab[i].dptr);
            break;
        default:
            for (j = 0; j < arr_num(ht->ptab[i].dptr); j++) {
                check(usrInfo, arr_value(ht->ptab[i].dptr, j));
            }
            break;
        }
    }
}



int ht_set (hashtab_t * ht, void * key, void * value)
{
    ulong   hash = 0;
    arr_t * valueList = NULL;

    if (!ht || !key) return -1;

    hash = (*ht->hashFunc)(key);

    hash %= ht->len;

    switch (ht->ptab[hash].count) {
    case 0:
        ht->ptab[hash].dptr = value;
        ht->ptab[hash].count++;
        ht->num++;

        if (ht->linear) arr_push(ht->nodelist, value);

#ifdef _DEBUG
        ht->collide_tab[ht->ptab[hash].count] += 1;
#endif
        return ht->ptab[hash].count;
 
    case 1:
        if ((*ht->cmp)(ht->ptab[hash].dptr, key) == 0) {
            return ht->ptab[hash].count;
        }

        ht->ptab[hash].count++;
        valueList = arr_new(4);
        arr_push(valueList, ht->ptab[hash].dptr);
        arr_push(valueList, value);
        ht->ptab[hash].dptr = valueList;
        ht->num++;

        if (ht->linear) arr_push(ht->nodelist, value);

#ifdef _DEBUG
        ht->collide_tab[ht->ptab[hash].count-1] -= 1;
        ht->collide_tab[ht->ptab[hash].count] += 1;
#endif
        return ht->ptab[hash].count;

    default:
        if (arr_search(ht->ptab[hash].dptr, key, ht->cmp) != NULL)
            return 0;

        ht->ptab[hash].count++;
        arr_push(ht->ptab[hash].dptr, value);
        ht->num++;

        if (ht->linear) arr_push(ht->nodelist, value);

#ifdef _DEBUG
        if (ht->ptab[hash].count <= sizeof(ht->collide_tab)/sizeof(int) - 1) {
            ht->collide_tab[ht->ptab[hash].count-1] -= 1;
            ht->collide_tab[ht->ptab[hash].count] += 1;
        } else {
            if (ht->ptab[hash].count-1 <= sizeof(ht->collide_tab)/sizeof(int) - 1)
                ht->collide_tab[ht->ptab[hash].count-1] -= 1;
            ht->collide_tab[0] += 1;
        }
#endif
        return ht->ptab[hash].count;
    }

    return 0;
}


void * ht_delete (hashtab_t * ht, void * key)
{
    ulong  hash = 0;
    void * value = NULL;

    if (!ht || !key) return NULL;

    hash = (*ht->hashFunc)(key);
    if (hash < 0) return NULL;

    hash %= ht->len;

    switch (ht->ptab[hash].count) {
    case 0:
        return NULL;

    case 1:
        if ((*ht->cmp)(ht->ptab[hash].dptr, key) == 0) {
            ht->ptab[hash].count--;
            ht->num--;
            if (ht->linear) arr_delete_ptr(ht->nodelist, ht->ptab[hash].dptr);
#ifdef _DEBUG
            ht->collide_tab[ht->ptab[hash].count+1] -= 1;
#endif
            return ht->ptab[hash].dptr;
        } else
            return NULL;

    default:
        value = arr_search(ht->ptab[hash].dptr, key, ht->cmp);
        if (value == NULL) return NULL;

        if (ht->linear) arr_delete_ptr(ht->nodelist, value);

        arr_delete_ptr(ht->ptab[hash].dptr, value);
        ht->ptab[hash].count--;
        ht->num--;

        if (arr_num((arr_t *)ht->ptab[hash].dptr) == 1) {
            void * tmp = arr_value(ht->ptab[hash].dptr, 0);
            arr_free(ht->ptab[hash].dptr);
            ht->ptab[hash].dptr = tmp;
            ht->ptab[hash].count = 1;
        }
#ifdef _DEBUG
        if (ht->ptab[hash].count < sizeof(ht->collide_tab)/sizeof(int) - 1) {
            ht->collide_tab[ht->ptab[hash].count] += 1;
            ht->collide_tab[ht->ptab[hash].count+1] -= 1;
        } else {
            if (ht->ptab[hash].count <= sizeof(ht->collide_tab)/sizeof(int) - 1)
                ht->collide_tab[ht->ptab[hash].count] += 1;
            ht->collide_tab[0] -= 1;
        }
#endif
        return value;
    }

    return NULL;
}



void ht_delete_pattern (hashtab_t * ht, void * usrInfo, HashTabCmp * cmp, HashTabFree * freeFunc)
{
    int i = 0, j = 0;

    if (!ht || !cmp || !freeFunc) return;

    for (i = 0; i < ht->len; i++) {
        switch (ht->ptab[i].count) {
        case 0:
            break;
        case 1:
            if ((*cmp)(ht->ptab[i].dptr, usrInfo) == 0) {
                ht->ptab[i].count--;
                ht->num--;
                if (ht->linear) arr_delete_ptr(ht->nodelist, ht->ptab[i].dptr);
                (*freeFunc)(ht->ptab[i].dptr);
            }
            break;
        default:
            for (j = 0; j < arr_num(ht->ptab[i].dptr); j++) {
                if ((*cmp)(arr_value(ht->ptab[i].dptr, j), usrInfo) == 0) {
                    ht->ptab[i].count--;
                    ht->num--;
                    if (ht->linear) arr_delete_ptr(ht->nodelist, arr_value(ht->ptab[i].dptr, j));
                    (*freeFunc)(arr_delete(ht->ptab[i].dptr, j));
                }
            }
            break;
        }
    }

    return;
}

void print_hashtab (void * vht, FILE * fp)
{
#ifdef _DEBUG
    hashtab_t * ht = (hashtab_t *)vht;
    int i=0;
    int total = 0;
    int num = sizeof(ht->collide_tab)/sizeof(int);

    if (!ht) return;

    //fprintf(fp, "\n");
    fprintf(fp, "-----------------------Hash Table---------------------\n");
    fprintf(fp, "Total Bucket Number: %d\n", ht->len);
    fprintf(fp, "Req Bucket Number  : %d\n", ht->num_requested);
    fprintf(fp, "Stored Data Number : %d\n", ht->num);
    for (i=1; i<num; i++) {
        total += ht->collide_tab[i];
        if (ht->collide_tab[i] > 0)
            fprintf(fp, "    Buckets Storing %d Data: %d\n", i, ht->collide_tab[i]);
    }
    total += ht->collide_tab[0];
    if (ht->collide_tab[0] > 0)
        fprintf(fp, "    Buckets Storing >= %d Data: %d\n", num, ht->collide_tab[0]);
    fprintf(fp, "Buckets Storing at least 1 data: %d\n", total);
    fprintf(fp, "-------------------------------------------------------\n");
#endif
    return;
}
