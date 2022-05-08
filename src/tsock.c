/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#include "btype.h"
#include "memory.h"
#include "tsock.h"
#include "fileop.h"
#include "strutil.h"
#include "trace.h"

#include <signal.h>

#define IOBUF_SIZE  65536
#define IO_TIMEOUT  120

#ifdef UNIX

#include <stdarg.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/poll.h>
#include <netinet/tcp.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>

#include <sys/stat.h>
#include <sys/uio.h>
#include <ifaddrs.h>

#ifdef _LINUX_
#include <sys/sendfile.h>
#endif
#ifdef _FREEBSD_
#include <netinet/tcp_fsm.h>
#include <net/if_dl.h>
#endif

#define SENDFILE_MAXSIZE  2147479552L

#endif

#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#include <mswsock.h>
#endif


int sock_nonblock_get (SOCKET fd)
{
#ifdef UNIX
    int flags = fcntl(fd, F_GETFL);

    if (flags < 0) return -1;

    if (flags & O_NONBLOCK) return 1;
#endif
    return 0;
}

int sock_nonblock_set (SOCKET fd, int nbflag)
{
#if defined(_WIN32) || defined(_WIN64)
    u_long  arg;

    if (nbflag) arg = 1;
    else arg = 0;

    if (ioctlsocket(fd, FIONBIO, &arg) == SOCKET_ERROR)
        return -1;
#endif
 
#ifdef UNIX
    int flags, newflags;
 
    flags = fcntl(fd, F_GETFL);
    if (flags < 0)  return -1;
 
    if (nbflag) newflags = flags | O_NONBLOCK;
    else newflags = flags & ~O_NONBLOCK;
 
    if (newflags != flags) {
        if (fcntl(fd, F_SETFL, newflags) < 0) 
            return -1;
    }
#endif
 
    return 0;
}

int pipe_create (SOCKET sockfd[2])
{
#if defined(_WIN32) || defined(_WIN64)
    SECURITY_ATTRIBUTES sa = {0};

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    return CreatePipe((PHANDLE)&sockfd[0], (PHANDLE)&sockfd[1], &sa, 0);

#else
    if (pipe(sockfd)) {
        return -1;
    }

    return 0;
#endif
}

int sock_pair_create (int type, SOCKET sockfd[2])
{
#if defined(_WIN32) || defined(_WIN64)
    struct sockaddr_in  saddr = { 0 };
    struct sockaddr_in  addr = { 0 };
    socklen_t           slen = sizeof(struct sockaddr_in);
    SOCKET              connfd = INVALID_SOCKET;

    sockfd[0] = INVALID_SOCKET;
    sockfd[1] = INVALID_SOCKET;

    do {
        sockfd[0] = socket(AF_INET, type, 0);
        if (INVALID_SOCKET == sockfd[0])
            break;

        sockfd[1] = socket(AF_INET, type, 0);
        if (INVALID_SOCKET == sockfd[1])
            break;

        saddr.sin_family = AF_INET;
        saddr.sin_port = 0;
        saddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        if (bind(sockfd[0], (struct sockaddr *)&saddr, sizeof(saddr)))
            break;

        if (getsockname(sockfd[0], (struct sockaddr*)&saddr, &slen))
            break;

        if (SOCK_STREAM == type) {
            if (listen(sockfd[0], 1)) break;

        } else {
            addr.sin_family = AF_INET;
            addr.sin_port = 0;
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

            if (bind(sockfd[1], (struct sockaddr*)&addr, sizeof(addr)))
                break;

            if (getsockname(sockfd[1], (struct sockaddr*)&addr, &slen))
                break;

            if (connect(sockfd[0], (struct sockaddr*)&addr, slen))
                break;
        }

        if (connect(sockfd[1], (struct sockaddr*)&saddr, slen))
            break;

        if (SOCK_STREAM == type) {
            int on = 1;

            connfd = accept(sockfd[0], NULL, NULL);
            if (INVALID_SOCKET == connfd)
                break;

            closesocket(sockfd[0]);

            sockfd[0] = connfd;
            if (setsockopt(sockfd[0], IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on)) ||
                setsockopt(sockfd[1], IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on)))
            {
                break;
            }
        }

        return 0;

    } while (0);

    if (sockfd[0] != INVALID_SOCKET) {
        closesocket(sockfd[0]);
    }

    if (sockfd[1] != INVALID_SOCKET) {
        closesocket(sockfd[1]);
    }

    return -1;

#else
    return socketpair(AF_UNIX, type, 0, sockfd) == 0;
#endif
}

int sock_unread_data (SOCKET fd)
{
#if defined(_WIN32) || defined(_WIN64)
    long count = 0;
#endif
#ifdef UNIX
    int count = 0;
#endif
    int  ret = 0;
 
    if (fd == INVALID_SOCKET) return 0;
 
#if !defined(FIONREAD)
    return 0;
#endif

#if defined(_WIN32) || defined(_WIN64)
        ret = ioctlsocket(fd, FIONREAD, (u_long *)&count);
#endif
#ifdef UNIX
        ret = ioctl(fd, FIONREAD, &count);
#endif

    if (ret >= 0) return (int)count;

    return ret;
}

int sock_is_open (SOCKET fd) 
{                
    fd_set r, w, e; 
    int rt;      
    struct timeval tv; 
                     
    if (fd == INVALID_SOCKET) return 0;

    for (;;) {            
#ifdef UNIX
        if (fd < FD_SETSIZE-10) {
            FD_ZERO(&r); FD_ZERO(&w); FD_ZERO(&e);  
            FD_SET(fd, &e);
            tv.tv_sec = 0; tv.tv_usec = 0;
 
            rt = select(fd+1, &r, &w, &e, &tv);
        } else {
            struct pollfd fds[1];

            fds[0].fd = fd;
            fds[0].events = POLLIN | POLLPRI | POLLOUT;
            fds[0].revents = 0;
            rt = poll(fds, 1, 0);
        }
        if (rt == -1 && (errno != EAGAIN && errno != EINTR)) {
            return 0;
        }
#endif
#if defined(_WIN32) || defined(_WIN64)
        FD_ZERO(&r); FD_ZERO(&w); FD_ZERO(&e);  
        FD_SET(fd, &e);
        tv.tv_sec = 0; tv.tv_usec = 0;

        rt = select(0, &r, &w, &e, &tv);
        if (rt == -1) {
            int errcode = WSAGetLastError();
            if (errcode != WSAEINTR && errcode != WSAEWOULDBLOCK)
                return 0;
        }
#endif
        if (rt != -1)
            return 1;
    }
    return 0;
}


int sock_read_ready (SOCKET fd, int ms)
{
    fd_set rf;
    struct timeval to;
    int ret;
    int errcode;

    if (fd == INVALID_SOCKET) return 0;

retry:
#ifdef UNIX
    errno = 0;
    if (fd < FD_SETSIZE-10) {
        FD_ZERO (&rf);
        FD_SET  (fd, &rf);
        to.tv_sec  = ms/1000;
        to.tv_usec = (ms%1000)*1000;

        ret = select(fd+1, &rf, NULL, NULL, &to);
        if (ret > 0 && FD_ISSET(fd, &rf))
            return 1;
    } else {
        struct pollfd fds[1];

        fds[0].fd = fd;
        fds[0].events = POLLIN | POLLPRI;
        fds[0].revents = 0;
        ret = poll(fds, 1, ms);
        if (ret > 0) return 1;
    }
#endif
#if defined(_WIN32) || defined(_WIN64)
    FD_ZERO (&rf);
    FD_SET  (fd, &rf);
    to.tv_sec  = ms/1000;
    to.tv_usec = (ms%1000)*1000;

    ret = select(0, &rf, NULL, NULL, &to);
    if (ret > 0 && FD_ISSET(fd, &rf))
        return 1;
#endif

    if (ret < 0) {
#if defined(_WIN32) || defined(_WIN64)
        errcode = WSAGetLastError();
        switch(errcode) {
        case WSAEINTR:
            goto retry;
        case WSAENOTSOCK:
            break;
        case WSAEINVAL:
            break;
        }
#endif
#ifdef UNIX
        errcode = errno;
        switch(errcode) {
        case EINTR:
            goto retry;
        case EAGAIN:
            return 1;
        case EBADF:
            break;
        case EINVAL:
            break;
        }
#endif
        return -1;
    }

    return 0;
}


