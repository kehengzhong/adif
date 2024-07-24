/*
 * Copyright (c) 2003-2024 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 *
 * #####################################################
 * #                       _oo0oo_                     #
 * #                      o8888888o                    #
 * #                      88" . "88                    #
 * #                      (| -_- |)                    #
 * #                      0\  =  /0                    #
 * #                    ___/`---'\___                  #
 * #                  .' \\|     |// '.                #
 * #                 / \\|||  :  |||// \               #
 * #                / _||||| -:- |||||- \              #
 * #               |   | \\\  -  /// |   |             #
 * #               | \_|  ''\---/''  |_/ |             #
 * #               \  .-\__  '-'  ___/-. /             #
 * #             ___'. .'  /--.--\  `. .'___           #
 * #          ."" '<  `.___\_<|>_/___.'  >' "" .       #
 * #         | | :  `- \`.;`\ _ /`;.`/ -`  : | |       #
 * #         \  \ `_.   \_ __\ /__ _/   .-` /  /       #
 * #     =====`-.____`.___ \_____/___.-`___.-'=====    #
 * #                       `=---='                     #
 * #     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   #
 * #               佛力加持      佛光普照              #
 * #  Buddha's power blessing, Buddha's light shining  #
 * #####################################################
 */ 

#ifdef HAVE_OPENSSL

#include "btype.h"
#include "memory.h"
#include "fileop.h"
#include "trace.h"
#include "tsock.h"
#include "frame.h"
#include "ssltcp.h"

int ssl_tcp_index = 0;

int ssl_tcp_init ()
{
    if (!SSL_library_init ()) {
        tolog(1, "ssl_tcp_init: OpenSSL library init failed\n");
        return -1;
    }

    SSL_load_error_strings();
    ERR_load_BIO_strings();

    OpenSSL_add_ssl_algorithms();

    ssl_tcp_index = SSL_get_ex_new_index(0, NULL, NULL, NULL, NULL);
    if (ssl_tcp_index == -1) {
        tolog(1, "ssl_tcp_init: OpenSSL SSL_get_ex_new_index() failed\n");
        return -2;
    }

    return 0;
}

void * ssl_server_ctx_init (char * cert, char * prikey, char * cacert)
{
    SSL_CTX     * ctx = NULL;
    struct stat   stcert;
    struct stat   stkey;
    struct stat   stca;

    if (!cert || file_stat(cert, &stcert) < 0)
        return NULL;

    if (!prikey || file_stat(prikey, &stkey) < 0)
        return NULL;

    ctx = SSL_CTX_new(SSLv23_server_method());
    if (!ctx) return NULL;

    /* load certificate and private key, verify the cert with private key */
    if (SSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM) <= 0) {
        tolog(1, "ssl_server_ctx_init: loading Certificate file %s failed\n", cert);
        SSL_CTX_free(ctx);
        return NULL;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, prikey, SSL_FILETYPE_PEM) <= 0) {
        tolog(1, "ssl_server_ctx_init: loading Private Key file %s failed\n", prikey);
        SSL_CTX_free(ctx);
        return NULL;
    }

    if (!SSL_CTX_check_private_key(ctx)) {
        tolog(1, "ssl_server_ctx_init: Certificate verify failed! Private Key %s DOES NOT "
                 "match Certificate %s\n", cert, prikey);
        SSL_CTX_free(ctx);
        return NULL;
    }

    if (cacert && file_stat(cacert, &stca) >= 0) {
        if (SSL_CTX_load_verify_locations(ctx, cacert, NULL) != 1) {
            tolog(1, "ssl_server_ctx_init: load CAcert %s failed\n", cacert);
            goto retctx;
        }

        if (SSL_CTX_set_default_verify_paths(ctx) != 1) {
            tolog(1, "ssl_server_ctx_init: SSL_ctx_set_default_verify_path failed\n");
            goto retctx;
        }
    }

