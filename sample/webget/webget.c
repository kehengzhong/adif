#include "adifall.ext"
#include <string.h>

#include "http_uri.h"
#include "http_cookie.h"
#include "http_msg.h"

void * mimemgmt = NULL;
void * ckiemgmt = NULL;

int main (int argc, char ** argv)
{
    HTTPMsg * msg = NULL;
    int       fd;
    SSLTcp  * ssltcp = NULL;
    SSL_CTX * ctx = NULL;
    static char * ckiefile = "cookie.inf";

    uint8     meth = 0; //GET or POST
    uint8     useproxy = 0;
    char    * puri = NULL;
    int       urilen = 0;
    char    * proxy = NULL;
    char    * pbody = NULL;
    int       bodylen = 0;
    uint8     bodytype = 0;
    int       ret = 0;
    char      dtbuf[32];
    int       err = 0;
    int       crashed = 0;

    if (argc < 2) {
        printf("Usage: %s [GET/POST] <URL> [BODY]\n", argv[0]);
        printf("Usage: %s <PROXY> <PROXY HOST:PORT> [GET/POST] <URL> [BODY]\n", argv[0]);
        return 0;
    }

    if (str_ncasecmp(argv[1], "POST", 4) == 0) {
        meth = 1;
        useproxy = 0;
        if (argc < 3) {
            printf("Usage: %s POST <URL> <BODY>\n", argv[0]);
            return -10;
        }
        puri = argv[2];
        urilen = str_len(puri);
        if (argc >= 4) pbody = argv[3];
    } else if (str_ncasecmp(argv[1], "GET", 3) == 0) {
        meth = 0;
        useproxy = 0;
        if (argc < 3) {
            printf("Usage: %s [GET] <URL>\n", argv[0]);
            return -11;
        }
        puri = argv[2];
        urilen = str_len(puri);
    } else if (str_ncasecmp(argv[1], "PROXY", 5) == 0) {
        if (argc < 4) {
            printf("Usage: %s <PROXY> <PROXY HOST:PORT> <GET/POST> <URL>\n", argv[0]);
            return -10;
        }
        useproxy = 1;
        proxy = argv[2];

        if (str_ncasecmp(argv[3], "POST", 4) == 0) {
            if (argc < 5) {
                printf("Usage: %s <PROXY> <PROXY HOST:PORT> POST <URL> <BODY>\n", argv[0]);
                return -10;
            }
            meth = 1;
            puri = argv[4];
            urilen = str_len(puri);
            if (argc >= 6) pbody = argv[5];
        } else if (str_ncasecmp(argv[3], "GET", 3) == 0) {
            if (argc < 5) {
                printf("Usage: %s <PROXY> <PROXY HOST:PORT> GET <URL>\n", argv[0]);
                return -10;
            }
            meth = 0;
            puri = argv[4];
            urilen = str_len(puri);
        } else {
            if (argc > 4) {
                printf("Error Parameter %s, Only support GET or POST method\n", argv[1]);
                return 0;
            }
            meth = 0;
            puri = argv[4];
            urilen = str_len(puri);
        }
    } else {
        if (argc > 2) {
            printf("Error Parameter %s, Only support GET or POST method\n", argv[1]);
            return 0;
        }
        meth = 0;
        useproxy = 0;
        puri = argv[1];
        urilen = str_len(puri);
    }

    ssl_tcp_init();
    ctx = ssl_client_ctx_init(NULL, NULL, NULL);

    mimemgmt = mime_type_init();
    ckiemgmt = cookie_mgmt_alloc(ckiefile, 0, NULL);

    msg = http_msg_alloc();
    if (!msg) {
        printf("http_msg_alloc failed\n");
        goto errexit;
    }

sendhttp:

    http_req_init(msg);

    http_req_set_meth(msg, meth);
    if (useproxy) http_req_set_proxy(msg, proxy, strlen(proxy));
    http_req_set_uri(msg, puri, urilen);

    if (msg->uri.host == NULL || msg->uri.hostlen <= 0) {
        printf("Error URL %s, No request Host\n", puri);
        return -100;
    }

    http_cookie_add(ckiemgmt, msg);

    bodylen = str_len(pbody);
    if (meth == 1 && pbody) { //POST
        if (file_is_regular(pbody)) { //POST
            bodytype = 3; //filename
            http_req_set_body(msg, bodytype, pbody, bodylen);
        } else {
            bodytype = 1; //bufptr
            http_req_set_body(msg, bodytype, pbody, bodylen);
            http_req_set_contype(msg, "text/plain", -1);
        }
    }

    http_req_encode(msg);

    printf("%s", frameS(msg->req_hdrfrm));

    if (msg->useproxy) {
        fd = tcp_connect(msg->proxy_host, msg->proxy_port, NULL, 0, NULL, NULL);
    } else {
        fd = tcp_connect(msg->host, msg->port, NULL, 0, NULL, NULL);
    }
    if (fd == INVALID_SOCKET) {
        printf("tcp_connect error, cannot connect %s:%d\n",
               msg->useproxy ? msg->proxy_host : msg->host,
               msg->useproxy ? msg->proxy_port : msg->port);
        goto errexit;
    }

    sock_nonblock_set(fd, 1);

    if (msg->ssllink && msg->useproxy) {
        /* use HTTP CONNECT to build a tunnel for TLS/SSL */
        HTTPMsg * tunmsg = NULL;
        char      buf[512];

        snprintf(buf, sizeof(buf)-1, "%s:%d", msg->host, msg->port);

        tunmsg = http_msg_alloc();
        http_req_set_meth(tunmsg, 4);
        http_req_set_uri(tunmsg, buf, strlen(buf));

        http_cookie_add(ckiemgmt, tunmsg);

        http_req_encode(tunmsg);
        printf("%s", frameS(tunmsg->req_hdrfrm));

        ssltcp = ssl_tcp_bind(fd, 0, NULL, 0);
        if (!ssltcp) {
            printf("ssl_tcp_bind: failed for CONNECT request. ssllink=%d\n", msg->ssllink);
            goto errexit;
        }

        /* now send HTTP request header/body to Web server */
        ret = http_req_send(tunmsg, ssltcp);
        if (ret < 0) {
            printf("http_req_send: failed for sending CONNECT request. ret=%d\n", ret);
            goto errexit;
        }

        ret = http_resp_recv(tunmsg, ssltcp, &crashed);
        printf("%s\r\n", (char *)frameS(tunmsg->res_hdrfrm));

        if (ret < 0) {
            printf("http_resp_recv: failed for receiving CONNECT response, ret=%d\n", ret);
            goto errexit;
        }
        if (crashed) {
            printf("http_resp_recv: connection crashed for receiving CONNECT response, ret=%d\n", ret);
            goto errexit;
        }

        ssl_tcp_unbind(ssltcp);
    }

    ssltcp = ssl_tcp_bind(fd, msg->ssllink, ctx, 0);
    if (!ssltcp) {
        printf("ssl_tcp_bind: failed. ssllink=%d\n", msg->ssllink);
        goto errexit;
    }

    while (ssltcp->ssllink && !ssltcp->handshaked) {
        ret = ssl_tcp_handshake(ssltcp, &err);
        if (ret > 0) break;
        else if (ret < 0) {
            printf("ssl_tcp_handshake: failed. ret=%d ssllink=%d\n", ret, msg->ssllink);
            goto errexit;
        }

        if (err == SSL_ERROR_WANT_READ) {
            sock_read_ready(ssltcp->fd, 2000);
        } else if (err == SSL_ERROR_WANT_WRITE) {
            sock_write_ready(ssltcp->fd, 2000);
        }
    }

    /* now send HTTP request header/body to Web server */
    ret = http_req_send(msg, ssltcp);
    if (ret < 0) {
        printf("http_req_send: failed. ret=%d\n", ret);
        goto errexit;
    }

    /* now receiving HTTP response from Web server */

    http_resp_init(msg);

    ret = http_resp_recv(msg, ssltcp, &crashed);

    printf("%s\r\n", (char *)frameS(msg->res_hdrfrm));

    if (ret < 0) {
        printf("http_resp_recv: failed ret=%d\n", ret);
        goto errexit;
    }

    str_datetime(NULL, dtbuf, sizeof(dtbuf)-1, 0);
    printf("%s - Saved to file \"%s\" [%d/%ld]\r\n\r\n",
           dtbuf, msg->res_file, ret, msg->res_length);

    if (msg->res_status == 301 || msg->res_status == 302) {
        ssl_tcp_close(ssltcp);
        ssltcp = NULL;

        meth = 0; //get
        puri = msg->res_loc;
        urilen = msg->res_loclen;
        puri[urilen] = '\0';

        printf("HTTP response redirect to %s\n", puri);

        goto sendhttp;
    }

errexit:
    if (ssltcp) ssl_tcp_close(ssltcp);
    if (msg) http_msg_free(msg);

    if (mimemgmt) mime_type_clean(mimemgmt);
    if (ckiemgmt) cookie_mgmt_free(ckiemgmt);
    if (ctx) ssl_ctx_free(ctx);

    return 0;
}