int sock_write_ready (SOCKET fd, int ms)
{
    fd_set wf;
    struct timeval to;
    int ret;
    int errcode;

    if (fd == INVALID_SOCKET) return 0;

retry:
#ifdef UNIX
    errno = 0;
    if (fd < FD_SETSIZE - 10) {
        FD_ZERO (&wf);
        FD_SET  (fd, &wf);
        to.tv_sec  = ms/1000;
        to.tv_usec = (ms%1000)*1000;

        ret = select(fd+1, NULL, &wf, NULL, &to);
        if (ret > 0 && FD_ISSET(fd, &wf))
            return 1;
    } else {
        struct pollfd fds[1];
 
        fds[0].fd = fd;
        fds[0].events = POLLOUT;
        fds[0].revents = 0;
        ret = poll(fds, 1, ms);
        if (ret > 0) return 1;
    }

#endif
#if defined(_WIN32) || defined(_WIN64)
    FD_ZERO (&wf);
    FD_SET  (fd, &wf);
    to.tv_sec  = ms/1000;
    to.tv_usec = (ms%1000)*1000;

    ret = select(0, NULL, &wf, NULL, &to);
    if (ret > 0 && FD_ISSET(fd, &wf))
        return 1;
#endif

    if (ret < 0) {
#if defined(_WIN32) || defined(_WIN64)
        errcode = WSAGetLastError();
        switch(errcode) {
        case WSAEINTR:
        case WSAEWOULDBLOCK:
            goto retry;
        case WSAENOTSOCK:
            break;
        case WSAEINVAL:
            break;
        }
#endif
#ifdef UNIX
        errcode = errno;
        switch(errcode) {
        case EINTR:
        case EAGAIN:
            goto retry;
        case EBADF:
            break;
        case EINVAL:
            break;
        }
#endif
        return -1;
    }

    return 0;
}

void sock_addr_to_epaddr (void * psa, ep_sockaddr_t * addr)
{
    struct sockaddr * sa = (struct sockaddr *)psa;

    if (!sa || !addr) return;

    addr->family = sa->sa_family;

    if (sa->sa_family == AF_INET) {
        addr->socklen = sizeof(addr->u.addr4);
        addr->u.addr4 = *(struct sockaddr_in *)sa;

    } else if (sa->sa_family == AF_INET6) {
        addr->socklen = sizeof(addr->u.addr6);
        addr->u.addr6 = *(struct sockaddr_in6 *)sa;
    }
}

void sock_addr_ntop (void * psa, char * buf)
{
    struct sockaddr * sa = (struct sockaddr *)psa;

    if (!sa || !buf) return;

    if (sa->sa_family == AF_INET) {
        inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), buf, 48);
    } else if (sa->sa_family == AF_INET6) {
        inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), buf, 48);
    }
}

uint16 sock_addr_port (void * psa)
{
    struct sockaddr * sa = (struct sockaddr *)psa;

    if (!sa) return 0;

    if (sa->sa_family == AF_INET) {
        return ntohs(((struct sockaddr_in *)sa)->sin_port);
    } else if (sa->sa_family == AF_INET6) {
        return ntohs(((struct sockaddr_in6 *)sa)->sin6_port);
    }
    return 0;
}

int sock_inet_addr_parse (char * text, int len, uint32 * inaddr, int * retlen) 
{ 
    int       segs = 0;     /* Segment count. */ 
    int       chcnt = 0;    /* Character count within segment. */ 
    int       accum = 0;    /* Accumulator for segment. */ 
    uint32    sect[4];
    uint32    addr = INADDR_ANY;
    int       i;
 
    if (retlen) *retlen = 0;

    if (!text) return -1; 
    if (len < 0) len = strlen(text);
    if (len <= 0) return -2;
 
    for (i = 0; i < len; i++) {
        if (text[i] == '.') { 
            /* Must have some digits in segment. */ 
            if (chcnt == 0) return -100; 
 
            /* Limit number of segments. */ 
            if (segs >= 3) return -101; 
 
            sect[segs++] = accum;

            /* Reset segment values and restart loop. */ 
            chcnt = accum = 0; 
            continue; 
        } 

        /* Check numeric. */ 
        if ((text[i] < '0') || (text[i] > '9')) {
            if (segs == 3 && chcnt > 0) {
                break;
            } 
            return -102; 
        }
 
        /* Accumulate and check segment. */ 
        if ((accum = accum * 10 + text[i] - '0') > 255) 
            return -103; 
 
        /* Advance other segment specific stuff and continue loop. */ 
        chcnt++; 
    } 
 
    /* Check enough segments and enough characters in last segment. */ 
    if (segs != 3 || chcnt == 0) return -104; 

    sect[segs] = accum; segs++;
    addr = (sect[0] << 24) + (sect[1] << 16) + (sect[2] << 8) + sect[3];

    if (inaddr) *inaddr = htonl(addr);
    if (retlen) *retlen = i;

    return i; 
} 

int sock_inet6_addr_parse (char * p, int len, char * addr, int * retlen)
{
    char      c, *zero, *digit, *s, *d, *p0;
    int       len4;
    uint32    n, nibbles, word;
 
    if (!p) return -1;
    if (len < 0) len = strlen(p);
    if (len <= 0) return -2;
 
    if (retlen) *retlen = 0;
    p0 = p;

    zero = NULL;
    digit = NULL;
    len4 = 0;
    nibbles = 0;
    word = 0;
    n = 8;
 
    if (p[0] == ':') {
        p++;
        len--;
    }
 
    for (/* void */; len; len--) {
        c = *p++;
 
        if (c == ':') {
            if (nibbles) {
                digit = p;
                len4 = len;
                *addr++ = (char) (word >> 8);
                *addr++ = (char) (word & 0xff);
 
                if (--n) {
                    nibbles = 0;
                    word = 0;
                    continue;
                }
            } else {
                if (zero == NULL) {
                    digit = p;
                    len4 = len;
                    zero = addr;
                    continue;
                }
            }
 
            return -100;
        }
 
        if (c == '.' && nibbles) {
            if (n < 2 || digit == NULL) {
                return -101;
            }
 
            if (sock_inet_addr_parse(digit, len4 - 1, &word, NULL) < 0)
               return -102;

            word = ntohl(word);
            *addr++ = (char) ((word >> 24) & 0xff);
            *addr++ = (char) ((word >> 16) & 0xff);
            n--;
            break;
        }
 
        if (++nibbles > 4) {
            return -103;
        }
 
        if (c >= '0' && c <= '9') {
            word = word * 16 + (c - '0');
            continue;
        }
 
        c |= 0x20;
 
        if (c >= 'a' && c <= 'f') {
            word = word * 16 + (c - 'a') + 10;
            continue;
        }
 
        return -104;
    }
 
    if (nibbles == 0 && zero == NULL) {
        return -105;
    }
 
    *addr++ = (char) (word >> 8);
    *addr++ = (char) (word & 0xff);
 
    if (--n) {
        if (zero) {
            n *= 2;
            s = addr - 1;
            d = s + n;
            while (s >= zero) {
                *d-- = *s--;
            }
            memset(zero, 0, n);
            if (retlen) *retlen = p - p0;
            return p - p0;
        }
 
    } else {
        if (zero == NULL) {
            if (retlen) *retlen = p - p0;
            return p - p0;
        }
    }
 
    return -200;
}

int sock_addr_parse (char * text, int len, int port, ep_sockaddr_t * addr)
{
    if (!text) return -1;
    if (len < 0) len = strlen(text);
    if (len <= 0) return -2;

    if (!addr) return -3;

    memset(addr, 0, sizeof(*addr));

    /* try first to parse the string as the inet4 IPv4 */
    if (sock_inet_addr_parse(text, len, (uint32 *)&addr->u.addr4.sin_addr.s_addr, NULL) > 0) {
        addr->u.addr4.sin_family = AF_INET;
        addr->socklen = sizeof(struct sockaddr_in);
        addr->family = AF_INET;
        addr->u.addr4.sin_port = htons((uint16)port);
        return 1;
    }

    /* try to parse the string as the inet6 IPv6 */
    if (sock_inet6_addr_parse(text, len, (char *)addr->u.addr6.sin6_addr.s6_addr, NULL) > 0) {
        addr->u.addr6.sin6_family = AF_INET6;
        addr->socklen = sizeof(struct sockaddr_in6);
        addr->family = AF_INET6;
        addr->u.addr6.sin6_port = htons((uint16)port);
        return 1;
    }

    return -100;
}


int checkcopy (ep_sockaddr_t * iter, struct addrinfo * rp)
{
    iter->socklen = rp->ai_addrlen;
    iter->family = rp->ai_family;
    iter->socktype = rp->ai_socktype;

    if (rp->ai_family == AF_INET) {
        memcpy(&iter->u.addr4, rp->ai_addr, iter->socklen);
        return 1;
    } else if (rp->ai_family == AF_INET6) {
        memcpy(&iter->u.addr6, rp->ai_addr, iter->socklen);
        return 1;
    }
    return 0;
}

/* proto options including: 
     IPPROTO_IP(0), IPPROTO_TCP(6), IPPROTO_UDP(17),
     IPPROTO_RAW(255) IPPROTO_IPV6(41)
   socktype value including: SOCK_STREAM(1), SOCK_DGRAM(2), SOCK_RAW(3) */

