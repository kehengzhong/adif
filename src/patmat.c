/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include "btype.h"
#include "memory.h"
#include "bitarr.h"
#include "patmat.h"
#include "fileop.h"
#include "arfifo.h"


typedef struct mstr_s {
    uint8     ** pbyte;
    int        * plen;
    unsigned     num   : 30;
    unsigned     alloc : 1;

    int          len;
} MultiStr, mstr_t;

mstr_t * mstr_init (mstr_t * mstr, void * ppbyte[], int * plen, int num)
{
    int  i = 0;

    if (!mstr) {
        mstr = kzalloc(sizeof(*mstr));
        if (!mstr) return NULL;
        mstr->alloc = 1;
    } else {
        memset(mstr, 0, sizeof(*mstr));
    }

    mstr->pbyte = (uint8 **)ppbyte;
    mstr->plen = plen;
    mstr->num = num;

    for (i = 0; i < num; i++) {
        if (mstr->pbyte[i] != NULL && mstr->plen[i] > 0)
            mstr->len += mstr->plen[i];
    }

    return mstr;
}

int mstr_len (mstr_t * mstr)
{
    return mstr ? mstr->len : 0;
}

int mstr_char (mstr_t * mstr, int pos, int * ind)
{
    int  i, len = 0;

    if (!mstr) return -1;

    if (pos < 0 || pos >= mstr->len) return -2;

    for (i = 0, len = 0; i < (int)mstr->num; i++) {
        if (pos < len + mstr->plen[i]) {
            if (ind) *ind = i;
            return mstr->pbyte[i][pos - len];
        }

        len += mstr->plen[i];
    }

    return -3;
}

uint8 * mstr_p (mstr_t * mstr, int pos, int * ind)
{
    int  i, len = 0;
 
    if (!mstr) return NULL;
 
    if (pos < 0 || pos >= mstr->len) return NULL;
 
    for (i = 0, len = 0; i < (int)mstr->num; i++) {
        if (pos < len + mstr->plen[i]) {
            if (ind) *ind = i;
            return mstr->pbyte[i] + pos - len;
        }
 
        len += mstr->plen[i];
    }
 
    return NULL;
}
 
int mstr_free (mstr_t * mstr)
{
    if (!mstr) return -1;

    if (mstr->alloc)
        kfree(mstr);

    return 0;
}


pat_bitvec_t * pat_bitvec_alloc (void * vpat, int len, int vectype)
{
    pat_bitvec_t  * patvec = NULL;
    uint8         * pat = (uint8 *)vpat;
    int             c;
    int             i;

    if (!pat) return NULL;
    if (len < 0) len = strlen((const char *)pat);
    if (len <= 0) return NULL;

    patvec = kzalloc(sizeof(*patvec));
    if (!patvec) return NULL;

    patvec->pattern = pat;
    patvec->patlen = len;

    if (vectype == 1 || vectype == 2) {
        patvec->vectype = vectype;
        patvec->veclen = 127;

        for (i = 0; i < patvec->veclen; i++) {
            patvec->bitvec[i] = bitarr_alloc(patvec->patlen);
        }

        for (i = 0; i < len; i++) {
            if (patvec->vectype == 2) {
                c = pat[i];
                if (c >= 'A' && c <= 'Z')
                    c = c | 0x20;
                bitarr_set(patvec->bitvec[c], i);
            } else {
                bitarr_set(patvec->bitvec[ pat[i] ], i);
            }
        }

    } else {
        patvec->vectype = 0;
        patvec->veclen = 256;

        for (i = 0; i < patvec->veclen; i++) {
            patvec->bitvec[i] = bitarr_alloc(patvec->patlen);
        }

        for (i = 0; i < len; i++) {
            bitarr_set(patvec->bitvec[ pat[i] ], i);
        }
    }

    return patvec;
}

void pat_bitvec_free (pat_bitvec_t * patvec)
{
    int i;

    if (!patvec) return;

    for (i = 0; i < patvec->veclen; i++) {
        bitarr_free(patvec->bitvec[i]);
        patvec->bitvec[i] = NULL;
    }

    kfree(patvec);
}