retctx:

    if (SSL_CTX_set_tlsext_servername_callback(ctx, ssl_servername_select) == 0) {
        tolog(1, "ssl_server_ctx_init: select servername by TLSEXT SNI failed.\n");
    }

    return ctx;
}

void * ssl_client_ctx_init (char * cert, char * prikey, char * cacert)
{
    SSL_CTX     * ctx = NULL;
    struct stat   stcert;
    struct stat   stkey;
    struct stat   stca;
    uint8         hascert = 0;
    uint8         haskey = 0;

    ctx = SSL_CTX_new(SSLv23_client_method());
    if (!ctx) return NULL;

    if (cert && file_stat(cert, &stcert) >= 0) {
        /* load certificate and private key, verify the cert with private key */
        if (SSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM) <= 0) {
            tolog(1, "ssl_client_ctx_init: loading Certificate file %s failed\n", cert);
            SSL_CTX_free(ctx);
            return NULL;
        }
        hascert = 1;
    }

    if (prikey && file_stat(prikey, &stkey) >= 0) {
        if (SSL_CTX_use_PrivateKey_file(ctx, prikey, SSL_FILETYPE_PEM) <= 0) {
            tolog(1, "ssl_client_ctx_init: loading Private Key file %s failed\n", prikey);
            SSL_CTX_free(ctx);
            return NULL;
        }
        haskey = 1;
    }

    if (hascert && haskey && !SSL_CTX_check_private_key(ctx)) {
        tolog(1, "ssl_client_ctx_init: Certificate verify failed! Private Key %s DOES NOT "
                 "match Certificate %s\n", cert, prikey);
        SSL_CTX_free(ctx);
        return NULL;
    }

    if (cacert && file_stat(cacert, &stca) >= 0) {
        if (SSL_CTX_load_verify_locations(ctx, cacert, NULL) != 1) {
            tolog(1, "ssl_client_ctx_init: load CAcert %s failed\n", cacert);
            goto retctx;
        }

        if (SSL_CTX_set_default_verify_paths(ctx) != 1) {
            tolog(1, "ssl_client_ctx_init: SSL_ctx_set_default_verify_path failed\n");
            goto retctx;
        }
    } else {
        /*SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);*/
    }

retctx:
    return ctx;
}

int ssl_ctx_free (void * vctx)
{
    SSL_CTX * ctx = (SSL_CTX *)vctx;

    if (!ctx) return -1;

    SSL_CTX_free(ctx);

    return 0;
}

int ssl_servername_select (SSL * ssl, int * ad, void * arg)
{
    char       * servername = NULL;
    SSL_CTX    * sslctx = NULL;
    ssl_tcp_t  * ssltcp = NULL;

    if (!ssl) return SSL_TLSEXT_ERR_NOACK;

    servername = (char *)SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    if (!servername)
        return SSL_TLSEXT_ERR_NOACK;

    ssltcp = SSL_get_ex_data(ssl, ssl_tcp_index);
    if (!ssltcp)
        return SSL_TLSEXT_ERR_NOACK;

    if (!ssltcp->ssllink || ssltcp->handshaked)
        return SSL_TLSEXT_ERR_NOACK;

    /* obtain the list of the relationship between the server domain name and
       sslctx through the ssltcp instance, then find the corresponding sslctx
       from the list according to the servername of SNI callback. example as following: */

    sslctx = ssltcp->sslctx;

    SSL_set_SSL_CTX(ssl, sslctx);

    SSL_set_verify(ssl, SSL_CTX_get_verify_mode(sslctx),
                        SSL_CTX_get_verify_callback(sslctx));

    SSL_set_verify_depth(ssl, SSL_CTX_get_verify_depth(sslctx));

#if OPENSSL_VERSION_NUMBER >= 0x009080dfL
    /* only in 0.9.8m+ */
    SSL_clear_options(ssl, SSL_get_options(ssl) & ~SSL_CTX_get_options(sslctx));
#endif

    SSL_set_options(ssl, SSL_CTX_get_options(sslctx));

#ifdef SSL_OP_NO_RENEGOTIATION
    SSL_set_options(ssl, SSL_OP_NO_RENEGOTIATION);
#endif

    tolog(1, "ssl_servername_select: SSL select server name %s successfully\n", servername);
    return SSL_TLSEXT_ERR_OK;
}