int sock_addr_acquire (ep_sockaddr_t * addr, char * host, int port, int socktype)
{
    struct addrinfo    hints;
    struct addrinfo  * result = NULL;
    struct addrinfo  * rp = NULL;
    char               buf[16];
    int                num = 0;
    ep_sockaddr_t    * iter = NULL;
    ep_sockaddr_t    * newa = NULL;
    int                dup = 0;

    if (!addr) return -1;

    if (sock_addr_parse(host, -1, port, addr) > 0) {
        return 1;
    }

    sprintf(buf, "%d", port);

    //if (socktype != SOCK_STREAM && socktype != SOCK_DGRAM && socktype != SOCK_RAW)
    //    socktype = SOCK_STREAM;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = socktype;
    hints.ai_protocol = 0;
    hints.ai_flags = 0;

    if (getaddrinfo(host, buf, &hints, &result) != 0) {
        if (result) freeaddrinfo(result);
        return -100;
    }

    for (num = 0, rp = result; rp != NULL; rp = rp->ai_next) {
        if (num == 0) {
            num += checkcopy(addr, rp);
            continue;
        }

        for (dup = 0, iter = addr; iter != NULL; iter = iter->next) {
            if (memcmp(&iter->u.addr, rp->ai_addr, iter->socklen) == 0) {
                dup = 1; break;
            }
        }
        if (dup == 0) {
            newa = kzalloc(sizeof(*newa));
            if (checkcopy(newa, rp) > 0) {
                for (iter = addr; iter->next != NULL; iter = iter->next);
                iter->next = newa;
                num++;
            } else kfree(newa);
        }
    }

    freeaddrinfo(result);

    return num;
}

void sock_addr_freenext (ep_sockaddr_t * addr)
{
    ep_sockaddr_t  * iter, * cur;

    if (!addr) return;

    for (iter = addr->next; iter != NULL; ) {
        cur = iter; iter = iter->next;
        kfree(cur);
    }
}

int sock_addr_get (char * dst, int dstlen, int port, int socktype,
                   char *ip, int * pport, ep_sockaddr_t * paddr)
{
    ep_sockaddr_t  addr;
    char           host[1024];
    int ret = 0;

    memset(&addr, 0, sizeof(addr));
    str_secpy(host, sizeof(host)-1, dst, dstlen);

    ret = sock_addr_acquire(&addr, host, port, socktype);
    if (ret <= 0) return -1;

    if (ip)
        sock_addr_ntop(&addr.u.addr, ip);

    if (pport)
        *pport = sock_addr_port(&addr.u.addr);

    if (paddr)
        *paddr = addr;

    sock_addr_freenext(&addr);

    return ret;
}


int sock_option_set (SOCKET fd, sockopt_t * opt)
{
    struct timeval tv;

    if (fd == INVALID_SOCKET)
        return -1;

    if (!opt) return -2;

    if (opt->mask & SOM_REUSEADDR) {
        opt->reuseaddr_ret = 
                setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, 
                           (const void *)&opt->reuseaddr, sizeof(int));
        if (opt->reuseaddr_ret != 0)
            perror("REUSEADDR");
    }

#ifdef SO_REUSEPORT
    if (opt->mask & SOM_REUSEPORT) {
        opt->reuseport_ret = 
            setsockopt(fd, SOL_SOCKET, SO_REUSEPORT,
                       (const void *)&opt->reuseport, sizeof(int));
        if (opt->reuseport_ret != 0)
            perror("REUSEPORT");
    }
#endif

    if (opt->mask & SOM_KEEPALIVE) {
        opt->keepalive_ret = 
            setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
                       (const void *)&opt->keepalive, sizeof(int));
        if (opt->keepalive_ret != 0)
            perror("KEEPALIVE");
    }

    if (opt->mask & SOM_IPV6ONLY) {
        opt->ipv6only_ret = 
            setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY,
                       (const void *)&opt->ipv6only, sizeof(int));
    }

    if (opt->mask & SOM_RCVBUF) {
        opt->rcvbuf_ret = 
            setsockopt(fd, SOL_SOCKET, SO_RCVBUF,
                       (const void *)&opt->rcvbuf, sizeof(int));
    }

    if (opt->mask & SOM_SNDBUF) {
        opt->sndbuf_ret = 
            setsockopt(fd, SOL_SOCKET, SO_RCVBUF,
                       (const void *)&opt->sndbuf, sizeof(int));
    }

    if ((opt->mask & SOM_RCVTIMEO) && opt->rcvtimeo > 0) {
        tv.tv_sec = opt->rcvtimeo / 1000;
        tv.tv_usec = opt->rcvtimeo % 1000;

        opt->rcvtimeo_ret = 
            setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
                       (const void *)&tv, sizeof(struct timeval));
    }

    if ((opt->mask & SOM_SNDTIMEO) && opt->sndtimeo > 0) {
        tv.tv_sec = opt->sndtimeo / 1000;
        tv.tv_usec = opt->sndtimeo % 1000;

        opt->sndtimeo_ret =
            setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO,
                       (const void *)&tv, sizeof(struct timeval));
    }

#ifdef TCP_KEEPIDLE
    if (opt->mask & SOM_TCPKEEPIDLE) {
        opt->keepidle_ret = 
            setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE,
                       (const void *)&opt->keepidle, sizeof(int));
    }
#endif

#ifdef TCP_KEEPINTVL
    if (opt->mask & SOM_TCPKEEPINTVL) {
        opt->keepintvl_ret = 
            setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL,
                       (const void *)&opt->keepintvl, sizeof(int));
    }
#endif

#ifdef TCP_KEEPCNT
    if (opt->mask & SOM_TCPKEEPCNT) {
        opt->keepcnt_ret = 
            setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT,
                       (const void *)&opt->keepcnt, sizeof(int));
    }
#endif

#ifdef SO_SETFIB
    if (opt->mask & SOM_SETFIB) {
        opt->setfib_ret = 
            setsockopt(fd, SOL_SOCKET, SO_SETFIB,
                   (const void *)&opt->setfib, sizeof(int));
    }
#endif

#ifdef TCP_FASTOPEN
    if (opt->mask & SOM_TCPFASTOPEN) {
        opt->fastopen_ret = 
            setsockopt(fd, IPPROTO_TCP, TCP_FASTOPEN,
                       (const void *)&opt->fastopen, sizeof(int));
    }
#endif

    if (opt->mask & SOM_TCPNODELAY) {
        opt->nodelay_ret = 
            setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
                       (const void *)&opt->nodelay, sizeof(int));
    }

#ifdef TCP_NOPUSH
    if (opt->mask & SOM_TCPNOPUSH) {
        opt->nopush_ret = 
            setsockopt(fd, IPPROTO_TCP, TCP_NOPUSH,
                       (const void *)&opt->nopush, sizeof(int));
    }
#endif

#ifdef SO_ACCEPTFILTER
    if ((opt->mask & SOM_ACCEPTFILTER) && opt->af != NULL) {
        opt->af_ret = 
            setsockopt(fd, SOL_SOCKET, SO_ACCEPTFILTER,
                       (const void *)opt->af, sizeof(struct accept_filter_arg));
    }
#endif

#ifdef TCP_DEFER_ACCEPT
    if (opt->mask & SOM_TCPDEFERACCEPT) {
        opt->defer_accept_ret = 
            setsockopt(fd, IPPROTO_TCP, TCP_DEFER_ACCEPT,
                       (const void *)&opt->defer_accept, sizeof(int));
    }
#endif

#ifdef IP_RECVDSTADDR
    if (opt->mask & SOM_IPRECVDSTADDR) {
        opt->recv_dst_addr_ret = 
            setsockopt(fd, IPPROTO_IP, IP_RECVDSTADDR,
                       (const void *)&opt->recv_dst_addr, sizeof(int));
    }
#endif

#ifdef IP_PKTINFO
    if (opt->mask & SOM_IPPKTINFO) {
        opt->ip_pktinfo_ret = 
            setsockopt(fd, IPPROTO_IP, IP_PKTINFO,
                       (const void *)&opt->ip_pktinfo, sizeof(int));
    }
#endif

#ifdef IPV6_RECVPKTINFO
    if (opt->mask & SOM_IPV6RECVPKTINFO) {
        opt->ipv6_recv_pktinfo_ret = 
            setsockopt(fd, IPPROTO_IPV6, IPV6_RECVPKTINFO,
                       (const void *)&opt->ipv6_recv_pktinfo, sizeof(int));
    }
#endif

    return 0;
}

int sock_nodelay_set (SOCKET fd)
{
    int  one = 1;

    /* Don't delay send to coalesce packets  */
    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
                      (const void *) &one, sizeof(int));
}

int sock_nodelay_unset (SOCKET fd)
{
    int  one = 0;
 
    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
                      (const void *) &one, sizeof(int));
}

int sock_nopush_set (SOCKET fd)
{
#ifdef TCP_CORK
    int  one = 1;
    return setsockopt(fd, IPPROTO_TCP, TCP_CORK,
                      (const void *) &one, sizeof(int));

#elif defined TCP_NOPUSH
    int  one = 1;
    return setsockopt(fd, IPPROTO_TCP, TCP_NOPUSH,
                      (const void *) &one, sizeof(int));

#else
    return 0;
#endif
}

int sock_nopush_unset (SOCKET fd)
{
#ifdef TCP_CORK 
    int  one = 0;
    return setsockopt(fd, IPPROTO_TCP, TCP_CORK,
                      (const void *) &one, sizeof(int));
 
#elif defined TCP_NOPUSH 
    int  one = 0;
    return setsockopt(fd, IPPROTO_TCP, TCP_NOPUSH,
                      (const void *) &one, sizeof(int));
 
#else 
    return 0;
#endif
}


