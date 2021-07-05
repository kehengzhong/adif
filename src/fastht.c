/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include "btype.h"
#include <math.h>
#include "memory.h"
#include "fastht.h"

typedef int (FAST_HASH_FREE) (void * val, int valen);

uint32 hash_crypt_table[1280] = { 0 };
int    crypt_table_init = 0;


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

void fast_ht_crypt_table_init ()
{
    uint32  seed = 0x00100001;
    uint32  index1 = 0, index2 = 0;
    uint32  temp1, temp2;
    uint32  i;

    if (crypt_table_init) return;

    for (index1 = 0; index1 < 256; index1++) {
        for (index2 = index1, i = 0; i < 5; i++, index2 += 256) {
            seed = (seed * 125 + 3) % 0x2AAAAB;
            temp1 = (seed & 0xFFFF) << 16;
    
            seed = (seed * 125 + 3) % 0x2AAAAB;
            temp2 = (seed & 0xFFFF);
    
            hash_crypt_table[index2] = (temp1 | temp2);
        }
    }

    crypt_table_init = 1;
}


uint32 fast_hash_func (void * pkey, int keylen, int hashtype)
{
    uint8   * key = (uint8 *)pkey;
    int       i = 0;
    uint32    seed1 = 0x7FED7FED;
    uint32    seed2 = 0xEEEEEEEE;
    int       ch;

    if (!key) return 0;
    if (keylen < 0) keylen = strlen((char *)key);
    if (keylen <= 0) return 0;

    if (!crypt_table_init)
        fast_ht_crypt_table_init();

    for (i = 0; i < keylen; i++) {
        ch = adf_toupper(key[i]);
    
        seed1 = hash_crypt_table[(hashtype << 8) + ch] ^ (seed1 + seed2);
        seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;
    }

    return seed1;
}
 

void * fast_ht_new (ulong num)
{
    FastHashTab * ht = NULL;
 
    ht = kzalloc(sizeof(*ht));
    if (ht == NULL) return NULL;
 
    ht->reqsize = num;
    ht->size = find_a_prime (num);
    ht->num = 0;
 
    ht->ptab = kzalloc(ht->size * sizeof(FastHashNode));
    if (ht->ptab == NULL) {
        kfree(ht);
        return NULL;
    }

    return ht;
}


void fast_ht_free (void * vht)
{
    FastHashTab * ht = (FastHashTab *)vht;

    if (!ht) return;
 
    kfree(ht->ptab);
    kfree(ht);
}
 
 
void fast_ht_free_all (void * vht, void * vfunc)
{
    FastHashTab * ht = (FastHashTab *)vht;
    ulong         i = 0;
    FAST_HASH_FREE * func = (FAST_HASH_FREE *)vfunc;
 
    if (!ht) return;
 
    if (func == NULL) {
        fast_ht_free(ht);
        return;
    }
 
    for (i = 0; i < ht->size; i++) {
        if (ht->ptab[i].exist) {
            func(ht->ptab[i].value, ht->ptab[i].valuelen);
        }
    }
 
    kfree(ht->ptab);
    kfree(ht);
}
 
void fast_ht_free_member (void * vht, void * vfunc)
{
    FastHashTab * ht = (FastHashTab *)vht;
    ulong         i = 0;
    FAST_HASH_FREE * func = (FAST_HASH_FREE *)vfunc;
 
    if (!ht) return;
 
    if (func == NULL) { 
        fast_ht_zero(ht);
        return;
    }
 
    for (i = 0; i < ht->size; i++) {
        if (ht->ptab[i].exist) {
            func(ht->ptab[i].value, ht->ptab[i].valuelen);
        }
 
        memset(&ht->ptab[i], 0, sizeof(ht->ptab[i]));
    }
 
    ht->num = 0;
}
 
 
void fast_ht_zero (void * vht)
{
    FastHashTab * ht = (FastHashTab *)vht;
    ulong         i = 0;
 
    if (!ht) return;
 
    for (i = 0; i < ht->size; i++) {
        if (ht->ptab[i].exist) {
            memset(&ht->ptab[i], 0, sizeof(ht->ptab[i]));
        }
    }
    ht->num = 0;
}
 