void * ssl_tcp_bind (SOCKET fd, int ssllink, void * ctx, int contype)
{
    ssl_tcp_t * ssltcp = NULL;

    if (fd == INVALID_SOCKET) return NULL;

    ssltcp = kzalloc(sizeof(*ssltcp));
    if (!ssltcp) return NULL;

    ssltcp->handshaked = 0;
    ssltcp->contype = contype;
    ssltcp->sslctx = ctx;
    ssltcp->fd = fd;

    ssltcp->ssllink = ssllink > 0 ? 1 : 0;
    if (!ssltcp->ssllink) return ssltcp;

    ssltcp->ssl = SSL_new(ctx);
    if (!ssltcp->ssl) {
        tolog(1, "ssl_tcp_bind: createing SSL instance failed");
        kfree(ssltcp);
        return NULL;
    }

    SSL_set_fd(ssltcp->ssl, fd);

    if (contype == 0) { //Connected
        SSL_set_connect_state(ssltcp->ssl);
    } else {
        SSL_set_accept_state(ssltcp->ssl);
    }

    if (SSL_set_ex_data(ssltcp->ssl, ssl_tcp_index, (void *)ssltcp) == 0) {
        tolog(1, "ssl_tcp_bind: SSL_set_ex_data() failed");
    }

    return ssltcp;
}

int ssl_tcp_handshake (void * vssltcp, int * perr)
{
    ssl_tcp_t * ssltcp = (ssl_tcp_t *)vssltcp;
    int         sslret = 0;
    int         ret = 0;

    if (perr) *perr = 0;

    if (!ssltcp) return -1;

    if (!ssltcp->ssllink) return 0;
    if (ssltcp->handshaked) return 1;

    if (ssltcp->contype == 0) {
        sslret = SSL_connect(ssltcp->ssl);
        if (sslret == 1) {
            ssltcp->handshaked = 1;
            tolog(1, "ssl_tcp_handshake: SSL_connect fd=%d successfully! Using cipher: %s\n",
                  ssltcp->fd, SSL_get_cipher(ssltcp->ssl));
            return 2;
        }

        ret = SSL_get_error(ssltcp->ssl, sslret);
        if (perr) *perr = ret;

        switch (ret) {
        case SSL_ERROR_WANT_READ:
            /* waiting fd READ event */
            return 0;

        case SSL_ERROR_WANT_WRITE:
            /* waiting fd WRITE event */
            return 0;

        case SSL_ERROR_SSL:
        case SSL_ERROR_SYSCALL:
        default:
            tolog(1, "ssl_tcp_bind: SSL connect fd=%d handshake failed!\n", ssltcp->fd);
            return -100;
        }

    } else {
        sslret = SSL_accept(ssltcp->ssl);
        if (sslret == 1) {
            ssltcp->handshaked = 1;
            tolog(1, "ssl_tcp_handshake: SSL_accept fd=%d successfully! Using cipher: %s\n",
                  ssltcp->fd, SSL_get_cipher(ssltcp->ssl));
            return 2;
        }

        ret = SSL_get_error(ssltcp->ssl, sslret);
        if (perr) *perr = ret;

        switch (ret) {
        case SSL_ERROR_WANT_READ:
            /* waiting clifd READ event */
            return 0;

        case SSL_ERROR_WANT_WRITE:
            /* waiting clifd WRITE event */
            return 0;

        case SSL_ERROR_SSL:
            tolog(1, "ssl_tcp_bind: SSL accept fd=%d handshake failed! SSL_ERROR_SSL ret=%d\n",
                  ssltcp->fd, ret);
            return -101;

        case SSL_ERROR_SYSCALL:
            tolog(1, "ssl_tcp_bind: SSL accept fd=%d handshake failed! SSL_ERROR_SYSCALL ret=%d\n",
                  ssltcp->fd, ret);
            return -102;

        default:
            tolog(1, "ssl_tcp_bind: SSL accept fd=%d handshake failed! ret=%d\n",
                  ssltcp->fd, ret);
            return -103;
        }
    }

    return 0;
}