void addrinfo_print (void * vrp)
{
    struct addrinfo * rp = (struct addrinfo *)vrp;
    char buf[512];
    char ipstr[128];

    if (!rp) return;

    buf[0] = '\0';
    if (rp->ai_family == AF_INET) sprintf(buf, "AF_INET");
    else if (rp->ai_family == AF_INET6) sprintf(buf, "AF_INET6");
    else if (rp->ai_family == AF_UNIX) sprintf(buf, "AF_UNIX");
    else sprintf(buf, "UnknownAF");

    if (rp->ai_socktype == SOCK_STREAM) sprintf(buf+strlen(buf), " SOCK_STREAM");
    else if (rp->ai_socktype == SOCK_DGRAM) sprintf(buf+strlen(buf), " SOCK_DGRAM");
    else if (rp->ai_socktype == SOCK_RAW) sprintf(buf+strlen(buf), " SOCK_RAW");
#ifdef SOCK_PACKET
    else if (rp->ai_socktype == SOCK_PACKET) sprintf(buf+strlen(buf), " SOCK_PACKET");
#endif
    else sprintf(buf+strlen(buf), " UnknownSockType");

    if (rp->ai_protocol == IPPROTO_IP) sprintf(buf+strlen(buf), " IPPROTO_IP");
    else if (rp->ai_protocol == IPPROTO_TCP) sprintf(buf+strlen(buf), " IPPROTO_TCP");
    else if (rp->ai_protocol == IPPROTO_UDP) sprintf(buf+strlen(buf), " IPPROTO_UDP");
    else if (rp->ai_protocol == IPPROTO_IPV6) sprintf(buf+strlen(buf), " IPPROTO_IPV6");
    else if (rp->ai_protocol == IPPROTO_RAW) sprintf(buf+strlen(buf), " IPPROTO_RAW");
    else if (rp->ai_protocol == IPPROTO_ICMP) sprintf(buf+strlen(buf), " IPPROTO_ICMP");
    else sprintf(buf+strlen(buf), " UnknownProto");

    if (rp->ai_family == AF_INET) {
        inet_ntop(AF_INET, &(((struct sockaddr_in *)rp->ai_addr)->sin_addr), ipstr, 128);
        sprintf(buf+strlen(buf), " %s:%d", ipstr, ntohs(((struct sockaddr_in *)rp->ai_addr)->sin_port));
    } else if (rp->ai_family == AF_INET6) {
        inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)rp->ai_addr)->sin6_addr), ipstr, 128);
        sprintf(buf+strlen(buf), " %s:%d", ipstr, ntohs(((struct sockaddr_in6 *)rp->ai_addr)->sin6_port));
    }

    sprintf(buf+strlen(buf), " SockLen: %d  AI_Flags: %d", rp->ai_addrlen, rp->ai_flags);
    printf("%s\n", buf);
}

SOCKET tcp_listen (char * localip, int port, void * psockopt, sockattr_t * fdlist, int * fdnum)
{
    struct addrinfo    hints;
    struct addrinfo  * result = NULL;
    struct addrinfo  * rp = NULL;
    SOCKET             aifd = INVALID_SOCKET;
    char               buf[128];

    SOCKET             listenfd = INVALID_SOCKET;
    sockopt_t        * sockopt = NULL;
    int                num = 0;
    int                rpnum = 0;
    int                one = 0;
    int                backlog = 511;
    int                ret = 0;

    sockopt = (sockopt_t *)psockopt;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;       /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;   /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;       /* For wildcard IP address */
    hints.ai_protocol = IPPROTO_TCP;   /* TCP protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    sprintf(buf, "%d", port);
 
    if (localip && strlen(localip) <= 1)
        localip = NULL;

    aifd = getaddrinfo(localip, buf, &hints, &result);
    if (aifd != 0) {
        if (result) freeaddrinfo(result);
        tolog(1, "getaddrinfo: %s:%s return %d\n", localip, buf, aifd);
        return -100;
    }

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully bind(2).
       If socket(2) (or bind(2)) fails, we (close the socket
       and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
#ifdef _DEBUG
        addrinfo_print(rp);
#endif

        rpnum++;

        listenfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (listenfd == INVALID_SOCKET) {
            tolog(1, "tcp_listen socket() failed");
            continue;
        }

        if (sockopt) {
            if (sockopt->mask & SOM_BACKLOG)
                backlog = sockopt->backlog;
            sock_option_set(listenfd, sockopt);

        } else { //set the default options
            one = 1;
            ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (void *)&one, sizeof(int));
            if (ret != 0) perror("TCPListen REUSEADDR");
#ifdef SO_REUSEPORT
            ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, (void *)&one, sizeof(int));
            if (ret != 0) perror("TCPListen REUSEPORT");
#endif
            ret = setsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&one, sizeof(int));
            if (ret != 0) perror("TCPListen KEEPALIVE");
        }

        if (bind(listenfd, rp->ai_addr, rp->ai_addrlen) != 0) {
            tolog(1, "tcp_listen bind() failed");
            closesocket(listenfd);
            listenfd = INVALID_SOCKET;
            continue; 
        }

        if (backlog <= 0) backlog = 511;
        if (listen(listenfd, backlog) == SOCKET_ERROR) {
            tolog(1, "tcp_listen fd=%d failed", listenfd);
            closesocket(listenfd);
            listenfd = INVALID_SOCKET;
            continue; 
        }

        if (fdlist && fdnum && num < *fdnum) {
            fdlist[num].fd = listenfd;
            fdlist[num].family = rp->ai_family;
            fdlist[num].socktype = rp->ai_socktype;
            fdlist[num].protocol = rp->ai_protocol;
            num++;
        } else
            break;
    }
    freeaddrinfo(result);

    if (fdnum) *fdnum = num;

    if (rpnum <= 0) {
        tolog(1, "tcp_listen no addrinfo available\n");
        /* there is no address/port that bound or listened successfully! */
        return INVALID_SOCKET;
    }

    return listenfd;
}

SOCKET tcp_connect_full (char * host, int port, int nonblk, char * lip, int lport, int * succ, sockattr_t * attr)
{
    struct addrinfo    hints;
    struct addrinfo  * result = NULL;
    struct addrinfo  * rp = NULL;
    SOCKET             aifd = INVALID_SOCKET;
    char               buf[128];
 
    SOCKET             confd = INVALID_SOCKET;
    ep_sockaddr_t      addr;
    int                one = 0;
 
    if (sock_addr_parse(host, -1, port, &addr) > 0) {
        addr.socktype = SOCK_STREAM;
        confd = tcp_ep_connect(&addr, nonblk, lip, lport, NULL, succ);

        if (attr) {
            attr->fd = confd;
            attr->family = addr.family;
            attr->socktype = addr.socktype;
            attr->protocol = IPPROTO_TCP;
        }

        return confd;
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;       /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;   /* Stream socket */
    hints.ai_flags = 0;               /* For wildcard IP address */
    hints.ai_protocol = IPPROTO_TCP;   /* TCP protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
 
    sprintf(buf, "%d", port);
 
    aifd = getaddrinfo(host, buf, &hints, &result);
    if (aifd != 0) {
        if (result) freeaddrinfo(result);
        tolog(1, "tcp_connect: getaddrinfo: %s:%s return %d\n", host, buf, aifd);
        return -100;
    }
 
    for (rp = result; rp != NULL; rp = rp->ai_next) {
#ifdef _DEBUG
        addrinfo_print(rp);
#endif
        confd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (confd == INVALID_SOCKET)
            continue;

        if (attr) {
            attr->fd = confd;
            attr->family = rp->ai_family;
            attr->socktype = rp->ai_socktype;
            attr->protocol = rp->ai_protocol;
        }

        one = 1;
        setsockopt(confd, SOL_SOCKET, SO_REUSEADDR, (void *)&one, sizeof(int));
#ifdef SO_REUSEPORT
        setsockopt(confd, SOL_SOCKET, SO_REUSEPORT, (void *)&one, sizeof(int));
#endif
        setsockopt(confd, SOL_SOCKET, SO_KEEPALIVE, (void *)&one, sizeof(int));
 
        if (nonblk) sock_nonblock_set(confd, 1);

        if ((lip && strlen(lip) > 0) || lport > 0) {
            memset(&addr, 0, sizeof(addr));
            one = sock_addr_acquire(&addr, lip, lport, SOCK_STREAM);
            if (one <= 0 || bind(confd, (struct sockaddr *)&addr.u.addr, addr.socklen) != 0) {
                if (one > 1) sock_addr_freenext(&addr);
                closesocket(confd);
                confd = -1;
                continue;
            }
        }

        if (connect(confd, rp->ai_addr, rp->ai_addrlen) == 0) {
            if (succ) *succ = 1;
        } else {
            if (succ) *succ = 0;
    #ifdef UNIX
            if (errno != 0  && errno != EINPROGRESS && errno != EALREADY && errno != EWOULDBLOCK) {
                closesocket(confd);
                confd = -1;
                continue;
            }
    #endif
    #if defined(_WIN32) || defined(_WIN64)
            if (WSAGetLastError() != WSAEWOULDBLOCK) { 
                closesocket(confd);
                confd = INVALID_SOCKET;
                continue;
            }
    #endif
        }

        break;
    }
    freeaddrinfo(result);
 
    if (rp == NULL) {
        /* there is no address/port that bound or listened successfully! */
        return INVALID_SOCKET;
    }
 
    return confd;
}