void * shift_and_find (void * pbyte, int len, void * pattern, int patlen, void * pvec)
{
    uint8        * pstr = (uint8 *)pbyte;
    uint8        * pat = (uint8 *)pattern;
    pat_bitvec_t * patvec = (pat_bitvec_t *)pvec;
    uint8          alloc = 0;
    int            pos = 0;
    uint8          matchfound = 0;
    bitarr_t     * matchbit = NULL;

    if (!pstr || len <= 0 || !pat || patlen <= 0)
        return NULL;
 
    if (patlen > len) return NULL;

    if (!patvec) {
        patvec = pat_bitvec_alloc(pat, patlen, 0);
        alloc = 1;
    }

    if (!patvec) return NULL;

    matchbit = bitarr_alloc(patlen);
    if (!matchbit) {
        pat_bitvec_free(patvec);
        return NULL;
    }

    for (pos = 0; pos < len; pos++) {
        bitarr_left(matchbit, 1);
        bitarr_set(matchbit, 0);
        bitarr_and(matchbit, patvec->bitvec[ pstr[pos] ]);

        if (bitarr_get(matchbit, patlen - 1) == 1) {
            /* matching success! pos is the location of end char */
            matchfound = 1;
            break;
        }
    }

    bitarr_free(matchbit);
    if (alloc) pat_bitvec_free(patvec);

    if (matchfound)
        return pstr + pos + 1 - patlen;

    return NULL;
}

void * shift_and_find_string (void * pbyte, int len, void * pattern, int patlen, void * pvec)
{
    uint8        * pstr = (uint8 *)pbyte;
    uint8        * pat = (uint8 *)pattern;
    pat_bitvec_t * patvec = (pat_bitvec_t *)pvec;
    uint8          alloc = 0;
    int            pos = 0;
    int            c = 0;
    uint8          matchfound = 0;
    bitarr_t     * matchbit = NULL;

    if (!pstr || len <= 0 || !pat || patlen <= 0)
        return NULL;
 
    if (patlen > len) return NULL;

    if (!patvec) {
        patvec = pat_bitvec_alloc(pat, patlen, 2);
        alloc = 1;
    }

    if (!patvec) return NULL;

    matchbit = bitarr_alloc(patlen);
    if (!matchbit) {
        pat_bitvec_free(patvec);
        return NULL;
    }

    for (pos = 0; pos < len; pos++) {
        bitarr_left(matchbit, 1);
        bitarr_set(matchbit, 0);

        c = pstr[pos];
        if (c >= 'A' && c <= 'Z') c = c | 0x20;
        bitarr_and(matchbit, patvec->bitvec[ c ]);

        if (bitarr_get(matchbit, patlen - 1) == 1) {
            /* matching success! pos is the location of end char */
            matchfound = 1;
            break;
        }
    }

    bitarr_free(matchbit);
    if (alloc) pat_bitvec_free(patvec);

    if (matchfound)
        return pstr + pos + 1 - patlen;

    return NULL;
}



pat_kmpvec_t * pat_kmpvec_alloc (void * vpat, int len, int vectype, int reverse)
{
    pat_kmpvec_t  * patvec = NULL;
    uint8         * pat = (uint8 *)vpat;
    int             i, j;

    if (!pat) return NULL;
    if (len < 0) len = strlen((const char *)pat);
    if (len <= 0) return NULL;

    patvec = kzalloc(sizeof(*patvec));
    if (!patvec) return NULL;

    patvec->pattern = pat;
    patvec->patlen = len;

    patvec->reverse = reverse;

    if (vectype == 1 || vectype == 2) {
        patvec->vectype = vectype;
    } else {
        patvec->vectype = 0;
    }

    patvec->failvec = kzalloc(len * sizeof(int));

    patvec->failvec[0] = -1;

    if (patvec->vectype != 2) { //case censitive

        for (j = 1; j < len; j++) {
            i = patvec->failvec[ j - 1 ];
    
            if (reverse) {
                while (pat[len - 1 - j] != pat[len - 1 - i - 1] && i >= 0)
                    i = patvec->failvec[i];
    
                if (pat[len - 1 - j] == pat[len - 1 - i - 1])
                    patvec->failvec[j] = i + 1;
                else
                    patvec->failvec[j] = -1;
    
            } else {
                while (pat[j] != pat[i + 1] && i >= 0)
                    i = patvec->failvec[i];
    
                if (pat[j] == pat[i + 1])
                    patvec->failvec[j] = i + 1;
                else
                    patvec->failvec[j] = -1;
            }
        }
    
    } else {

        for (j = 1; j < len; j++) {
            i = patvec->failvec[ j - 1 ];
    
            if (reverse) {
                while (adf_tolower(pat[len - 1 - j]) != adf_tolower(pat[len - 1 - i - 1]) && i >= 0)
                    i = patvec->failvec[i];
    
                if (adf_tolower(pat[len - 1 - j]) == adf_tolower(pat[len - 1 - i - 1]))
                    patvec->failvec[j] = i + 1;
                else
                    patvec->failvec[j] = -1;
    
            } else {
                while (adf_tolower(pat[j]) != adf_tolower(pat[i + 1]) && i >= 0)
                    i = patvec->failvec[i];
    
                if (adf_tolower(pat[j]) == adf_tolower(pat[i + 1]))
                    patvec->failvec[j] = i + 1;
                else
                    patvec->failvec[j] = -1;
            }
        }
    
    }
    return patvec;
}