/* When SSL handshake is completed, the peer's certificate can be verified as needed.
 * when verify is ok, retuan value will be greater than 0,
 * when the conditions required for verifying are not available, 0 is returned.
 * when errors occur, a negative value is returned.
 */
int ssl_tcp_verify (void * vssltcp, char * peername)
{
    ssl_tcp_t * ssltcp = (ssl_tcp_t *)vssltcp;
    X509      * peer = NULL;
    char        peer_CN[256] = {0};

    if (!ssltcp) return -1;

    if (!ssltcp->ssllink) return 1;
    if (!ssltcp->handshaked) return 0;

    /* verify peer certificate */
    if (SSL_get_verify_result(ssltcp->ssl) != X509_V_OK) {
        return -100;
    }

    /*Check the cert chain. The chain length is automatically checked by OpenSSL when
      we set the verify depth in the ctx */

    /*Check the common name*/
    if (peername) {
        peer = SSL_get_peer_certificate(ssltcp->ssl);
        if (!peer) return -101;

        X509_NAME_get_text_by_NID(X509_get_subject_name(peer), NID_commonName, peer_CN, 256);

        if (strcasecmp(peer_CN, peername) != 0) {
            return -110;
        }
    }

    return 1;
}

int ssl_tcp_close (void * vssltcp)
{
    ssl_tcp_t * ssltcp = (ssl_tcp_t *)vssltcp;

    if (!ssltcp) return -1;

    if (ssltcp->ssl) {
        SSL_shutdown(ssltcp->ssl);
        SSL_free(ssltcp->ssl);
    }

    if (ssltcp->fd != INVALID_SOCKET)
        closesocket(ssltcp->fd);

    kfree(ssltcp);
    return 0;
}

int ssl_tcp_unbind (void * vssltcp)
{
    ssl_tcp_t * ssltcp = (ssl_tcp_t *)vssltcp;

    if (!ssltcp) return -1;

    if (ssltcp->ssl) {
        SSL_shutdown(ssltcp->ssl);
        SSL_free(ssltcp->ssl);
    }

    kfree(ssltcp);

    return 0;
}

void * ssl_tcp_from_ssl (SSL * ssl)
{
    if (!ssl) return NULL;

    return SSL_get_ex_data(ssl, ssl_tcp_index);
}

int ssl_tcp_read (void * vssltcp, frame_p frm, int * num, int * perr)
{
    ssl_tcp_t * ssltcp = (ssl_tcp_t *)vssltcp;
    uint8       buf[128*1024];
    int         size = sizeof(buf);
    int         ret = 0, readLen = 0;
    int         sslerr = 0;

    if (!ssltcp) return -1;

    if (!ssltcp->ssllink)
        return frame_tcp_nbzc_recv(frm, ssltcp->fd, num, perr);

    if (!ssltcp->ssl) return -2;

    for (readLen = 0; ;) {
        ret = SSL_read(ssltcp->ssl, buf, size);

        if (ret > 0) {
            readLen += ret;
            if (frm) frame_put_nlast(frm, buf, ret);
            continue;
        }

        sslerr = SSL_get_error(ssltcp->ssl, ret);

        if (num) *num = readLen;

        if (ret == 0) {
            if (sslerr == SSL_ERROR_ZERO_RETURN) {
                if (perr) *perr = EBADF;
                return -20;

            } else {
                if (perr) *perr = EBADF;
                return -30;
            }

        } else { //ret < 0
            if (sslerr == SSL_ERROR_WANT_READ) {
                if (perr) *perr = EAGAIN;
                break;

            } else if (sslerr == SSL_ERROR_WANT_WRITE) {
                if (perr) *perr = EAGAIN;
                break;

            } else if (sslerr == SSL_ERROR_SSL) {
                if (perr) *perr = EPROTO;

            } else if (sslerr == SSL_ERROR_SYSCALL) {
                if (perr) *perr = errno;
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;

            } else {
                if (perr) *perr = EINVAL;
            }

            return -30;
        }
    }

    if (num) *num = readLen;

    return readLen;
}


