
#include <ctype.h>

#include "btype.h"
#include "frame.h"
#include "memory.h"
#include "kemalloc.h"
#include "strutil.h"
#include "mimetype.h"
#include "fileop.h"
#include "nativefile.h"
#include "ssltcp.h"
#include "trace.h"
#include "patmat.h"

#include "http_uri.h"
#include "http_chunk.h"
#include "http_cookie.h"
#include "http_msg.h"

extern void * mimemgmt;
extern void * ckiemgmt;


void * http_msg_alloc ()
{
    http_msg_t * msg = NULL;

    msg = kzalloc(sizeof(*msg));
    if (!msg) return NULL;

    http_req_init(msg);
    http_resp_init(msg);

    return msg;
}

void http_msg_free  (void * vmsg)
{
    http_msg_t * msg = (http_msg_t *)vmsg;

    if (!msg) return;

    http_uri_free(&msg->uri);

    frame_delete(&msg->ckiefrm);

    frame_delete(&msg->req_hdrfrm);
    //frame_delete(&msg->bodyfrm);

    chunk_free(msg->req_chunk);

    frame_delete(&msg->res_hdrfrm);

    http_chunk_free(msg->res_httpchk);

    frame_delete(&msg->resfrm);

    kfree(msg);
}

int http_req_init  (void * vmsg)
{
    http_msg_t * msg = (http_msg_t *)vmsg;

    if (!msg) return -1;

    http_uri_init(&msg->uri);

    msg->ssllink = 0;

    msg->host[0] = '\0';
    msg->port = 0;

    msg->meth = 0;

    if (!msg->ckiefrm) msg->ckiefrm = frame_new(0);
    else frame_empty(msg->ckiefrm);

    msg->useproxy = 0;
    msg->proxy_host[0] = '\0';
    msg->proxy_port = 0;

    msg->req_hdrlen = 0;
    if (!msg->req_hdrfrm) msg->req_hdrfrm = frame_new(256);
    else frame_empty(msg->req_hdrfrm);

    msg->req_bodytype = 0;
    msg->req_mime[0] = '\0';
    msg->req_mimeid = 0;
    msg->req_httpchunk = 0;

    if (!msg->req_chunk) msg->req_chunk = chunk_new(8192);
    else chunk_zero(msg->req_chunk);

    msg->req_sentnum = 0;

    return 0;
}

void * http_req_set_uri   (void * vmsg, char * url, int urllen)
{
    http_msg_t * msg = (http_msg_t *)vmsg;
    http_uri_t * uri = NULL;

    if (!msg) return NULL;

    uri = &msg->uri;

    http_uri_set(uri, url, urllen, 0);

    msg->ssllink = uri->ssl_link;
    msg->req_hdrenc = 0;

    str_secpy(msg->host, sizeof(msg->host)-1, uri->host, uri->hostlen);
    msg->port = uri->port;

    return uri;
}

int http_req_set_meth  (void * vmsg, int meth)
{
    http_msg_t * msg = (http_msg_t *)vmsg;

    if (!msg) return -1;

    if (meth < 0) meth = 0;
    if (meth > 8) meth = 0;

    msg->meth = meth;
    msg->req_hdrenc = 0;

    return 0;
}

int http_req_set_proxy (void * vmsg, char * proxy, int proxylen)
{
    http_msg_t * msg = (http_msg_t *)vmsg;
    char * pbgn = NULL;
    char * pend = NULL;
    char * poct = NULL;

    if (!msg) return -1;

    msg->req_hdrenc = 0;

    if (proxy && proxylen < 0) proxylen = str_len(proxy);
    if (!proxy || proxylen <= 0) {
        msg->useproxy = 0;
        msg->proxy_host[0] = '\0';
        msg->proxy_port = 0;
        return 0;
    }

    pbgn = proxy; pend = pbgn + proxylen;
    pbgn = skipOver(pbgn, pend-pbgn, " \t\r\n", 4);
    poct = skipTo(pbgn, pend-pbgn, ": \t,;", 5);
    if (poct > pbgn) {
        str_secpy(msg->proxy_host, sizeof(msg->proxy_host)-1, pbgn, poct-pbgn);
        msg->useproxy = 1;
    } else {
        msg->proxy_host[0] = '\0';
        msg->useproxy = 0;
    }

    pbgn = skipOver(poct, pend-poct, ": \t,;", 5);
    if (pbgn >= pend || !isdigit(*pbgn)) msg->proxy_port = 80;
    else str_atoi(pbgn, pend-pbgn, &msg->proxy_port);

    return 1;
}