void pat_kmpvec_free (pat_kmpvec_t * patvec)
{
    if (!patvec) return;

    if (patvec->failvec)
        kfree(patvec->failvec);

    kfree(patvec);
}


void * kmp_find_bytes  (void * pbyte, int len, void * pattern, int patlen, void * pvec)
{
    uint8        * pstr = (uint8 *)pbyte;
    uint8        * pat = (uint8 *)pattern;
    pat_kmpvec_t * patvec = (pat_kmpvec_t *)pvec;
    uint8          alloc = 0;
    int            pos = 0;
    int            patpos = 0;
 
    if (!pstr || len <= 0 || !pat || patlen <= 0)
        return NULL;
 
    if (patlen > len) return NULL;
 
    if (!patvec) {
        patvec = pat_kmpvec_alloc(pat, patlen, 0, 0);
        alloc = 1;
    }
 
    if (!patvec) return NULL;

    for (pos = 0, patpos = 0; pos < len && patpos < patlen; ) {

        if (pstr[pos] == pat[patpos]) {
            pos++;
            patpos++;

        } else if (patpos == 0) {
            pos++;

        } else {
            patpos = patvec->failvec[patpos - 1] + 1;
        }
    }

    if (alloc) pat_kmpvec_free(patvec);

    if (patpos < patlen) return NULL;

    return pstr + pos - patlen;
}

void * kmp_rfind_bytes  (void * pbyte, int len, void * pattern, int patlen, void * pvec)
{
    uint8        * pstr = (uint8 *)pbyte;
    uint8        * pat = (uint8 *)pattern;
    pat_kmpvec_t * patvec = (pat_kmpvec_t *)pvec;
    uint8          alloc = 0;
    int            pos = 0;
    int            patpos = 0;
 
    if (!pstr || len <= 0 || !pat || patlen <= 0)
        return NULL;
 
    if (patlen > len) return NULL;
 
    if (!patvec) {
        patvec = pat_kmpvec_alloc(pat, patlen, 0, 1);
        alloc = 1;
    }
 
    if (!patvec) return NULL;

    for (pos = 0, patpos = 0; pos < len && patpos < patlen; ) {

        if (pstr[len - 1 - pos] == pat[patlen - 1 - patpos]) {
            pos++;
            patpos++;

        } else if (patpos == 0) {
            pos++;

        } else {
            patpos = patvec->failvec[patpos - 1] + 1;
        }
    }

    if (alloc) pat_kmpvec_free(patvec);

    if (patpos < patlen) return NULL;

    return pstr + pos - patlen;
}



void * kmp_find_string (void * pbyte, int len, void * pattern, int patlen, void * pvec)
{
    uint8        * pstr = (uint8 *)pbyte;
    uint8        * pat = (uint8 *)pattern;
    pat_kmpvec_t * patvec = (pat_kmpvec_t *)pvec;
    uint8          alloc = 0;
    int            pos = 0;
    int            patpos = 0;
 
    if (!pstr || len <= 0 || !pat || patlen <= 0)
        return NULL;
 
    if (patlen > len) return NULL;
 
    if (!patvec) {
        patvec = pat_kmpvec_alloc(pat, patlen, 2, 0);
        alloc = 1;
    }
 
    if (!patvec) return NULL;

    for (pos = 0, patpos = 0; pos < len && patpos < patlen; ) {

        if (adf_tolower(pstr[pos]) == adf_tolower(pat[patpos])) {
            pos++;
            patpos++;

        } else if (patpos == 0) {
            pos++;

        } else {
            patpos = patvec->failvec[patpos - 1] + 1;
        }
    }

    if (alloc) pat_kmpvec_free(patvec);

    if (patpos < patlen) return NULL;

    return pstr + pos - patlen;
}