int ssl_tcp_write (void * vssltcp, void * pbyte, int bytelen, int * num, int * perr)
{
    ssl_tcp_t * ssltcp = (ssl_tcp_t *)vssltcp;
    int         wbytes = 0;
    int         wlen = 0;
    int         ret = 0;
    int         sslerr = 0;

    if (num) *num = 0;
    if (perr) *perr = 0;

    if (!ssltcp) return -1;
    if (!pbyte || bytelen <= 0) return 0;

    if (!ssltcp->ssllink) {
        struct iovec iov[2];
        iov[0].iov_base = pbyte;
        iov[1].iov_len = bytelen;
        return tcp_writev(ssltcp->fd, &iov[0], 1, num, perr);
    }

    if (!ssltcp->ssl) return -2;

    for (wbytes = 0; wbytes < bytelen; ) {

        ret = SSL_write(ssltcp->ssl, pbyte + wbytes, bytelen - wbytes);
        if (ret > 0) {
            wbytes += ret;
            wlen += ret;
            continue;
        }

        sslerr = SSL_get_error(ssltcp->ssl, ret);

        if (num) *num = wlen;

        if (ret == 0) {
            if (sslerr == SSL_ERROR_ZERO_RETURN) {
                if (perr) *perr = EBADF;
                return -20;

            } else {
                if (perr) *perr = EBADF;
                return -30;
            }

        } else { //ret < 0
            if (sslerr == SSL_ERROR_WANT_READ) {
                if (perr) *perr = EAGAIN;
                return wlen;

            } else if (sslerr == SSL_ERROR_WANT_WRITE) {
                if (perr) *perr = EAGAIN;
                return wlen;

            } else if (sslerr == SSL_ERROR_SSL) {
                if (perr) *perr = EPROTO;

            } else if (sslerr == SSL_ERROR_SYSCALL) {
                if (perr) *perr = errno;
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    return wlen;
                }

            } else {
                if (perr) *perr = EINVAL;
            }

            return -30;
        }
    } //end for (wbytes = 0; wbytes < bytelen; )

    if (num) *num = wlen;

    return wlen;
}


int ssl_tcp_writev (void * vssltcp, void * piov, int iovcnt, int * num, int * perr)
{
    ssl_tcp_t    * ssltcp = (ssl_tcp_t *)vssltcp;
    struct iovec * iov = (struct iovec *)piov;
    void         * pbyte;
    int            bytelen;
    int            wbytes;
    int            wlen = 0;
    int            i;
    int            ret = 0;
    int            sslerr = 0;

    if (num) *num = 0;
    if (perr) *perr = 0;

    if (!iov || iovcnt <= 0) return 0;

    if (!ssltcp) return -1;

    if (!ssltcp->ssllink) {
        return tcp_writev(ssltcp->fd, iov, iovcnt, num, perr);
    }

    if (!ssltcp->ssl) return -2;

    for (i = 0; i < iovcnt; i++) {
        pbyte = iov[i].iov_base;
        bytelen = iov[i].iov_len;

        for (wbytes = 0; wbytes < bytelen; ) {

            ret = SSL_write(ssltcp->ssl, pbyte + wbytes, bytelen - wbytes);
            if (ret > 0) {
                wbytes += ret;
                wlen += ret;
                continue;
            }

            sslerr = SSL_get_error(ssltcp->ssl, ret);

            if (num) *num = wlen;

            if (ret == 0) {
                if (sslerr == SSL_ERROR_ZERO_RETURN) {
                    if (perr) *perr = EBADF;
                    return -20;

                } else {
                    if (perr) *perr = EBADF;
                    return -30;
                }

            } else { //ret < 0
                if (sslerr == SSL_ERROR_WANT_READ) {
                    if (perr) *perr = EAGAIN;
                    return wlen;

                } else if (sslerr == SSL_ERROR_WANT_WRITE) {
                    if (perr) *perr = EAGAIN;
                    return wlen;

                } else if (sslerr == SSL_ERROR_SSL) {
                    if (perr) *perr = EPROTO;

                } else if (sslerr == SSL_ERROR_SYSCALL) {
                    if (perr) *perr = errno;
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        return wlen;
                    }

                } else {
                    if (perr) *perr = EINVAL;
                }

                return -30;
            }
        } //end for (wbytes = 0; wbytes < bytelen; )
    }

    if (num) *num = wlen;

    return wlen;
}


