
#include "adifall.ext"
#include "http_chunk.h"


void * http_buf_alloc (int alloctype, void * mpool)
{
    HTTPBuf  * pbuf = NULL;

    pbuf = k_mem_zalloc(sizeof(*pbuf), alloctype, mpool);
    if (!pbuf) {
        return NULL;
    }

    pbuf->alloctype = alloctype;
    pbuf->mpool = mpool;

    return pbuf;
}

void http_buf_free (void * vbuf)
{
    HTTPBuf  * pbuf = (HTTPBuf *)vbuf;

    if (!pbuf) return;

    if (pbuf->alloc) {
        k_mem_free(pbuf->pbgn, pbuf->alloctype, pbuf->mpool);
    }

    k_mem_free(pbuf, pbuf->alloctype, pbuf->mpool);
}

void * http_buf_dup (void * vbuf)
{
    HTTPBuf  * pbuf = (HTTPBuf *)vbuf;
    HTTPBuf  * dup = NULL;

    if (!pbuf) return NULL;

    dup = http_buf_alloc(pbuf->alloctype, pbuf->mpool);

    dup->pbgn = pbuf->pbgn;
    dup->len = pbuf->len;

    dup->body_bgn = pbuf->body_bgn;
    dup->body_len = pbuf->body_len;

    dup->alloc = 0;

    dup->alloctype = pbuf->alloctype;
    dup->mpool = pbuf->mpool;

    return dup;
}


void * http_chunk_item_alloc (void * vchk)
{
    HTTPChunk     * chk = (HTTPChunk *)vchk;
    HTTPChunkItem * item = NULL;

    if (!chk) return NULL;

    item = k_mem_zalloc(sizeof(*item), chk->alloctype, chk->mpool);
    if (!item) return NULL;

    item->chk = chk;

    return item;
}

void http_chunk_item_free (void * vitem)
{
    HTTPChunkItem * item = (HTTPChunkItem *)vitem;
    HTTPChunk     * chk = NULL;

    if (!item) return;

    chk = (HTTPChunk *)item->chk;
    if (!chk) return;

    k_mem_free(item, chk->alloctype, chk->mpool);
}

void * http_chunk_item_dup (void * vitem)
{
    HTTPChunkItem * item = (HTTPChunkItem *)vitem;
    HTTPChunkItem * dup = NULL;

    if (!item) return NULL;

    dup = http_chunk_item_alloc(item->chk);

    dup->chksize = item->chksize;
    dup->chklen = item->chklen;

    dup->recvsize = item->recvsize;
    dup->recvlen = item->recvlen;

    dup->gotall = item->gotall;

    dup->chk = item->chk;

    return dup;
}



void * http_chunk_alloc (int alloctype, void * mpool)
{
    HTTPChunk  * chk = NULL;

    chk = k_mem_zalloc(sizeof(*chk), alloctype, mpool);
    if (!chk) return NULL;

    chk->alloctype = alloctype;
    chk->mpool = mpool;

    chk->item_list = arr_alloc(4, alloctype, mpool);

    chk->chunk = chunk_alloc(16384, alloctype, mpool);

    return chk;
}

int http_chunk_zero (void * vchk)
{
    HTTPChunk  * chk = (HTTPChunk *)vchk;
    int          i, num;

    if (!chk) return -1;

    chk->gotall = 0;
    chk->gotallbody = 0;

    chk->chksize = 0;
    chk->chklen = 0;
    chk->recvsize = 0;
    chk->recvlen = 0;
    chk->chknum = 0;

    if (chk->curitem) {
        http_chunk_item_free(chk->curitem);
        chk->curitem = NULL;
    }

    num = arr_num(chk->item_list);
    for (i = 0; i < num; i++) {
        http_chunk_item_free(arr_value(chk->item_list, i));
    }
    arr_zero(chk->item_list);

    chk->enthdrsize = 0;
    if (chk->enthdr) {
        http_buf_free(chk->enthdr);
        chk->enthdr = NULL;
    }

    chunk_zero(chk->chunk);

    return 0;
}

