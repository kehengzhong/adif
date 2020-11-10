/*
 *  Copyright (c) 2012-2017, Jyri J. Virkki
 *  All rights reserved.  Original file is under BSD license.  
 *
 *  The bloom source is modified from libbloom project by Jyri J. Virkki
 */

 
#include "btype.h"
#include <math.h>
#include "memory.h"
#include "hashtab.h"
#include "bloom.h"

inline static int test_bit_set_bit (uint8 * buf, uint64 x, int set_bit)
{
    uint64   byte = x >> 3;
    uint8    c = buf[byte];
    uint32   mask = 1 << (x % 8);
 
    if (c & mask) {
        return 1;

    } else {
        if (set_bit) {
            buf[byte] = c | mask;
        }

        return 0;
    }
}
 
static int bloom_check_add (bloom_p bf, void * key, int len, int add)
{
    int      hits = 0;
    register uint64 a;
    register uint64 b;
    register uint64 x;
    register uint64 i;
 
    if (!bf) return -1;

    a = murmur_hash2_64(key, len, 0x7af9cb4d9747b28c);
    b = murmur_hash2_64(key, len, a);

    for (i = 0; i < (uint64)bf->hashes; i++) {
        x = (a + i*b) % bf->bits;

        if (test_bit_set_bit(bf->bitarr, x, add)) {
            hits++;

        } else if (!add) {
            return 0;
        }
    }
 
    if (hits == bf->hashes) {
        return 1;    // 1 == element already in (or collision)
    }
 
    return 0;
}

bloom_p bloom_new (uint64 entries, double error)
{
    bloom_t * bf = NULL;
    double    num;
    double    denom;
    double    dentries;

    bf = kzalloc(sizeof(*bf));
    if (!bf) return NULL;

    if (entries < 1000) entries = 1000;
    if (error == 0) error = 0.0001;
   
    bf->entries = entries;
    bf->error = error;
   
    num = log(bf->error);
    denom = 0.480453013918201; // ln(2)^2
    bf->bpe = -(num / denom);
   
    dentries = (double)entries;
    bf->bits = (uint64)(dentries * bf->bpe);
   
    if (bf->bits % 8) {
        bf->bytes = (bf->bits / 8) + 1;
    } else {
        bf->bytes = bf->bits / 8;
    }
   
    bf->hashes = (int)ceil(0.693147180559945 * bf->bpe);  // ln(2)
   
    bf->bitarr = (uint8 *)kalloc(bf->bytes * sizeof(uint8));
    if (bf->bitarr == NULL) {
        kfree(bf);
        return NULL;
    }

    return bf;
}

void bloom_free (bloom_t * bf)
{
    if (!bf) return;

    if (bf->bitarr) {
        kfree(bf->bitarr);
        bf->bitarr = NULL;
    }

    kfree(bf);
}
 
int bloom_add (bloom_t * bf, void * key, int keylen)
{
    return bloom_check_add(bf, key, keylen, 1);
}

int bloom_check (bloom_t * bf, void * key, int keylen)
{
    return bloom_check_add(bf, key, keylen, 0);
}
 
int blomm_reset (bloom_t * bf)
{
    if (!bf) return -1;

    memset(bf->bitarr, 0, bf->bytes);

    return 0;
}

void bloom_print (bloom_t * bloom)
{
    printf("bloom at %p\n", (void *)bloom);
    printf(" ->entries = %llu\n", bloom->entries);
    printf(" ->error = %f\n", bloom->error);
    printf(" ->bits = %llu\n", bloom->bits);
    printf(" ->bits per elem = %f\n", bloom->bpe);
    printf(" ->bytes = %llu\n", bloom->bytes);
    printf(" ->hash functions = %d\n", bloom->hashes);
}