void * kmp_rfind_string (void * pbyte, int len, void * pattern, int patlen, void * pvec)
{
    uint8        * pstr = (uint8 *)pbyte;
    uint8        * pat = (uint8 *)pattern;
    pat_kmpvec_t * patvec = (pat_kmpvec_t *)pvec;
    uint8          alloc = 0;
    int            pos = 0;
    int            patpos = 0;
 
    if (!pstr || len <= 0 || !pat || patlen <= 0)
        return NULL;
 
    if (patlen > len) return NULL;
 
    if (!patvec) {
        patvec = pat_kmpvec_alloc(pat, patlen, 2, 1);
        alloc = 1;
    }
 
    if (!patvec) return NULL;

    for (pos = 0, patpos = 0; pos < len && patpos < patlen; ) {

        if (adf_tolower(pstr[len - 1 - pos]) == adf_tolower(pat[patlen - 1 - patpos])) {
            pos++;
            patpos++;

        } else if (patpos == 0) {
            pos++;

        } else {
            patpos = patvec->failvec[patpos - 1] + 1;
        }
    }

    if (alloc) pat_kmpvec_free(patvec);

    if (patpos < patlen) return NULL;

    return pstr + pos - patlen;
}


 
pat_bmvec_t * pat_bmvec_alloc (void * vpat, int len, int vectype)
{
    pat_bmvec_t  * patvec = NULL;
    uint8        * pat = (uint8 *)vpat;
    int          * suffix = NULL;
    int            i, f, g;
 
    if (!pat) return NULL;
    if (len < 0) len = strlen((const char *)pat);
    if (len <= 0) return NULL;
 
    patvec = kzalloc(sizeof(*patvec));
    if (!patvec) return NULL;
 
    patvec->pattern = pat;
    patvec->patlen = len;
 
    if (vectype == 1 || vectype == 2) {
        patvec->vectype = vectype;
    } else {
        patvec->vectype = 0;
    }
 
    patvec->goodsuff = kzalloc(len * sizeof(int));

    suffix = kzalloc(len * sizeof(int));

    if (!patvec->goodsuff || !suffix) {
        pat_bmvec_free(patvec);
        return NULL;
    }
 
    for (i = 0; i < 256; i++) 
        patvec->badch[i] = len;

    if (patvec->vectype != 2) { //case censitive
 
        for (i = 0; i < len - 1; i++) 
            patvec->badch[ pat[i] ] = len - 1 - i;
    
        f = 0; 
        suffix[len - 1] = len;
        g = len - 1;
    
        for (i = len - 2; i >= 0; i--) {
            if (i > g && suffix[i + len - 1 - f] < i - g)
                suffix[i] = suffix[i + len - 1 - f];
            else {
                if (i < g) g = i;
                f = i;
                while (g >= 0 && pat[g] == pat[g + len - 1 - f])
                    g--;
    
                suffix[i] = f - g;
            }
        }

    } else {

        for (i = 0; i < len - 1; i++) 
            patvec->badch[ adf_tolower(pat[i]) ] = len - 1 - i;
    
        f = 0; 
        suffix[len - 1] = len;
        g = len - 1;
    
        for (i = len - 2; i >= 0; i--) {
            if (i > g && suffix[i + len - 1 - f] < i - g)
                suffix[i] = suffix[i + len - 1 - f];
            else {
                if (i < g) g = i;
                f = i;
                while (g >= 0 && adf_tolower(pat[g]) == adf_tolower(pat[g + len - 1 - f]))
                    g--;
    
                suffix[i] = f - g;
            }
        }
    
    }
 
    for (i = 0; i < len; i++) 
        patvec->goodsuff[i] = len;

    for (f = 0, i = len - 1; i >= 0; --i) {
        if (suffix[i] == i + 1) {
            for ( ; f < len - 1 - i; f++) {
                if (patvec->goodsuff[f] == len)
                    patvec->goodsuff[f] = len - 1 - i;
            }
        }
    }

    for (i = 0; i <= len - 2; i++) {
        patvec->goodsuff[ len - 1 - suffix[i] ] = len - 1 - i;
    }

    kfree(suffix);

    return patvec;
}
 