int http_req_clear_body  (void * vmsg)
{
    http_msg_t * msg = (http_msg_t *)vmsg;

    if (!msg) return -1;

    msg->req_bodytype = 0;
    chunk_zero(msg->req_chunk);

    return 0;
}


int http_req_set_body  (void * vmsg, uint8 bodytype, char * pbyte, int bytelen)
{
    http_msg_t * msg = (http_msg_t *)vmsg;

    if (!msg) return -1;
    if (!pbyte) return -2;
    if (bytelen < 0) bytelen = str_len(pbyte);
    if (bytelen <= 0) return -3;

    msg->req_bodytype = bodytype;

    switch (bodytype) {
    case 0:
        return 0;

    case 1: //butptr
        return chunk_add_bufptr(msg->req_chunk, pbyte, bytelen, NULL, NULL);

    case 2: //buffer
        return chunk_add_buffer(msg->req_chunk, pbyte, bytelen);

    case 3: //filename
        file_mime_type(mimemgmt, (char*)pbyte, msg->req_mime, &msg->req_mimeid, NULL);
        return chunk_add_file(msg->req_chunk, pbyte, 0, -1, 0);

    case 4: //fp = fopen
        return chunk_add_filefp(msg->req_chunk, (FILE *)pbyte, 0, -1);

    case 5: //fd = open
        return chunk_add_filefd(msg->req_chunk, (int)(long)pbyte, 0, -1);

    default:
        return -20;
    }

    return -21;
}

int http_req_set_contype (void * vmsg, char * ctype, int ctypelen)
{
    http_msg_t * msg = (http_msg_t *)vmsg;

    if (!msg) return -1;

    if (!ctype) return -2;
    if (ctypelen < 0) ctypelen = str_len(ctype);
    if (ctypelen <= 0) return -3;

    str_secpy(msg->req_mime, sizeof(msg->req_mime)-1, ctype, ctypelen);

    mime_type_get_by_mime(mimemgmt, msg->req_mime, NULL, &msg->req_mimeid, NULL);

    msg->req_hdrenc = 0;
    return 0;
}

int http_req_set_transenc (void * vmsg, int httpchunk)
{
    http_msg_t * msg = (http_msg_t *)vmsg;

    if (!msg) return -1;

    msg->req_httpchunk = httpchunk > 0 ? 1 : 0;

    return 0;
}


int http_req_encode (void * vmsg)
{
    http_msg_t * msg = (http_msg_t *)vmsg;
    static char * s_meth[9] = { "GET", "POST", "PUT", "OPTION", "CONNECT", "DELETE", "HEAD", "TRACE", ""};

    if (!msg) return -1;

    //if (msg->req_hdrenc) return 0;

    if (!msg->req_hdrfrm) msg->req_hdrfrm = frame_new(256);
    else frame_empty(msg->req_hdrfrm);

    if (msg->meth == 4) { //CONNECT
        frame_appendf(msg->req_hdrfrm, "%s %s HTTP/1.1\r\n", s_meth[msg->meth], frameP(msg->uri.uri));
    } else if (msg->useproxy) {
        frame_appendf(msg->req_hdrfrm, "%s %s HTTP/1.1\r\n", s_meth[msg->meth], frameP(msg->uri.uri));
    } else {
        frame_appendf(msg->req_hdrfrm, "%s %s HTTP/1.1\r\n", s_meth[msg->meth], msg->uri.path);
    }

    /* HTTP request headers */
    frame_append(msg->req_hdrfrm, "Accept: */*\r\n");
    //frame_append(msg->req_hdrfrm, "Accept-Encoding: gzip, deflate\r\n");
    frame_append(msg->req_hdrfrm, "Cache-control: no-cache\r\n");
    frame_append(msg->req_hdrfrm, "Connection: keep-alive\r\n");

    if (msg->req_bodytype > 0) {
        frame_appendf(msg->req_hdrfrm, "Content-Type: %s\r\n", msg->req_mime);
        if (msg->req_httpchunk)
            frame_append(msg->req_hdrfrm, "Transfer-Encoding: chunked\r\n");
        else
            frame_appendf(msg->req_hdrfrm, "Content-Length: %ld\r\n", (long)chunk_size(msg->req_chunk, 0));
    }

    if (frameL(msg->ckiefrm) > 0)
        frame_appendf(msg->req_hdrfrm, "Cookie: %s\r\n", frameS(msg->ckiefrm));

    if (msg->useproxy)
        frame_appendf(msg->req_hdrfrm, "Host: %s:%d\r\n", msg->proxy_host, msg->proxy_port);
    else
        frame_appendf(msg->req_hdrfrm, "Host: %s:%d\r\n", msg->host, msg->port);

    frame_append(msg->req_hdrfrm, "Pragma: no-cache\r\n");
    frame_append(msg->req_hdrfrm, "User-Agent: Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/70.0.3538.77 Safari/537.36\r\n");
    frame_append(msg->req_hdrfrm, "\r\n");

    msg->req_hdrenc = 1;
    msg->req_hdrlen = frameL(msg->req_hdrfrm);
    msg->req_sentnum = 0;

    chunk_prepend_bufptr(msg->req_chunk, frameP(msg->req_hdrfrm), frameL(msg->req_hdrfrm), NULL, NULL, 1);

    chunk_set_end(msg->req_chunk);

    return 1;
}