int fast_ht_num (void * vht)
{
    FastHashTab * ht = (FastHashTab *)vht;

    if (!ht) return 0;
 
    return ht->num;
}
 
void * fast_ht_get (void * vht, void * key, int keylen, void ** pval, int * vallen)
{
    FastHashTab * ht = (FastHashTab *)vht;
    uint32  hash = 0;
    uint32  hashA = 0;
    uint32  hashB = 0;
    uint32  hashbgn = 0;

    if (!ht) return NULL;

    hash = fast_hash_func(key, keylen, HASH_OFFSET);
    hashA = fast_hash_func(key, keylen, HASH_A);
    hashB = fast_hash_func(key, keylen, HASH_B);

    hash %= ht->size;
    hashbgn = hash;

    while (ht->ptab[hash].exist) {
        if (ht->ptab[hash].hashA == hashA && ht->ptab[hash].hashB == hashB) {
            if (pval) *pval = ht->ptab[hash].value;
            if (vallen) *vallen = ht->ptab[hash].valuelen;
            return ht->ptab[hash].value;
        }

        hash = (hash + 1) % ht->size;
     
        if (hash == hashbgn) break;
    }

    if (pval) *pval = NULL;
    if (vallen) *vallen = 0;
    return NULL;
}

int fast_ht_set (void * vht, void * key, int keylen, void * value, int valuelen)
{
    FastHashTab * ht = (FastHashTab *)vht;
    uint32  hash = 0;
    uint32  hashA = 0;
    uint32  hashB = 0;
    uint32  hashbgn = 0;

    if (!ht) return -1;

    hash = fast_hash_func(key, keylen, HASH_OFFSET);
    hashA = fast_hash_func(key, keylen, HASH_A);
    hashB = fast_hash_func(key, keylen, HASH_B);

    hash %= ht->size;
    hashbgn = hash;

    while (ht->ptab[hash].exist) {
        if (ht->ptab[hash].hashA == hashA && ht->ptab[hash].hashB == hashB) {
            ht->ptab[hash].value = value;
            ht->ptab[hash].valuelen = valuelen;
            return 0;
        }

        hash = (hash + 1) % ht->size;
        if (hash == hashbgn) return -100;
    }

    ht->ptab[hash].exist = 1;
    ht->ptab[hash].hashA = hashA;
    ht->ptab[hash].hashB = hashB;
    ht->ptab[hash].value = value;
    ht->ptab[hash].valuelen = valuelen;
    return 1;
}

void * fast_ht_del (void * vht, void * key, int keylen, void ** pval, int * vallen)
{
    FastHashTab * ht = (FastHashTab *)vht;
    uint32   hash = 0;
    uint32   hashA = 0;
    uint32   hashB = 0;
    uint32   hashbgn = 0;
    void   * old = NULL;

    if (!ht) return NULL;

    hash = fast_hash_func(key, keylen, HASH_OFFSET);
    hashA = fast_hash_func(key, keylen, HASH_A);
    hashB = fast_hash_func(key, keylen, HASH_B);

    hash %= ht->size;
    hashbgn = hash;

    while (ht->ptab[hash].exist) {
        if (ht->ptab[hash].hashA == hashA && ht->ptab[hash].hashB == hashB) {
            if (pval) *pval = ht->ptab[hash].value;
            if (vallen) *vallen = ht->ptab[hash].valuelen;

            old = ht->ptab[hash].value;

            memset(&ht->ptab[hash], 0, sizeof(ht->ptab[hash]));
            return old;
        }

        hash = (hash + 1) % ht->size;
     
        if (hash == hashbgn) break;
    }

    if (pval) *pval = NULL;
    if (vallen) *vallen = 0;
    return NULL;
}