void pat_bmvec_free (pat_bmvec_t * patvec)
{
    if (!patvec) return;
 
    if (patvec->goodsuff)
        kfree(patvec->goodsuff);
 
    kfree(patvec);
}
 
 
void * bm_find_bytes  (void * pbyte, int len, void * pattern, int patlen, void * pvec)
{
    uint8       * pstr = (uint8 *)pbyte;
    uint8       * pat = (uint8 *)pattern;
    pat_bmvec_t * patvec = (pat_bmvec_t *)pvec;
    uint8         alloc = 0;
    int           pos = 0;
    int           i = 0;
    uint8         matchfound = 0;
 
    if (!pstr || len <= 0 || !pat || patlen <= 0)
        return NULL;
 
    if (patlen > len) return NULL;
 
    if (!patvec) {
        patvec = pat_bmvec_alloc(pat, patlen, 0);
        alloc = 1;
    }
 
    if (!patvec) return NULL;

    for (pos = 0; pos < len - patlen; ) {
        for (i = patlen - 1;
             i >= 0 && pat[i] == pstr[i + pos];
             i--);

        if (i < 0) {
            matchfound = 1;
            break;
        } else {
            pos += max( patvec->goodsuff[i], 
                        patvec->badch[ pstr[i + pos] ] - patlen + 1 + i );
        }
    }

    if (alloc) pat_bmvec_free(patvec);
 
    if (matchfound)
        return pstr + pos;
 
    return NULL;
}

 
void * bm_find_string (void * pbyte, int len, void * pattern, int patlen, void * pvec)
{
    uint8       * pstr = (uint8 *)pbyte;
    uint8       * pat = (uint8 *)pattern;
    pat_bmvec_t * patvec = (pat_bmvec_t *)pvec;
    uint8         alloc = 0;
    int           pos = 0;
    int           i = 0;
    uint8         matchfound = 0;
 
    if (!pstr || len <= 0 || !pat || patlen <= 0)
        return NULL;
 
    if (patlen > len) return NULL;
 
    if (!patvec) {
        patvec = pat_bmvec_alloc(pat, patlen, 2);
        alloc = 1;
    }
 
    if (!patvec) return NULL;

    for (pos = 0; pos < len - patlen; ) {
        for (i = patlen - 1;
             i >= 0 && adf_tolower(pat[i]) == adf_tolower(pstr[i + pos]);
             i--);

        if (i < 0) {
            matchfound = 1;
            break;

            /* if going on find next match, pos should shift good suffix 0
            iter += goodsuff[0]; */

        } else {
            pos += max( patvec->goodsuff[i], 
                        patvec->badch[ adf_tolower(pstr[i + pos]) ] - patlen + 1 + i );
        }
    }

    if (alloc) pat_bmvec_free(patvec);
 
    if (matchfound)
        return pstr + pos;
 
    return NULL;
}

int64 bm_find_file (char * fname, int64 offset, void * pattern, int patlen,
                    int nocase, void * pvec, void * poslist)
{
    void        * fbf = NULL;
    int64         fsize = 0;
    uint8       * pat = (uint8 *)pattern;
    pat_bmvec_t * patvec = (pat_bmvec_t *)pvec;
    int64         pos = 0;
    int           fchar = 0;

    uint8         alloc = 0;
    int           i = 0;
    uint8         matchfound = 0;
    int64         matchpos = -1;
 
    if (!fname || !pat || patlen <= 0)
        return -1;
 
    if ((fbf = fbuf_init(fname, 512)) == NULL)
        return -2;

    if (patlen > (fsize = fbuf_size(fbf)) || offset >= fsize) {
        fbuf_free(fbf);
        return -3;
    }
 
    if (!patvec) {
        patvec = pat_bmvec_alloc(pat, patlen, nocase ? 2 : 0);
        alloc = 1;
    }
 
    if (!patvec) {
        fbuf_free(fbf);
        return -4;
    }

    for (pos = offset; pos < fsize - patlen; ) {

        for (i = patlen - 1; i >= 0; i--) {

            fchar = fbuf_at(fbf, i + pos);
            if (fchar < 0) break;

            if (nocase) {
                if (adf_tolower(pat[i]) == adf_tolower(fchar))
                    continue;
                else
                    break;
            } else {
                if (pat[i] == fchar) continue;
                else break;
            }
        }

        if (fchar < 0) break;

        if (i < 0) {
            matchfound = 1;
            if (matchpos < 0) matchpos = pos;

            if (poslist) {
                ar_fifo_push(poslist, (void *)pos);

                /* if going on find next match, pos should shift good suffix 0 */
                pos += patvec->goodsuff[0]; 

            } else 
                break;

        } else {
            pos += max( patvec->goodsuff[i], 
                        patvec->badch[ nocase ? adf_tolower(fchar) : fchar ] - patlen + 1 + i );
        }
    }

    if (alloc) pat_bmvec_free(patvec);
 
    fbuf_free(fbf);

    if (matchfound)
        return matchpos;
 
    return -1;
}