int http_req_send (void * vmsg, void * vssltcp)
{
    http_msg_t  * msg = (http_msg_t *)vmsg;
    ssl_tcp_t   * ssltcp = (ssl_tcp_t *)vssltcp;
    void        * chunk = NULL;
    chunk_vec_t   iovec;

    uint8         httpchunk = 0;
    int64         filepos = 0;
    int           sentnum = 0;
    int           ret = 0;
    int           num = 0;
    int           err = 0;

    if (!msg) return -1;
    if (!ssltcp) return -2;

    httpchunk = msg->req_httpchunk;
    chunk = msg->req_chunk;
    filepos = msg->req_sentnum;

    if (chunk_has_file(chunk) > 0) {
        sock_nodelay_unset(ssltcp->fd);
        sock_nopush_set(ssltcp->fd);
    }

    for (sentnum = 0; chunk_get_end(chunk, filepos, httpchunk) == 0; ) {

        memset(&iovec, 0, sizeof(iovec));
        ret = chunk_vec_get(chunk, filepos, &iovec, httpchunk);
        if (ret < 0 || (iovec.size > 0 && iovec.vectype != 1 && iovec.vectype != 2)) {
            return ret;
        }

        if (iovec.size == 0) {
            /* no available data to send, waiting for more data... */
            return 0;
        }

        if (iovec.vectype == 2) { //sendfile
            ret = ssl_tcp_sendfile(ssltcp, iovec.filefd, iovec.fpos, iovec.size , &num, &err);
            if (ret < 0) return ret;

        } else if (iovec.vectype == 1) { //mem buffer, writev
            ret = ssl_tcp_writev(ssltcp, iovec.iovs, iovec.iovcnt, &num, &err);
            if (ret < 0) return ret;
        }

        filepos += num;
        sentnum += num;

        msg->req_sentnum += num;

#ifdef UNIX
        if (err == EINTR || err == EAGAIN || err == EWOULDBLOCK) { //EAGAIN
#elif defined(_WIN32) || defined(_WIN64)
        if (err == WSAEWOULDBLOCK) {
#else
        if (num == 0) {
#endif
            sock_write_ready(ssltcp->fd, 1000);
            continue;
        }
    }

    if (chunk_get_end(chunk, msg->req_sentnum, httpchunk) == 1) {
        return sentnum;
    }

    return 0;
}



int http_resp_init  (void * vmsg)
{
    http_msg_t * msg = (http_msg_t *)vmsg;

    if (!msg) return -1;

    msg->res_hdrgot = 0;
    msg->res_hdrlen = 0;
    if (!msg->res_hdrfrm) msg->res_hdrfrm = frame_new(0);
    else frame_empty(msg->res_hdrfrm);

    msg->res_ver[0] = '\0';
    msg->res_status = 0;

    msg->res_mime[0] = '\0';
    msg->res_mimeid = 0;
    msg->res_ext[0] = '\0';

    msg->res_contenc = 0;
    msg->res_keepalive = 0;

    msg->res_length = 0;

    msg->res_loc = NULL;
    msg->res_loclen = 0;

    msg->res_file[0] = '\0';

    if (msg->res_httpchk) http_chunk_zero(msg->res_httpchk);
    else msg->res_httpchk = http_chunk_alloc(0, NULL);

    if (!msg->resfrm) msg->resfrm = frame_new(0);
    else frame_empty(msg->resfrm);

    return 0;
}

int http_resp_decode (void * vmsg, char * pbyte, int bytelen)
{
    HTTPMsg   * msg = (HTTPMsg *)vmsg;
    HTTPUri   * uri = NULL;
    char      * pbgn = NULL;
    char      * pend = NULL;
    char      * poct = NULL;
    char      * pval = NULL;
    char      * ptmp = NULL;

    char      * name = NULL;
    char      * value = NULL;
    int         namelen = 0;
    int         valuelen = 0;
    int         hdrnum = 0;
    int         times = 0;

    if (!msg) return -1;

    if (!pbyte) return -2;
    if (bytelen < 0) bytelen = str_len(pbyte);
    if (bytelen <= 0) return -3;

    uri = &msg->uri;

    pbgn = pbyte; pend = pbgn + bytelen;

    pbgn = skipOver(pbgn, pend-pbgn, " \t\r\n", 4);
    if (!pbgn || pbgn >= pend) return -100;

    /* parse Status line: HTTP-Version SP Status-Code SP Reason-Phrase CRLF */
    poct = memchr(pbgn, '\n', pend-pbgn);
    if (!poct) return -101;

    pval = skipTo(pbgn, poct-pbgn, " \t\r", 3);
    if (!pval) return -102;
    str_secpy(msg->res_ver, sizeof(msg->res_ver)-1, pbgn, pval-pbgn);

    pbgn = skipOver(pval, poct-pval, " \t\r\n", 4);
    if (pbgn >= poct) return -103;
    pval = skipTo(pbgn, poct-pbgn, " \t\r", 3);
    str_atoi(pbgn, pval-pbgn, &msg->res_status);

    /* now parse the response header */
    for (pbgn = poct; pbgn < pend; pbgn = poct) {
        pbgn = skipOver(poct, pend-poct, " \t\r\n", 4);
        if (pbgn >= pend) break;

        poct = memchr(pbgn, '\n', pend-pbgn);
        if (!poct) break;

        pval = skipTo(pbgn, poct-pbgn, ":", 1);
        if (pval >= pend) continue;
        ptmp = rskipOver(pval-1, pval-pbgn, " \t", 2);
        if (ptmp < pbgn) continue;

        name = pbgn; namelen = ptmp - pbgn + 1;
        hdrnum++;

        pbgn = skipOver(pval+1, pend-pval-1, " \t\r", 3);
        if (pbgn > poct) continue;
        ptmp = rskipOver(poct-1, poct-pbgn, " \t\r", 3);
        if (ptmp < pbgn) continue;

        value = pbgn; valuelen = ptmp - pbgn + 1;

        if (namelen == 12 && str_ncasecmp(name, "Content-Type", namelen) == 0) {
            pval = skipTo(value, valuelen, "\r\n \t,;", 6);
            str_secpy(msg->res_mime, sizeof(msg->res_mime)-1, value, pval-value);

            mime_type_get_by_mime(mimemgmt, msg->res_mime, &pval, &msg->res_mimeid, NULL);
            if (pval) str_ncpy(msg->res_ext, pval, sizeof(msg->res_ext)-1);

        } else if (namelen == 17 && str_ncasecmp(name, "Transfer-Encoding", namelen) == 0) {
            if (valuelen == 7 && str_ncasecmp(value, "chunked", valuelen) == 0)
                msg->res_contenc = 1;
            else
                msg->res_contenc = 0;

        } else if (namelen == 8 && str_ncasecmp(name, "Location", namelen) == 0) {
            msg->res_loc = value;
            msg->res_loclen = valuelen;

        } else if (namelen == 10 && str_ncasecmp(name, "Set-Cookie", namelen) == 0) {
            http_set_cookie_parse(ckiemgmt, msg, value, valuelen);

        } else if (namelen == 14 && str_ncasecmp(name, "Content-Length", namelen) == 0) {
            msg->res_contenc = 0;
            str_atol(value, valuelen, &msg->res_length);

        } else if ((namelen == 10 && str_ncasecmp(name, "Connection", namelen) == 0) || 
                   (namelen == 16 && str_ncasecmp(name, "Proxy-Connection", namelen) == 0))
        {
            if (valuelen == 10 && str_ncasecmp(value, "keep-alive", valuelen) == 0)
                msg->res_keepalive = 1;
            else
                msg->res_keepalive = 0;
        }
    }

    if (uri->filelen <= 0) {
        sprintf(msg->res_file, "index%s", msg->res_ext); 
    } else {
        str_secpy(msg->res_file, sizeof(msg->res_file)-4, uri->file, uri->filelen);
        if (uri->file_extlen <= 0)
            str_cat(msg->res_file, msg->res_ext);
    }

    if (uri->query && uri->querylen > 0) {
        pval = skipTo(uri->query, uri->querylen, " &", 2);
        
        namelen = strlen(msg->res_file);
        str_secpy(msg->res_file + namelen, sizeof(msg->res_file)-4-namelen, uri->query, pval-uri->query);
    }

    namelen = strlen(msg->res_file);
    for (times = 1; ; times++) {
        if (!file_exist(msg->res_file)) break;

        snprintf(msg->res_file + namelen, sizeof(msg->res_file)-1-namelen, ".%d", times);
    }

    return hdrnum;
}


int http_resp_recv (void * vmsg, void * vssltcp, int * crashed)
{
    HTTPMsg      * msg = (HTTPMsg *)vmsg;
    ssl_tcp_t    * ssltcp = (ssl_tcp_t *)vssltcp;
    int            ret, ionum = 0;
    int            readlen = 0;
    int            bodylen = 0;
    http_chunk_t * chunk = NULL;
    char         * p = NULL;
    void         * hfile = NULL;
    int            closed = 0;

    if (!msg) return -1;
    if (!ssltcp) return -2;

    if (crashed) *crashed = 0;
    chunk = msg->res_httpchk;

    bodylen = 0;
    while (!closed) {
        sock_read_ready(ssltcp->fd, 1000);

        //ret = frame_tcp_nbzc_recv(msg->resfrm, fd, &ionum, NULL);
        ret = ssl_tcp_read(ssltcp, msg->resfrm, &ionum, NULL);
        if (ret < 0) {
            if (crashed) *crashed = 1;
            closed = 1;

            if (ionum <= 0) {
                if (hfile) native_file_close(hfile);
                tolog(1, "Receive HTTP response failed %d\n", ret);
                return ret;;
            }
        }

        if (!msg->res_hdrgot) {
            p = sun_find_bytes(frameP(msg->resfrm), frameL(msg->resfrm), "\r\n\r\n", 4, NULL);
            if (!p) {
                if (frameL(msg->resfrm) >= 16000) {
                    tolog(1, "HTTP response header is too big %d\n", frameL(msg->resfrm));
                    return -100;
                }
                continue;
            }

            msg->res_hdrlen = p + 2 - (char *)frameP(msg->resfrm);
            frame_put_nlast(msg->res_hdrfrm, frameP(msg->resfrm), msg->res_hdrlen);

            frame_del_first(msg->resfrm, msg->res_hdrlen + 2);

            ret = http_resp_decode(msg, frameP(msg->res_hdrfrm), msg->res_hdrlen);
            if (ret < 0) {
                tolog(1, "HTTP response header format error ret=%d\n", ret);
                //printf("%s", frameS(msg->res_hdrfrm));
                return ret;
            }

            msg->res_hdrgot = 1;

            if (msg->res_length > 0 || msg->res_contenc == 1) {
                hfile = native_file_open(msg->res_file, NF_WRITEPLUS);
                if (!hfile) {
                    tolog(1, "Could not create response file %s\n", msg->res_file);
                    return -101;
                }

                //native_file_write(hfile, frameP(msg->res_hdrfrm), msg->res_hdrlen);
            }

            //printf("%s\r\n", (char *)frameS(msg->res_hdrfrm));
        }

        if (frameL(msg->resfrm) > 0) {
            if (msg->res_contenc == 1) {
                http_chunk_add_bufptr(chunk, frameP(msg->resfrm), frameL(msg->resfrm), &readlen);
                bodylen += chunk_rest_size(http_chunk_obj(chunk), 0);
                msg->res_length += chunk_rest_size(http_chunk_obj(chunk), 0);

                chunk_write_file(http_chunk_obj(chunk), native_file_fd(hfile), 0, -1, 0); //write encoded body content
                //native_file_write(hfile, frameP(msg->resfrm), readlen);  //write un-encoded body-content

                chunk_remove(http_chunk_obj(chunk), bodylen, 0);
            } else if (msg->res_length > 0) {
                readlen = msg->res_length - bodylen;
                if (readlen > frameL(msg->resfrm)) readlen = frameL(msg->resfrm);
                bodylen += readlen;

                native_file_write(hfile, frameP(msg->resfrm), readlen);
            } else {
                readlen = frameL(msg->resfrm);
            }

            frame_del_first(msg->resfrm, readlen);
        }

        if (msg->res_contenc == 0) {
            if (bodylen >= msg->res_length) break;
        } else {
            if (chunk->gotall) break;
        }
    }

    if (hfile) native_file_close(hfile);

    return bodylen;
}