SOCKET tcp_connect (char * host, int port, char * lip, int lport, sockattr_t * attr)
{
    return tcp_connect_full(host, port, 0, lip, lport, NULL, attr);
}

SOCKET tcp_nb_connect (char * host, int port, char * lip, int lport, int * consucc, sockattr_t * attr)
{
    return tcp_connect_full(host, port, 1, lip, lport, consucc, attr);
}

SOCKET tcp_ep_connect (ep_sockaddr_t * addr, int nblk, char * lip, int lport, void * popt, int * succ)
{
    ep_sockaddr_t   sock;
    sockopt_t     * opt = (sockopt_t *)popt;
    SOCKET          confd = INVALID_SOCKET;
    int             ret, one = 0;

    if (!addr) return confd;

    confd = socket(addr->family, addr->socktype, IPPROTO_TCP);
    if (confd == INVALID_SOCKET)
        return confd;

    if (opt) {
        sock_option_set(confd, opt);

    } else { //set the default options
        one = 1;
        ret = setsockopt(confd, SOL_SOCKET, SO_REUSEADDR, (void *)&one, sizeof(int));
        if (ret != 0) perror("TCPConnect REUSEADDR");
    #ifdef SO_REUSEPORT
        ret = setsockopt(confd, SOL_SOCKET, SO_REUSEPORT, (void *)&one, sizeof(int));
        if (ret != 0) perror("TCPConnect REUSEPORT");
    #endif
        ret = setsockopt(confd, SOL_SOCKET, SO_KEEPALIVE, (void *)&one, sizeof(int));
        if (ret != 0) perror("TCPConnect KEEPALIVE");
    }

    if (nblk) sock_nonblock_set(confd, 1);

    if ((lip && strlen(lip) > 0) || lport > 0) {
        memset(&sock, 0, sizeof(sock));
        one = sock_addr_acquire(&sock, lip, lport, SOCK_STREAM);
        if (one <= 0 || bind(confd, (struct sockaddr *)&sock.u.addr, sock.socklen) != 0) {
            if (one > 1) sock_addr_freenext(&sock);
            closesocket(confd);
            confd = -1;
            return confd;
        }
    }
 
    if (connect(confd, &addr->u.addr, addr->socklen) == 0) {
        if (succ) *succ = 1;
    } else {
        if (succ) *succ = 0;
#ifdef UNIX
        if (errno != 0  && errno != EINPROGRESS && errno != EALREADY && errno != EWOULDBLOCK) {
            closesocket(confd);
            confd = -1;
            return confd;
        }
#endif
#if defined(_WIN32) || defined(_WIN64)
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            closesocket(confd);
            confd = INVALID_SOCKET;
            return confd;
        }
#endif
    }

    return confd;
}

int tcp_connected (SOCKET fd)
{
#ifdef UNIX
    struct tcp_info  info; 
    int              len = 0;
    int              ret = 0;

    if (fd < 0) return 0;

    len = sizeof(info); 

    ret = getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len); 
    if (ret == SOCKET_ERROR) return 0;
#ifdef _FREEBSD_
    if (info.tcpi_state == TCPS_ESTABLISHED) {
#else
    if (info.tcpi_state == TCP_ESTABLISHED) {
#endif
        return 1; 
    } 

    return 0; 
#endif

#if defined(_WIN32) || defined(_WIN64)
    int  ret = 0;
    int  secnum = 0;
    int  len = 0;

    if (fd < 0) return 0;

    len = sizeof(secnum);

    ret = getsockopt(fd, SOL_SOCKET, SO_CONNECT_TIME, &secnum, &len);
    if (ret == SOCKET_ERROR) {
        return 0;
    }

    return 1;
#endif
}

int tcp_recv (SOCKET fd, void * prcvbuf, int toread, long waitms, int * actnum)
{
    uint8 * rcvbuf = (uint8 *)prcvbuf;
    int     ret=0, readLen = 0, reqLen = 0;
    int     errcode;
    long    restms = 0;
    fd_set  rFDs;
    struct  timeval tv, *ptv = NULL;
    struct  timeval tick0, tick1;
    int     unread = 0;
#ifdef UNIX
#ifdef _SOLARIS_
    int     errcase = 0, eret = 0;
    int     errlen = sizeof(errcase);
#endif
#endif

    if (actnum) *actnum = 0;

    if (toread <= 0) reqLen = 1024*1024;
    else reqLen = toread;

    if (fd == INVALID_SOCKET) return -70;
    if (!rcvbuf) return 0;

    if (waitms > 0) {
        gettimeofday(&tick0, NULL);
        restms = waitms;
    } else {
        restms = 0;
    }

    for (readLen = 0; readLen < reqLen; ) {
        if (waitms > 0) {
            gettimeofday(&tick1, NULL);
            restms -= tv_diff_us(&tick0, &tick1)/1000;
            if (restms <= 0) return 0;
            tick0 = tick1;
        } else {
            if (toread <= 0) goto beginrecv;
        }
        if (restms > 0) {
            tv.tv_sec = restms/1000;
            tv.tv_usec = (restms%1000)*1000;
            ptv = &tv;
        } else ptv = NULL;

#ifdef UNIX
        if (fd < FD_SETSIZE - 10) {
            FD_ZERO(&rFDs);
            FD_SET(fd, &rFDs);

            ret = select(fd+1, &rFDs, NULL, NULL, ptv);
        } else {
            struct pollfd fds[1];
 
            fds[0].fd = fd;
            fds[0].events = POLLIN | POLLPRI;
            fds[0].revents = 0;
            ret = poll(fds, 1, restms);
        }
#endif
#if defined(_WIN32) || defined(_WIN64)
        FD_ZERO(&rFDs);
        FD_SET(fd, &rFDs);

        ret = select(0, &rFDs, NULL, NULL, ptv);
#endif
        if (ret < 0) {
#if defined(_WIN32) || defined(_WIN64)
            errcode = WSAGetLastError();
            switch(errcode) {
            case WSAEINTR:
                continue;
            case WSAENOTSOCK:
            case WSAEINVAL:
                ret = -100;
                goto error;;
            }
#endif
#ifdef UNIX
            errcode = errno;
            switch(errcode) {
            case EINTR:
            case EAGAIN:
                continue;
            case EBADF:
            case EINVAL:
                ret = -100;
                goto error;;
            }
#endif
            continue;
        } else if (ret == 0) continue;

beginrecv:
        unread = sock_unread_data(fd);
#ifdef UNIX
        errno = 0;
#endif
        if (unread > 0) {
            if (unread > reqLen - readLen)
                unread = reqLen - readLen;

            while (unread > 0) {
                ret = recv(fd, rcvbuf + readLen, unread, MSG_NOSIGNAL);
                if (ret > 0) {
                    readLen += ret;
                    unread -= ret;
                    continue;
                }
                break;
            } 

        } else {
            ret = recv (fd, rcvbuf+readLen, reqLen-readLen, MSG_NOSIGNAL);
            if (ret > 0) {
                readLen += ret;
                continue;
            }
        }

        if (ret == 0) { 
            if (actnum) *actnum = readLen; 
            return -20; /* connection closed by other end */

        } else if (ret == -1) {
#if defined(_WIN32) || defined(_WIN64)
            errcode = WSAGetLastError();
            if (errcode == WSAEINTR || errcode == WSAEWOULDBLOCK) {
                if (toread <= 0 && waitms <= 0) break;
                continue;
            }
#endif
#ifdef UNIX
            errcode = errno;
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                if (toread <= 0 && waitms <= 0) break;
                continue;
            }
    #ifdef _SOLARIS_
            errlen = sizeof(errcase);
            errcase = 0;
            ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &errcase, &errlen);
            if (ret < 0 || errcase != 0) { ret = -31; goto error; }
            if(errcode == 0) continue;
    #endif
#endif
            ret = -30;
            goto error;
        }
    }

    if (actnum) *actnum = readLen;
    return readLen;

error:
    if (actnum) *actnum = readLen;
    return ret;
}