void http_chunk_free (void * vchk)
{
    HTTPChunk     * chk = (HTTPChunk *)vchk;
    HTTPChunkItem * item = NULL;
    int             i, num;

    if (!chk) return;

    if (chk->curitem) {
        http_chunk_item_free(chk->curitem);
        chk->curitem = NULL;
    }

    num = arr_num(chk->item_list);

    for (i = 0; i < num; i++) {
        item = arr_value(chk->item_list, i);
        http_chunk_item_free(item);
    }

    arr_free(chk->item_list);

    http_buf_free(chk->enthdr);

    chunk_free(chk->chunk);

    k_mem_free(chk, chk->alloctype, chk->mpool);
}
 
chunk_t * http_chunk_obj (void * vchk)
{
    HTTPChunk * chk = (HTTPChunk *)vchk;

    if (!chk) return NULL;

    return chk->chunk;
}


int http_chunk_gotall (void * vchk)
{
    HTTPChunk * chk = (HTTPChunk *)vchk;

    if (!chk) return 0;

    return chk->gotall;
}

void * http_chunk_dup (void * vchk)
{
    HTTPChunk     * chk = (HTTPChunk *)vchk;
    HTTPChunk     * dup = NULL;
    HTTPChunkItem * item = NULL;
    int             i, num;

    dup = http_chunk_alloc(chk->alloctype, chk->mpool);

    if (!chk) return dup;

    dup->gotall = chk->gotall;
    dup->gotallbody = chk->gotallbody;

    dup->chksize = chk->chksize;
    dup->chklen = chk->chklen;
    dup->recvsize = chk->recvsize;
    dup->recvlen = chk->recvlen;
    dup->chknum = chk->chknum;

    if (chk->curitem) {
        dup->curitem = http_chunk_item_dup(chk->curitem);
    } else {
        dup->curitem = NULL;
    }

    num = arr_num(chk->item_list);
    for (i = 0; i < num; i++) {
        item = http_chunk_item_dup(arr_value(chk->item_list, i));
        arr_push(dup->item_list, item);
    }

    dup->enthdrsize = chk->enthdrsize;
    dup->enthdr = http_buf_dup(chk->enthdr);

    return dup;
}



/* return value:
        < 0, error
        = 0, waiting more data
        > 0, parse successfully
 */