int64 bm_find_filebuf (void * fbf, int64 offset, void * pattern, int patlen,
                    int nocase, void * pvec, void * poslist)
{
    int64         fsize = 0;
    uint8       * pat = (uint8 *)pattern;
    pat_bmvec_t * patvec = (pat_bmvec_t *)pvec;
    int64         pos = 0;
    int           fchar;

    uint8         alloc = 0;
    int           i = 0;
    uint8         matchfound = 0;
    int64         matchpos = -1;
 
    if (!fbf || !pat || patlen <= 0)
        return -1;
 
    if (patlen > (fsize = fbuf_size(fbf)) || offset >= fsize) {
        return -2;
    }
 
    if (!patvec) {
        patvec = pat_bmvec_alloc(pat, patlen, nocase ? 2 : 0);
        alloc = 1;
    }
 
    if (!patvec) {
        return -3;
    }

    for (pos = offset; pos < fsize - patlen; ) {

        for (i = patlen - 1; i >= 0; i--) {
 
            fchar = fbuf_at(fbf, i + pos);
            if (fchar < 0) break;
 
            if (nocase) {
                if (adf_tolower(pat[i]) == adf_tolower(fchar))
                    continue;
                else
                    break;
            } else {
                if (pat[i] == fchar) continue;
                else break;
            }
        }
 
        if (fchar < 0) break; 

        if (i < 0) {
            matchfound = 1;
            if (matchpos < 0) matchpos = pos;

            if (poslist) {
                ar_fifo_push(poslist, (void *)pos);

                /* if going on find next match, pos should shift good suffix 0 */
                pos += patvec->goodsuff[0]; 

            } else 
                break;

        } else {
            pos += max( patvec->goodsuff[i], 
                        patvec->badch[ nocase ? adf_tolower(fchar) : fchar ] - patlen + 1 + i );
        }
    }

    if (alloc) pat_bmvec_free(patvec);
 
    if (matchfound)
        return matchpos;
 
    return -1;
}


pat_sunvec_t * pat_sunvec_alloc (void * vpat, int len, int vectype)
{
    pat_sunvec_t * patvec = NULL;
    uint8        * pat = (uint8 *)vpat;
    int            i;
 
    if (!pat) return NULL;
    if (len < 0) len = strlen((const char *)pat);
    if (len <= 0) return NULL;
 
    patvec = kzalloc(sizeof(*patvec));
    if (!patvec) return NULL;
 
    patvec->pattern = pat;
    patvec->patlen = len;
 
    if (vectype == 1 || vectype == 2) {
        patvec->vectype = vectype;
    } else {
        patvec->vectype = 0;
    }

    for (i = 0; i < 256; i++) {
        patvec->chloc[i] = -1;
    }

    for (i = 0; i < len; i++) {

        /* ignore the char case */
        if (patvec->vectype == 2)
            patvec->chloc[ adf_tolower(pat[i]) ] = i;

        else
            patvec->chloc[ pat[i] ] = i;
    }

    return patvec;
}

void pat_sunvec_free (pat_sunvec_t * patvec)
{
    if (!patvec) return;
 
    kfree(patvec);
}

void * sun_find_bytes (void * pbyte, int len, void * pattern, int patlen, void * pvec)
{
    uint8        * pstr = (uint8 *)pbyte;
    uint8        * pat = (uint8 *)pattern;
    pat_sunvec_t * patvec = (pat_sunvec_t *)pvec;
    uint8          alloc = 0;
    int            pos = 0;
    int            i = 0;
    uint8          matchfound = 0;
 
    if (!pstr || len <= 0 || !pat || patlen <= 0)
        return NULL;
 
    if (patlen > len) return NULL;
 
    if (!patvec) {
        patvec = pat_sunvec_alloc(pat, patlen, 0);
        alloc = 1;
    }
 
    if (!patvec) return NULL;
 
    for (pos = 0; pos <= len - patlen; ) {

        for (i = 0; i < patlen && pat[i] == pstr[i + pos]; i++);

        if (i >= patlen) {
            matchfound = 1;
            break;

        } else {
            pos += patlen - patvec->chloc[ pstr[pos + patlen] ];
        }
    }
 
    if (alloc) pat_sunvec_free(patvec);
 
    if (matchfound)
        return pstr + pos;
 
    return NULL;
}