int tcp_send (SOCKET fd, void * psendbuf, int towrite, long waitms, int * actnum)
{
    uint8 * sendbuf = (uint8 *)psendbuf;
    int     ret, sendLen = 0;
    int     errcode;
    long    restms = 0;
    fd_set  wFDs;
    struct  timeval tv, *ptv = NULL;
    struct  timeval tick0, tick1;
#ifdef UNIX
#ifdef _SOLARIS_
    int     errcase = 0, eret = 0;
    int     errlen = sizeof(errcase);
#endif
#endif

    if (actnum) *actnum = 0;

    if (fd == INVALID_SOCKET) return -70;
    if (!sendbuf) return 0;
    if (towrite <= 0) return 0;

    if (waitms > 0) {
        gettimeofday(&tick0, NULL);
        restms = waitms;
    } else {
        restms = 0;
    }

    for (sendLen=0; sendLen < towrite; ) {
        if (waitms > 0) {
            gettimeofday(&tick1, NULL);
            restms -= tv_diff_us(&tick0, &tick1)/1000;
            if (restms <= 0) {
                if (actnum) *actnum = sendLen;
                return 0;
            }
            tick0 = tick1;
        }
        if (restms > 0) {
            tv.tv_sec = restms/1000;
            tv.tv_usec = (restms%1000)*1000;
            ptv = &tv;
        } else ptv = NULL;

#ifdef UNIX
        if (fd < FD_SETSIZE - 10) {
            FD_ZERO(&wFDs);
            FD_SET(fd, &wFDs);
            ret = select(fd+1, NULL, &wFDs, NULL, ptv);
        } else {
            struct pollfd fds[1];
 
            fds[0].fd = fd;
            fds[0].events = POLLOUT;
            fds[0].revents = 0;
            ret = poll(fds, 1, restms);
        }
#endif
#if defined(_WIN32) || defined(_WIN64)
        FD_ZERO(&wFDs);
        FD_SET(fd, &wFDs);
        ret = select(0, NULL, &wFDs, NULL, ptv);
#endif
        if (ret < 0) {
#if defined(_WIN32) || defined(_WIN64)
            errcode = WSAGetLastError();
            switch(errcode) {
            case WSAEINTR:
                continue;
            case WSAENOTSOCK:
            case WSAEINVAL:
                ret = -100;
                goto error;;
            }
#endif
#ifdef UNIX
            errcode = errno;
            switch(errcode) {
            case EINTR:
            case EAGAIN:
                continue;
            case EBADF:
            case EINVAL:
                ret = -100;
                goto error;;
            }
#endif
            continue;
        } else if (ret == 0) continue;

#ifdef UNIX
        errno = 0;
#endif
        ret = send (fd, sendbuf+sendLen, towrite-sendLen, MSG_NOSIGNAL);
        if (ret == -1) {
#if defined(_WIN32) || defined(_WIN64)
            errcode = WSAGetLastError();
            if (errcode == WSAEINTR || errcode == WSAEWOULDBLOCK) {
                continue;
            }
#endif
#ifdef UNIX
            errcode = errno;
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
    #ifdef _SOLARIS_
            errlen = sizeof(errcase);
            errcase = 0;
            ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &errcase, &errlen);
            if (ret < 0 || errcase != 0) { ret = -31; goto error; }
            if(errcode == 0) continue;
    #endif
#endif
            ret = -30;
            goto error;
        } else {
            sendLen += ret;
        }
    }

    if (actnum) *actnum = sendLen;
    return sendLen;
error:
    if (actnum) *actnum = sendLen;
    return ret;
}


int tcp_nb_recv (SOCKET fd, void * prcvbuf, int bufsize, int * actnum)
{
    uint8 * rcvbuf = (uint8 *)prcvbuf;
    int     ret=0, readLen = 0;
    int     errcode;
    int     unread = 0;
    int     errtimes = 0;
#ifdef UNIX
#ifdef _SOLARIS_
    int     errcase = 0, eret = 0;
    int     errlen = sizeof(errcase);
#endif
#endif 
        
    if (actnum) *actnum = 0;
            
    if (fd == INVALID_SOCKET) return -70;
    if (!rcvbuf || bufsize <= 0) return 0;

    for (readLen = 0, errtimes = 0; readLen < bufsize; ) {
        unread = sock_unread_data(fd);
#ifdef UNIX
        errno = 0;
#endif
        if (unread > 0) {
            if (unread > bufsize - readLen)
                unread = bufsize - readLen;

            while (unread > 0) {
                ret = recv(fd, rcvbuf+readLen, unread, MSG_NOSIGNAL);
                if (ret > 0) {
                    readLen += ret;
                    unread -= ret;
                    continue;
                }
                break;
            }
            if (ret > 0 && unread == 0) 
                break;

        } else {
            ret = recv(fd, rcvbuf+readLen, bufsize-readLen, MSG_NOSIGNAL);
            if (ret > 0) {
                readLen += ret;
                continue;
            }
        }

        if (ret == 0) {
            if (actnum) *actnum = readLen;
            return -20; /* connection closed by other end */

        } else if (ret == -1) {
#if defined(_WIN32) || defined(_WIN64)
            errcode = WSAGetLastError();
            if (errcode == WSAEINTR) {
                continue;
            }
            if (errcode == WSAEWOULDBLOCK) {
                if (++errtimes >= 1) break;
                continue;
            }
#endif
#ifdef UNIX
            errcode = errno;
            if (errcode == EINTR) {
                continue;
            }
            if (errcode == EAGAIN || errcode == EWOULDBLOCK) {
                if (++errtimes >= 1) break;
                continue;
            }
    #ifdef _SOLARIS_
            errlen = sizeof(errcase);
            errcase = 0;
            ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &errcase, &errlen);
            if (ret < 0 || errcase != 0) { ret = -31; goto error; }
            if(errcode == 0) break;
    #endif
#endif
            ret = -30;
            goto error;
        }
    }

    if (actnum) *actnum = readLen;
    return readLen;

error:
    if (actnum) *actnum = readLen;
    return ret;
}


int tcp_nb_send (SOCKET fd, void * psendbuf, int towrite, int * actnum)
{
    uint8 * sendbuf = (uint8 *)psendbuf;
    int     sendLen = 0;
    int     ret, errcode;
    int     errtimes = 0;
#ifdef UNIX
#ifdef _SOLARIS_
    int     errcase = 0, eret = 0;
    int     errlen = sizeof(errcase);
#endif
#endif

    if (actnum) *actnum = 0;

    if (fd == INVALID_SOCKET) return -1;
    if (!sendbuf) return 0;
    if (towrite <= 0) return 0;

    for (sendLen = 0, errtimes = 0; sendLen < towrite; ) {
#ifdef UNIX
        errno = 0;
#endif
        ret = send (fd, sendbuf+sendLen, towrite-sendLen, MSG_NOSIGNAL);
        if (ret == -1) {
#if defined(_WIN32) || defined(_WIN64)
            errcode = WSAGetLastError();
            if (errcode == WSAEINTR || errcode == WSAEWOULDBLOCK) {
                if (++errtimes >= 1) break;
                continue;
            }
#endif
#ifdef UNIX
            errcode = errno;
            if (errcode == EINTR || errcode == EAGAIN || errcode == EWOULDBLOCK) {
                if (++errtimes >= 1) break;
                continue;
            }
    #ifdef _SOLARIS_
            errlen = sizeof(errcase);
            errcase = 0;
            ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &errcase, &errlen);
            if (ret < 0 || errcase != 0) { ret = -31; goto error; }
            if (errcode == 0) {
                if (++errtimes >= 1) break;
                continue;
            }
    #endif
#endif
            ret = -30;
            goto error;
        } else {
            sendLen += ret;
        }
    }

    if (actnum) *actnum = sendLen;
    return sendLen;
error:
    if (actnum) *actnum = sendLen;
    return ret;
}