int ssl_tcp_sendfile (void * vssltcp, int filefd, int64 pos, int64 size, int * num, int * perr)
{
    ssl_tcp_t    * ssltcp = (ssl_tcp_t *)vssltcp;
    static int     mmapsize = 8192 * 1024;
    void         * pbyte = NULL;
    void         * pmap = NULL;
    int64          maplen = 0;
#if defined(_WIN32) || defined(_WIN64)
    HANDLE         hmap;
    int64          mapoff = 0;
#endif

    size_t         onelen = 0;
    int64          wlen = 0;
    int            wbytes = 0;

    int            ret = 0;
    int            sslerr = 0;

    if (num) *num = 0;
    if (perr) *perr = 0;

    if (!ssltcp) return -1;
    if (filefd < 0) return -2;

    if (!ssltcp->ssllink) {
        return tcp_sendfile(ssltcp->fd, filefd, pos, size, num, perr);
    }

    for (wlen = 0; pos + wlen < size; ) {
        onelen = size - wlen;
        if (onelen > mmapsize) onelen = mmapsize;

#ifdef UNIX
        pbyte = file_mmap(NULL, filefd, pos + wlen, onelen, PROT_READ, MAP_PRIVATE, &pmap, &maplen, NULL);
#elif defined(_WIN32) || defined(_WIN64)
        pbyte = file_mmap(NULL, (HANDLE)filefd, pos + wlen, onelen, NULL, &hmap, &pmap, &maplen, &mapoff);
#endif
        if (!pbyte) break;

        for (wbytes = 0; wbytes < onelen; ) {
            ret = SSL_write(ssltcp->ssl, pbyte + wbytes, onelen - wbytes);
            if (ret > 0) {
                wbytes += ret;
                wlen += ret;
                continue;
            }

#ifdef UNIX
            munmap(pmap, maplen);
#elif defined(_WIN32) || defined(_WIN64)
            file_munmap(hmap, pmap);
#endif

            if (num) *num = wlen;

            sslerr = SSL_get_error(ssltcp->ssl, ret);

            if (ret == 0) {
                if (sslerr == SSL_ERROR_ZERO_RETURN) {
                    if (perr) *perr = EBADF;
                    return -20;

                } else {
                    if (perr) *perr = EBADF;
                    return -30;
                }

            } else { //ret < 0
                if (sslerr == SSL_ERROR_WANT_READ) {
                    if (perr) *perr = EAGAIN;
                    return wlen;

                } else if (sslerr == SSL_ERROR_WANT_WRITE) {
                    if (perr) *perr = EAGAIN;
                    return wlen;

                } else if (sslerr == SSL_ERROR_SSL) {
                    if (perr) *perr = EPROTO;

                } else if (sslerr == SSL_ERROR_SYSCALL) {
                    if (perr) *perr = errno;
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        return wlen;
                    }

                } else {
                    if (perr) *perr = EINVAL;
                }

                return -30;
            }

        } //end for (wbytes = 0; wbytes < onelen; )

#ifdef UNIX
        munmap(pmap, maplen);
#elif defined(_WIN32) || defined(_WIN64)
        file_munmap(hmap, pmap);
#endif
    }

    if (num) *num = wlen;

    return wlen;
}

#endif