void * sun_find_string (void * pbyte, int len, void * pattern, int patlen, void * pvec)
{
    uint8        * pstr = (uint8 *)pbyte;
    uint8        * pat = (uint8 *)pattern;
    pat_sunvec_t * patvec = (pat_sunvec_t *)pvec;
    uint8          alloc = 0;
    int            pos = 0;
    int            i = 0;
    uint8          matchfound = 0;
 
    if (!pstr || len <= 0 || !pat || patlen <= 0)
        return NULL;
 
    if (patlen > len) return NULL;
 
    if (!patvec) {
        patvec = pat_sunvec_alloc(pat, patlen, 2);
        alloc = 1;
    }
 
    if (!patvec) return NULL;
 
    for (pos = 0; pos <= len - patlen; ) {

        for (i = 0; i < patlen && adf_tolower(pat[i]) == adf_tolower(pstr[i + pos]); i++);

        if (i >= patlen) {
            matchfound = 1;
            break;

        } else {
            pos += patlen - patvec->chloc[ adf_tolower(pstr[pos + patlen]) ];
        }
    }
 
    if (alloc) pat_sunvec_free(patvec);
 
    if (matchfound)
        return pstr + pos;
 
    return NULL;
}


void * sun_find_mbytes (void ** ppbyte, int * plen, int num, void * pattern, int patlen, void * pvec, int * pind)
{
    mstr_t         mstr;
    uint8        * pat = (uint8 *)pattern;
    pat_sunvec_t * patvec = (pat_sunvec_t *)pvec;
    uint8          alloc = 0;
    int            pos = 0;
    int            i = 0;
    uint8          matchfound = 0;
 
    if (!pat || patlen <= 0)
        return NULL;
 
    mstr_init(&mstr, ppbyte, plen, num);

    if (patlen > mstr.len) return NULL;
 
    if (!patvec) {
        patvec = pat_sunvec_alloc(pat, patlen, 0);
        alloc = 1;
    }
 
    if (!patvec) return NULL;
 
    for (pos = 0; pos <= mstr.len - patlen; ) {
 
        for (i = 0; i < patlen && pat[i] == mstr_char(&mstr, i + pos, NULL); i++);
 
        if (i >= patlen) {
            matchfound = 1;
            break;
 
        } else {
            pos += patlen - patvec->chloc[ mstr_char(&mstr, pos + patlen, NULL) ];
        }
    }
 
    if (alloc) pat_sunvec_free(patvec);
 
    if (matchfound)
        return mstr_p(&mstr, pos, pind);
 
    return NULL;
}

void * sun_find_mstring (void ** ppbyte, int * plen, int num, void * pattern, int patlen, void * pvec, int * pind)
{
    mstr_t         mstr;
    uint8        * pat = (uint8 *)pattern;
    pat_sunvec_t * patvec = (pat_sunvec_t *)pvec;
    uint8          alloc = 0;
    int            pos = 0;
    int            i = 0;
    uint8          matchfound = 0;
 
    if (!pat || patlen <= 0)
        return NULL;
 
    mstr_init(&mstr, ppbyte, plen, num);

    if (patlen > mstr.len) return NULL;
 
    if (!patvec) {
        patvec = pat_sunvec_alloc(pat, patlen, 2);
        alloc = 1;
    }
 
    if (!patvec) return NULL;
 
    for (pos = 0; pos <= mstr.len - patlen; ) {
 
        for (i = 0; i < patlen && adf_tolower(pat[i]) == adf_tolower(mstr_char(&mstr, i + pos, NULL)); i++);
 
        if (i >= patlen) {
            matchfound = 1;
            break;
 
        } else {
            pos += patlen - patvec->chloc[ adf_tolower(mstr_char(&mstr, pos + patlen, NULL)) ];
        }
    }
 
    if (alloc) pat_sunvec_free(patvec);
 
    if (matchfound)
        return mstr_p(&mstr, pos, pind);
 
    return NULL;
}