int tcp_writev (SOCKET fd, void * piov, int iovcnt, int * actnum, int * perr)
{
#ifdef UNIX
    struct iovec * iov = (struct iovec *)piov;
    int  ret, errcode;
    int  ind, wlen, sentnum = 0;
 
    if (actnum) *actnum = 0;
    if (perr) *perr = 0;
 
    if (fd == INVALID_SOCKET) return -1;
    if (!iov || iovcnt <= 0) return 0;

    signal(SIGPIPE, SIG_IGN);

    for (ind = 0, sentnum = 0; ind < iovcnt; ) {
        ret = writev(fd, iov + ind, iovcnt - ind);
        if (ret < 0) {
            errcode = errno;
            if (perr) *perr = errcode;
     
            if (errcode == EINTR) {
                continue;
     
            } else if (errcode == EAGAIN || errcode == EWOULDBLOCK) {
                return 0;
            }
     
            return -30;
        }
    
        sentnum += ret;
        if (actnum) *actnum += ret;
    
        for (wlen = ret; ind < iovcnt && wlen >= iov[ind].iov_len; ind++)
            wlen -= iov[ind].iov_len;
    
        if (ind >= iovcnt) break;
    
        iov[ind].iov_base = (char *)iov[ind].iov_base + wlen;
        iov[ind].iov_len -= wlen;
    }

    return sentnum;

#elif defined (_WIN32) || defined(_WIN64)

    struct iovec * iov = (struct iovec *)piov;
    int  i, ret, errcode;
    int  wlen = 0, sendLen = 0;
    uint8 * pbyte = NULL;
 
    if (actnum) *actnum = 0;
    if (perr) *perr = 0;
 
    if (fd == INVALID_SOCKET) return -1;
    if (!iov || iovcnt <= 0) return 0;

    for (i = 0; i < iovcnt; i++) {
        for (sendLen = 0; sendLen < (int)iov[i].iov_len; ) {
#ifdef UNIX
            errno = 0;
#endif
            pbyte = iov[i].iov_base;
            ret = send (fd, pbyte + sendLen, iov[i].iov_len - sendLen, MSG_NOSIGNAL);
            if (ret == -1) {
#if defined(_WIN32) || defined(_WIN64)
                errcode = WSAGetLastError();
                if (errcode == WSAEINTR || errcode == WSAEWOULDBLOCK) {
#else
                errcode = errno;
                if (errcode == EINTR || errcode == EAGAIN || errcode == EWOULDBLOCK) {
#endif
                    if (actnum) *actnum = wlen;
                    if (perr) *perr = errcode;
                    return wlen;
                }

                if (actnum) *actnum = wlen;
                if (perr) *perr = errcode;
                return -30;
            } else {
                sendLen += ret;
                wlen += ret;
            }
        }
    }

    if (actnum) *actnum = wlen;
    if (perr) *perr = 0;

    return wlen;
#endif
}

int tcp_sendfile (SOCKET fd, int srcfd, int64 pos, int64 size, int * actnum, int * perr)
{
#ifdef UNIX
    off_t       offval = pos;
    int64       wlen = 0;
    int64       onelen = 0;
    int         ret, err;
    int         times = 0;
    struct stat st;
 
    if (actnum) *actnum = 0;
    if (perr) *perr = 0;
 
    if (fd == INVALID_SOCKET) return -1;
    if (srcfd < 0) return -2;

    if (fstat(srcfd, &st) < 0)
        return -3;

    if (pos >= st.st_size) return 0;
 
    wlen = st.st_size - pos;
    if (size > wlen) size = wlen;

    for (wlen = 0 ; wlen < size; ) {
        onelen = min(size - wlen, SENDFILE_MAXSIZE);

#ifdef _FREEBSD_
        offval = 0;

        ret = sendfile(srcfd, fd, pos, onelen, NULL, &offval, 0);

        if (offval > 0) {
            wlen += offval;
            pos += offval;
        }

#elif defined(_OSX_) || defined(_MACOS_)
        offval = 0;

        ret = sendfile(srcfd, fd, pos, &offval, NULL, 0);

        if (offval > 0) {
            wlen += offval;
            pos += offval;
        }

#else
        ret = sendfile(fd, srcfd, &offval, onelen);
#endif
 
        if (ret < 0) { 
            err = errno;
            if (perr) *perr = err;

            if (err == EINTR) continue;

            if (err == EAGAIN || err == EWOULDBLOCK) {
                if (++times >= 1) break;
                continue;  
            } 

            if (actnum) *actnum = wlen;
            return -30;
        }
#if defined(_FREEBSD_) || defined(_OSX_) || defined(_MACOS_)
        else {
            /* do nothing */
        }
#else
        else if (ret == 0) { 
            /* if sendfile returns zero, then someone has truncated the file,
             * so the offset became beyond the end of the file */

            if (actnum) *actnum = wlen;
            return -40; 

        } else {
            wlen += ret;  
        }
#endif
    }

    if (actnum) *actnum = wlen;
 
    return (int)wlen;

#elif defined (_WIN32) || defined(_WIN64)

    struct _stat   st;
    uint8          pbyte[64*1024];

    int            onelen = 0;
    int64          wlen = 0;
    int            wbytes = 0;

    int            ret = 0;
    int            errcode = 0;

    if (actnum) *actnum = 0;
    if (perr) *perr = 0;

    if (fd == INVALID_SOCKET) return -1;
    if (srcfd < 0) return -2;

    if (_fstat(srcfd, &st) < 0)
        return -3;

    if (pos >= st.st_size) return 0;

    wlen = st.st_size - pos;
    if (size > wlen) size = wlen;

    _lseeki64(srcfd, pos, SEEK_SET);

    for (wlen = 0 ; wlen < size; ) {
        onelen = size - wlen;
        if (onelen > 64*1024) onelen = 64*1024;

        ret = filefd_read(srcfd, pbyte, onelen);
        if (ret <= 0) break;
        onelen = ret;

        for (wbytes = 0; wbytes < onelen; ) {
            ret = send(fd, pbyte + wbytes, onelen - wbytes, MSG_NOSIGNAL);
            if (ret > 0) {
                wbytes += ret;
                wlen += ret;
                continue;
            }

            errcode = WSAGetLastError();
            if (actnum) *actnum = wlen;
            if (perr) *perr = errcode;

            if (errcode == WSAEINTR || errcode == WSAEWOULDBLOCK) {
                return wlen;
            }
            return -30;
        } //end for (wbytes = 0; wbytes < onelen; )
    }

    if (actnum) *actnum = wlen;
    if (perr) *perr = 0;

    return wlen;

#else
    return 0;
#endif
}


SOCKET udp_listen (char * localip, int port, void * psockopt, sockattr_t * fdlist, int * fdnum)
{
    struct addrinfo    hints;
    struct addrinfo  * result = NULL;
    struct addrinfo  * rp = NULL;
    sockopt_t        * sockopt = NULL;
    SOCKET             aifd = INVALID_SOCKET;
    char               buf[128];
    SOCKET             listenfd = INVALID_SOCKET;
    int                num = 0;
    int                rpnum = 0;
    int                one, ret;
 
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;       /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM;   /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;       /* For wildcard IP address */
    hints.ai_protocol = IPPROTO_UDP;   /* TCP protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
 
    if (localip && strlen(localip) <= 1)
        localip = NULL;

    sprintf(buf, "%d", port);
 
    sockopt = (sockopt_t *)psockopt;

    aifd = getaddrinfo(localip, buf, &hints, &result);
    if (aifd != 0) {
        if (result) freeaddrinfo(result);
        return -100;
    }
 
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        rpnum++;

        listenfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (listenfd == INVALID_SOCKET)
            continue;
 
        if (sockopt) {
            sock_option_set(listenfd, sockopt);
 
        } else { //set the default options
            one = 1;
            ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (void *)&one, sizeof(int));
            if (ret != 0) perror("UDPListen REUSEADDR");
#ifdef SO_REUSEPORT
            ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, (void *)&one, sizeof(int));
            if (ret != 0) perror("UDPListen REUSEPORT");
#endif
            ret = setsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&one, sizeof(int));
            if (ret != 0) perror("UDPListen KEEPALIVE");
        }

        if (bind(listenfd, rp->ai_addr, rp->ai_addrlen) != 0) {
            closesocket(listenfd);
            listenfd = INVALID_SOCKET;
            continue;
        }

        if (fdlist && fdnum && num < *fdnum) {
            fdlist[num].fd = listenfd;
            fdlist[num].family = rp->ai_family;
            fdlist[num].socktype = rp->ai_socktype;
            fdlist[num].protocol = rp->ai_protocol;
            num++;
        } else
            break;
    }
    freeaddrinfo(result);

    if (fdnum) *fdnum = num;

    if (rpnum <= 0) {
        /* there is no address/port that bound or listened successfully! */
        return INVALID_SOCKET;
    }

    return listenfd;
}


#ifdef _LINUX_

#define inaddrr(x) (*(struct in_addr *) &ifr->x[sizeof sa.sin_port])
#define IFRSIZE   ((int)(size * sizeof (struct ifreq)))

