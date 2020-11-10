/*
 *  Copyright (c) 2012-2017, Jyri J. Virkki
 *  All rights reserved.  Original file is under BSD license.
 *
 *  The bloom source is modified from libbloom project by Jyri J. Virkki
 */

#ifndef _BLOOM_H_
#define _BLOOM_H_

#ifdef __cplusplus
extern "C" {
#endif

/* bloom filter definition, please refer to
     http://en.wikipedia.org/wiki/Bloom_filter
 */

typedef struct bloom_s {

    /* The expected number of entries which will be inserted */
    int64       entries;

    /* Probability of collision, as long as entries are not exceeded */
    double      error;

    /* number of bits is calculated by entries and error :
     *  bits = (entries * ln(error)) / ln(2)^2  */
    int64       bits;
    int64       bytes;

    /* number of hash functions is from :
     * hashes = bpe * ln(2) */
    int         hashes;
 
    /* bits per element */
    double      bpe;

    uint8     * bitarr;

} bloom_t, *bloom_p;

bloom_p  bloom_new (uint64 entries, double error);
void     bloom_free (bloom_t * bf);

int      bloom_add   (bloom_t * bf, void * key, int keylen);
int      bloom_check (bloom_t * bf, void * key, int keylen);

int      blomm_reset (bloom_t * bf);

void     bloom_print (bloom_t * bloom);

#ifdef __cplusplus
}
#endif

#endif