int http_chunk_add_bufptr (void * vchk, void * vbgn, int len, int * rmlen)
{
    HTTPChunk     * chk = (HTTPChunk *)vchk;
    HTTPChunkItem * item = NULL;
    HTTPBuf       * pbuf = NULL;
    int             restnum = 0;
    int64           chksizelen = 0;
    int64           chkbodylen = 0;
    int64           restlen = 0;
    uint8         * pbgn = (uint8 *)vbgn;
    uint8         * poct = NULL;
    uint8         * pcrlf = NULL;
    uint8         * pend = NULL;

    if (!chk) return -1;

    pend = pbgn + len;

    while (!chk->gotallbody && pbgn < pend) {

        if ( (item = chk->curitem) == NULL ) {
            chkbodylen = strtoll((char *)pbgn, (char **)&poct, 16);
    
            pcrlf = sun_find_bytes(pbgn, pend - pbgn, "\r\n", 2, NULL);
            if (!pcrlf) {
                /* lack bytes to form chunk-size line, waiting for more data */
                if (rmlen) *rmlen = pbgn - (uint8 *)vbgn;
                return 0;
            }
    
            if (poct != pcrlf) {
                /* there is irrelavent bytes intervened between chunk-size and \r\n */
            }
    
            poct = pcrlf + 2;
            chksizelen = poct - pbgn;
    
            item = chk->curitem = http_chunk_item_alloc(chk);
            if (!item) return -10;
    
            if (chkbodylen > 0)
                item->chksize = chksizelen + chkbodylen + 2;
            else
                item->chksize = chksizelen + chkbodylen;
            item->chklen = chkbodylen;

            item->recvsize = chksizelen;
            item->recvlen = 0;
    
            chk->chksize += item->chksize;
            chk->chklen += item->chklen;
            chk->recvsize += chksizelen;
            chk->chknum++;

/*#ifdef _DEBUG
if (pbgn >= (uint8 *)vbgn + 16) {
    printf("##chunk_add_bufptr: new entity-item begin: entity-len=%lld buflen=%d "
           "entitypos=%ld bufremaining=%ld, chunksize=%lld chunklen=%lld chunkrcvlen=%lld "
           "chunknum=%d.  pbgn-16 bytes:\n",
           chkbodylen, len, pbgn-(uint8 *)vbgn, pend-pbgn,
           chk->chksize, chk->chklen, chk->recvsize, chk->chknum);
    printOctet(stdout, pbgn - 16, 0, pend-pbgn+16 >= 64 ? 64 : pend-pbgn+16, 4);
} else {
    printf("##chunk_add_bufptr: new entity-item begin: entity-len=%lld buflen=%d "
           "entitypos=%ld bufremaining=%ld, chunksize=%lld chunklen=%lld chunkrcvlen=%lld "
           "chunknum=%d.  pbgn bytes:\n",
           chkbodylen, len, pbgn-(uint8 *)vbgn, pend-pbgn,
           chk->chksize, chk->chklen, chk->recvsize, chk->chknum);
    printOctet(stdout, pbgn, 0, pend-pbgn >= 64 ? 64 : pend-pbgn, 4);
}
#endif*/

            if (chkbodylen == 0) {
                /* 0\r\n
                 * \r\n */
                arr_push(chk->item_list, item);
                chk->curitem = NULL;
 
                chk->gotallbody = 1;
                chunk_set_end(chk->chunk);

                pbgn = poct;

                break;
            }

        } else {
            poct = pbgn;
        }
 
        restlen = item->chksize - item->recvsize;
        restnum = pend - poct;

        if (restnum >= restlen) {
            item->gotall = 1;
            item->recvsize += restlen; 
            item->recvlen += restlen - 2;  //trailer \r\n should be detracted

            chk->recvsize += restlen;
            chk->recvlen += restlen - 2;

            chunk_add_bufptr(chk->chunk, poct, restlen - 2, NULL, NULL);

            arr_push(chk->item_list, item);
            chk->curitem = NULL;

            pbgn = poct + restlen;

        } else {
            item->gotall = 0;

            item->recvsize += restnum;
            chk->recvsize += restnum;

            item->recvlen += min(restnum, restlen);
            chk->recvlen += min(restnum, restlen);

            chunk_add_bufptr(chk->chunk, poct, min(restnum, restlen), NULL, NULL);
 
            if (rmlen) *rmlen = len;

            return 0; //waiting for more data 
        }

    } //end while

    if (chk->gotallbody && !chk->gotall) {

        if (pend - pbgn >= 2 && pbgn[0] == '\r' && pbgn[1] == '\n') {
            poct = pbgn + 2;

        } else {
            pcrlf = sun_find_bytes(pbgn, pend - pbgn, "\r\n\r\n", 4, NULL);
            if (!pcrlf) {
/*#ifdef _DEBUG
printf("###find entity header: no 0D0A \n");
printOctet(stdout, pbgn, 0, pend-pbgn>=16?16:pend-pbgn, 4);
#endif*/
                /* lack bytes to form chunk-size line, waiting for more data */
                if (rmlen) *rmlen = pbgn - (uint8 *)vbgn;
                return 0;
            }
            poct = pcrlf + 4;
        }

        chk->chksize += poct - pbgn;
        chk->recvsize += poct - pbgn;

        chk->enthdrsize = poct - pbgn;

        chk->enthdr = pbuf = http_buf_alloc(chk->alloctype, chk->mpool);
        
        pbuf->pbgn = pbgn;
        pbuf->len = poct - pbgn;

        pbuf->body_bgn = pbgn;
        pbuf->body_len = poct - pbgn;
 
        if (rmlen) *rmlen = poct - (uint8 *)vbgn;

        chk->gotall = 1;
        chunk_set_end(chk->chunk);

/*#ifdef _DEBUG
printf("###found entity header: found 0D0A or 0D0A0D0A parse SUCC! restlen=%ld\n", pend-pbgn);
printf("    gotall=%d gotallbody=%d chksize=%lld chklen=%lld rcvsize=%lld"
       " rcvlen=%lld chknum=%d enthdrsize=%d\n\n",
       chk->gotall, chk->gotallbody, chk->chksize, chk->chklen,
       chk->recvsize, chk->recvlen, chk->chknum, chk->enthdrsize);
printOctet(stdout, pbgn, 0, pend-pbgn>=256?256:pend-pbgn, 4);
#endif*/

        return 1;
    }

    if (rmlen) *rmlen = pbgn - (uint8 *)vbgn;

    /* why reached here? */
    return 1;
}