int get_selfaddr (int num, AddrItem * pitem)
{
    int                 i;
    int                 sockfd, size = 1;
    struct ifreq      * ifr;
    struct ifconf       ifc;

    if (!pitem) return -1;

    if (0 > (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP))) {
        return -50;
    }

    ifc.ifc_len = IFRSIZE;
    ifc.ifc_req = NULL;

    do {
        ++size;

        /* realloc buffer size until no overflow occurs  */
        if (NULL == (ifc.ifc_req = realloc(ifc.ifc_req, IFRSIZE))) {
          close(sockfd);
          return -60;
        }
        ifc.ifc_len = IFRSIZE;
        if (ioctl(sockfd, SIOCGIFCONF, &ifc)) {
            close(sockfd);
            if (ifc.ifc_req) free(ifc.ifc_req);
            return -70;
        }
    } while (IFRSIZE <= ifc.ifc_len);

    ifr = ifc.ifc_req;
    for (i=0; (char *)ifr < (char *)ifc.ifc_req+ifc.ifc_len && i<num; ++ifr) {
        if (ifr->ifr_addr.sa_data == (ifr+1)->ifr_addr.sa_data) {
            continue;  /* duplicate, skip it */
        }

        if (ioctl(sockfd, SIOCGIFFLAGS, ifr)) {
            continue;  /* failed to get flags, skip it */
        }

        if (strcmp(ifr->ifr_name, "lo") == 0)
            continue;

        strncpy(pitem[i].ifac, ifr->ifr_name, sizeof(pitem[i].ifac)-1);

        if (ifr->ifr_addr.sa_family == AF_INET) {
            sock_addr_to_epaddr(&ifr->ifr_addr, &pitem[i].addr);
            sock_addr_to_epaddr(&ifr->ifr_netmask, &pitem[i].netmask);
        } else if (ifr->ifr_addr.sa_family == AF_INET6) {
            sock_addr_to_epaddr(&ifr->ifr_addr, &pitem[i].addr);
            sock_addr_to_epaddr(&ifr->ifr_netmask, &pitem[i].netmask);
        }

        sock_addr_ntop(&ifr->ifr_addr, pitem[i].ipstr);

        /* This won't work on HP-UX 10.20 as there's no SIOCGIFHWADDR ioctl. You'll
         * need to use DLPI or the NETSTAT ioctl on /dev/lan0, etc (and you'll need
         * to be root to use the NETSTAT ioctl. Also this is deprecated and doesn't
         * work on 11.00).
         * On Digital Unix you can use the SIOCRPHYSADDR ioctl according to an old
         * utility I have. Also on SGI I think you need to use a raw socket, e.g. s
         * = socket(PF_RAW, SOCK_RAW, RAWPROTO_SNOOP)
         *   Dave
         *   From: David Peter <dave.peter@eu.citrix.com> */

        if (0 == ioctl(sockfd, SIOCGIFHWADDR, ifr)) {
            /* Select which  hardware types to process.
             *    See list in system include file included from
             *    /usr/include/net/if_arp.h  (For example, on
             *    Linux see file /usr/include/linux/if_arp.h to
             *    get the list.) */
            switch (ifr->ifr_hwaddr.sa_family) {
            default:
                break;
            case ARPHRD_NETROM:
            case ARPHRD_ETHER:
            case ARPHRD_EETHER:
            case ARPHRD_IEEE802:
                pitem[i].type = ADDR_TYPE_ETHERNET;
                break;
            case ARPHRD_PPP:
                pitem[i].type = ADDR_TYPE_PPP;
                break;
            case ARPHRD_SLIP:
            case ARPHRD_CSLIP:
                pitem[i].type = ADDR_TYPE_SLIP;
                break;
            case ARPHRD_LOOPBACK:
                pitem[i].type = ADDR_TYPE_LOOPBACK;
                break;
            case ARPHRD_FDDI:
                pitem[i].type = ADDR_TYPE_FDDI;
                break;
            case ARPHRD_PRONET:
                pitem[i].type = ADDR_TYPE_TOKENRING;
                break;
            }

            memcpy(pitem[i].mac, (char *)&ifr->ifr_addr.sa_data, 6);
        }

        if (0 == ioctl(sockfd, SIOCGIFMTU, ifr)) {
            pitem[i].mtu = ifr->ifr_mtu;
        }

        i++;
    }

    close(sockfd);
    if (ifc.ifc_req) free(ifc.ifc_req);
    return i;
}

#elif defined(_WIN32) || defined(_WIN64)

int get_selfaddr (int num, AddrItem * pitem)
{
    PIP_ADAPTER_INFO  porig = NULL;    // Allocate information for up to 16 NICs
    PIP_ADAPTER_INFO  pAdapterInfo = NULL;    // Allocate information for up to 16 NICs
    DWORD dwBufLen = 0;                        // Save the memory size of buffer
    int   i;

    DWORD dwStatus = GetAdaptersInfo(        // Call GetAdapterInfo
        NULL,                                // [out] buffer to receive data
        &dwBufLen);                            // [in] size of receive data buffer
    if (dwStatus != ERROR_SUCCESS &&
        dwStatus != ERROR_BUFFER_OVERFLOW)
        return -1;                            // Verify return value is valid, no buffer overflow

    porig = pAdapterInfo = malloc(dwBufLen);
    dwStatus = GetAdaptersInfo(                    // Call GetAdapterInfo
        pAdapterInfo,                            // [out] buffer to receive data
        &dwBufLen);                                // [in] size of receive data buffer
    if (dwStatus != ERROR_SUCCESS) return -2;

    for (i=0; i<num && pAdapterInfo; i++) {
        memset(&pitem[i], 0, sizeof(pitem[i]));
        //strcpy(pitem[i].ifac, pAdapterInfo->AdapterName);
        strcpy(pitem[i].ifac, pAdapterInfo->Description);

        strncpy(pitem[i].ipstr, pAdapterInfo->IpAddressList.IpAddress.String, sizeof(pitem[i].ipstr-1));

        sock_addr_parse(pAdapterInfo->IpAddressList.IpAddress.String,
                        strlen(pAdapterInfo->IpAddressList.IpAddress.String),
                        0, &pitem[i].addr);
        sock_addr_parse(pAdapterInfo->IpAddressList.IpMask.String,
                        strlen(pAdapterInfo->IpAddressList.IpMask.String),
                        0, &pitem[i].netmask);

        memcpy(pitem[i].mac,
            (char *)pAdapterInfo->Address,
            pAdapterInfo->AddressLength );        // MAC address

        switch (pAdapterInfo->Type) {
        case MIB_IF_TYPE_OTHER:
            pitem[i].type = ADDR_TYPE_UNKNOWN;
            strcpy(pitem[i].desc, "Unknown");
            break;
          case MIB_IF_TYPE_ETHERNET:
            pitem[i].type = ADDR_TYPE_ETHERNET;
            strcpy(pitem[i].desc, "Ethernet");
            break;
          case MIB_IF_TYPE_TOKENRING:
            pitem[i].type = ADDR_TYPE_TOKENRING;
            strcpy(pitem[i].desc, "TokenRing");
            break;
          case MIB_IF_TYPE_FDDI:
            pitem[i].type = ADDR_TYPE_FDDI;
            strcpy(pitem[i].desc, "FDDI");
            break;
          case MIB_IF_TYPE_PPP:
            pitem[i].type = ADDR_TYPE_PPP;
            strcpy(pitem[i].desc, "PPP");
            break;
          case MIB_IF_TYPE_LOOPBACK:
            pitem[i].type = ADDR_TYPE_LOOPBACK;
            strcpy(pitem[i].desc, "LoopBack");
            break;
          case MIB_IF_TYPE_SLIP:
            pitem[i].type = ADDR_TYPE_SLIP;
            strcpy(pitem[i].desc, "SLIP");
            break;
          case 71:
            pitem[i].type = ADDR_TYPE_WIFI;
            strcpy(pitem[i].desc, "WIFI");
            break;
          default:
            pitem[i].type = ADDR_TYPE_UNKNOWN;
            strcpy(pitem[i].desc, "Unknown");
            break;
        }

        pAdapterInfo = pAdapterInfo->Next;        // Progress through linked list
    }

    free(porig);

    return i;
}

#else

static int addr_item_find (AddrItem * pitem, int num, char * name)
{
    int i;

    if (!name) return -1;

    if (!pitem || num <= 0) return 0;

    for (i = 0; i < num; i++) {
        if (strcasecmp(pitem[i].ifac, name) == 0)
            return i;
    }

    return i;
}

int get_selfaddr (int num, AddrItem * pitem)
{
    struct ifaddrs     * ifa, * curifa;
#ifdef _LINUX_
    struct sockaddr_ll   macaddr;
#else
    struct sockaddr_dl   macaddr;
#endif
    int                  addrnum = 0;
    int                  curind = 0;

    if (num <= 0 || !pitem) return -1;

    memset(pitem, 0, sizeof(AddrItem *) * num);

    if (getifaddrs(&ifa) < 0) return -10;

    for (curifa = ifa; curifa != NULL && addrnum < num; curifa = curifa->ifa_next) {
        if (!curifa->ifa_addr)
            continue;

        if ((curifa->ifa_flags & IFF_UP) == 0)
            continue;

        if (curifa->ifa_flags & IFF_LOOPBACK)
            continue;

        curind = addr_item_find(pitem, addrnum, curifa->ifa_name);
        if (curind < 0) continue;
        if (curind >= addrnum) addrnum++;

        str_secpy(pitem[curind].ifac, sizeof(pitem[curind].ifac)-1, curifa->ifa_name, -1);

        switch (curifa->ifa_addr->sa_family) {
        case AF_INET:
        case AF_INET6:
            if (pitem[curind].addr.socklen <= 0) {
                sock_addr_to_epaddr(curifa->ifa_addr, &pitem[curind].addr);
                sock_addr_ntop(&pitem[curind].addr, pitem[curind].ipstr);
                sock_addr_to_epaddr(curifa->ifa_netmask, &pitem[curind].netmask);

            } else {
                sock_addr_to_epaddr(curifa->ifa_addr, &pitem[curind].addr2);
                sock_addr_ntop(&pitem[curind].addr, pitem[curind].ipstr2);
                sock_addr_to_epaddr(curifa->ifa_netmask, &pitem[curind].netmask2);
            }
            break;

#ifdef _LINUX_
        case AF_PACKET:
            memcpy(&macaddr, curifa->ifa_addr, sizeof(struct sockaddr_ll));
            if (macaddr.sll_halen < 6)
                continue;

            memcpy(pitem[curind].mac, macaddr.sll_addr, macaddr.sll_halen);
            break;
#else
        case AF_LINK:
            memcpy(&macaddr, curifa->ifa_addr, sizeof(struct sockaddr_dl));
            if (macaddr.sdl_alen < 6)
                continue;

            memcpy(pitem[curind].mac, macaddr.sdl_data + macaddr.sdl_nlen, macaddr.sdl_alen);
            break;
#endif
        }
    }

    freeifaddrs(ifa);

    return addrnum;
}

#endif

