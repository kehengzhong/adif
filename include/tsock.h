/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */
 
#ifndef _T_SOCK_H_
#define _T_SOCK_H_
 
#ifdef UNIX
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#endif
 
#if defined(_WIN32) || defined(_WIN64)
#include <ws2tcpip.h>
#include <Iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#endif
 
 
#if 0
/* ======== defined in /usr/include/bits/sockaddr.h ========= */
 
typedef unsigned short int sa_family_t;
#define __SOCKADDR_COMMON(sa_prefix) sa_family_t sa_prefix##family
#define __SOCKADDR_COMMON_SIZE  (sizeof (unsigned short int))
 
/* ======== defined in /usr/include/bits/socket.h ========= */
/* Types of sockets.  */
 
#define SOCK_STREAM    1   /* Sequenced, reliable, connection-based byte streams.*/
#define SOCK_DGRAM     2   /* Connectionless, unreliable datagrams of 
                              fixed maximum length.*/
#define SOCK_RAW       3   /* Raw protocol interface.*/
#define SOCK_RDM       4   /* Reliably-delivered messages.*/
#define SOCK_SEQPACKET 5   /* Sequenced, reliable, connection-based, 
                              datagrams of fixed maximum length.*/
#define SOCK_DCCP      6   /* Datagram Congestion Control Protocol.*/
#define SOCK_PACKET    10  /* Linux specific way of getting packets 
                              at the dev level.  For writing rarp and
                              other similar things on the user level. */
 
  /* Flags to be ORed into the type parameter of socket and socketpair and
     used for the flags parameter of paccept.  */
#define SOCK_CLOEXEC   02000000  /* Atomically set close-on-exec flag for the
                                    new descriptor(s).*/
#define SOCK_NONBLOCK  04000     /* Atomically mark descriptor(s) as
                                   non-blocking.*/
 
/* Protocol families.  */
#define PF_UNSPEC       0       /* Unspecified.  */
#define PF_LOCAL        1       /* Local to host (pipes and file-domain).  */
#define PF_UNIX         PF_LOCAL /* POSIX name for PF_LOCAL.  */
#define PF_FILE         PF_LOCAL /* Another non-standard name for PF_LOCAL.  */
#define PF_INET         2       /* IP protocol family.  */
#define PF_INET6        10      /* IP version 6.  */
#define PF_PACKET       17      /* Packet family.  */
#define PF_IRDA         23      /* IRDA sockets.  */
 
/* Address families.  */
#define AF_UNSPEC       PF_UNSPEC
#define AF_LOCAL        PF_LOCAL
#define AF_UNIX         PF_UNIX
#define AF_FILE         PF_FILE
#define AF_INET         PF_INET
#define AF_INET6        PF_INET6
#define AF_PACKET       PF_PACKET
 
/* Standard well-defined IP protocols.  */
enum {
  IPPROTO_IP = 0,               /* Dummy protocol for TCP               */
  IPPROTO_ICMP = 1,             /* Internet Control Message Protocol    */
  IPPROTO_IGMP = 2,             /* Internet Group Management Protocol   */
  IPPROTO_IPIP = 4,             /* IPIP tunnels (older KA9Q tunnels use 94) */
  IPPROTO_TCP = 6,              /* Transmission Control Protocol        */
  IPPROTO_EGP = 8,              /* Exterior Gateway Protocol            */
  IPPROTO_PUP = 12,             /* PUP protocol                         */
  IPPROTO_UDP = 17,             /* User Datagram Protocol               */
  IPPROTO_IDP = 22,             /* XNS IDP protocol                     */
  IPPROTO_DCCP = 33,            /* Datagram Congestion Control Protocol */
  IPPROTO_RSVP = 46,            /* RSVP protocol                        */
  IPPROTO_GRE = 47,             /* Cisco GRE tunnels (rfc 1701,1702)    */
 
  IPPROTO_IPV6   = 41,          /* IPv6-in-IPv4 tunnelling              */
 
  IPPROTO_ESP = 50,            /* Encapsulation Security Payload protocol */
  IPPROTO_AH = 51,             /* Authentication Header protocol       */
  IPPROTO_BEETPH = 94,         /* IP option pseudo header for BEET */
  IPPROTO_PIM    = 103,         /* Protocol Independent Multicast       */
 
  IPPROTO_COMP   = 108,                /* Compression Header protocol */
  IPPROTO_SCTP   = 132,         /* Stream Control Transport Protocol    */
  IPPROTO_UDPLITE = 136,        /* UDP-Lite (RFC 3828)                  */
 
  IPPROTO_RAW    = 255,         /* Raw IP packets                       */
  IPPROTO_MAX
};
 
 
struct sockaddr {
    sa_family_t sa_family;     /* Common data: address family and length.  */ 
    char sa_data[14];           /* Address data.  */
  };
 
/* ======== defined in /usr/include/inet/in.h ========= */
/* Internet address.  */
typedef uint32_t in_addr_t;
struct in_addr {
    in_addr_t s_addr;
  };
 
/* IPv6 address */
struct in6_addr {
    union {
        uint8_t __u6_addr8[16];
#if defined __USE_MISC || defined __USE_GNU
        uint16_t __u6_addr16[8];
        uint32_t __u6_addr32[4];
#endif
      } __in6_u;
#define s6_addr                 __in6_u.__u6_addr8
#if defined __USE_MISC || defined __USE_GNU
# define s6_addr16              __in6_u.__u6_addr16
# define s6_addr32              __in6_u.__u6_addr32
#endif
  };
 
/* Structure describing an Internet socket address.  */
struct sockaddr_in {
    sa_family_t      sin_family;      /* AF_INET */
    in_port_t        sin_port;        /* Port number.  */
    struct in_addr   sin_addr;        /* Internet address.  */
 
    /* Pad to size of `struct sockaddr'.  */
    unsigned char    sin_zero[sizeof (struct sockaddr) -
                           __SOCKADDR_COMMON_SIZE -
                           sizeof (in_port_t) -
                           sizeof (struct in_addr)];
  };
 
/* Ditto, for IPv6.  */
struct sockaddr_in6 {
    sa_family_t      sin6_family;    /* AF_INET6 */
    in_port_t        sin6_port;      /* Transport layer port # */
    uint32_t         sin6_flowinfo;  /* IPv6 flow information */
    struct in6_addr  sin6_addr;      /* IPv6 address */
    uint32_t         sin6_scope_id;  /* IPv6 scope-id */
  };
 
 
/* ======== defined in /usr/include/linux/un.h ========= */
struct sockaddr_un {
        sa_family_t sun_family;        /* AF_UNIX */
        char        sun_path[108];     /* pathname */
};
 
#endif
 
#ifdef __cplusplus
extern "C" {
#endif
 
#define SOM_BACKLOG            0x00000001
#define SOM_REUSEADDR          0x00000002
#define SOM_REUSEPORT          0x00000004
#define SOM_KEEPALIVE          0x00000008
#define SOM_IPV6ONLY           0x00000010
#define SOM_RCVBUF             0x00000020
#define SOM_SNDBUF             0x00000040
#define SOM_RCVTIMEO           0x00000080
#define SOM_SNDTIMEO           0x00000100
#define SOM_TCPKEEPIDLE        0x00000200
#define SOM_TCPKEEPINTVL       0x00000400
#define SOM_TCPKEEPCNT         0x00000800
#define SOM_SETFIB             0x00001000
#define SOM_TCPFASTOPEN        0x00002000
#define SOM_TCPNODELAY         0x00004000
#define SOM_TCPNOPUSH          0x00008000
#define SOM_ACCEPTFILTER       0x00010000
#define SOM_TCPDEFERACCEPT     0x00020000
#define SOM_IPRECVDSTADDR      0x00040000
#define SOM_IPPKTINFO          0x00080000
#define SOM_IPV6RECVPKTINFO    0x00100000
 
typedef struct SockOption_ {
    uint32  mask;
 
    //listen backlog, TCP connection buffer for 3-way handshake finishing
    int     backlog;
 
    //setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&val, sizeof(int))
    int    reuseaddr;
    int    reuseaddr_ret;
 
    //setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (const void *)&val, sizeof(int))
    int    reuseport;
    int    reuseport_ret;
 
    //setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const void *)&val, sizeof(int))
    int    keepalive;
    int    keepalive_ret;
 
    //setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (const void *)&val, sizeof(int))
    int    ipv6only;  //only for AF_INET6
    int    ipv6only_ret;
 
    //setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const void *)&val, sizeof(int))
    int  rcvbuf;
    int  rcvbuf_ret;
 
    //setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const void *)&val, sizeof(int))
    int  sndbuf;
    int  sndbuf_ret;
 
    //setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const void *)&val, sizeof(struct timeval))
    int  rcvtimeo;
    int  rcvtimeo_ret;
 
    //setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const void *)&val, sizeof(struct timeval))
    int  sndtimeo;
    int  sndtimeo_ret;
 
    //setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, (const void *)&val, sizeof(int))
    int  keepidle;
    int  keepidle_ret;
 
    //setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, (const void *)&val, sizeof(int))
    int  keepintvl;
    int  keepintvl_ret;
 
    //setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, (const void *)&val, sizeof(int))
    int  keepcnt;
    int  keepcnt_ret;
 
    //setsockopt(fd, SOL_SOCKET, SO_SETFIB, (const void *)&val, sizeof(int))
    int  setfib;
    int  setfib_ret;
 
    //setsockopt(fd, IPPROTO_TCP, TCP_FASTOPEN, (const void *)&val, sizeof(int))
    int  fastopen;
    int  fastopen_ret;
 
    //setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const void *)&val, sizeof(int))
    int  nodelay;
    int  nodelay_ret;
 
    //setsockopt(fd, IPPROTO_TCP, TCP_NOPUSH, (const void *)&val, sizeof(int))
    int  nopush;
    int  nopush_ret;
 
    //setsockopt(fd, SOL_SOCKET, SO_ACCEPTFILTER, af, sizeof(struct accept_filter_arg))
    struct accept_filter_arg * af;
    int  af_ret;
 
    //setsockopt(fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, (const void *)&val, sizeof(int))
    int  defer_accept;
    int  defer_accept_ret;
 
    //setsockopt(fd, IPPROTO_IP, IP_RECVDSTADDR, (const void *)&val, sizeof(int))
    int  recv_dst_addr;
    int  recv_dst_addr_ret;
 
    //setsockopt(fd, IPPROTO_IP, IP_PKTINFO, (const void *)&val, sizeof(int))
    int  ip_pktinfo;  //only SOCK_DGRAM
    int  ip_pktinfo_ret;
 
    //setsockopt(fd, IPPROTO_IPV6, IPV6_RECVPKTINFO, (const void *)&val, sizeof(int))
    int  ipv6_recv_pktinfo;  //only SOCK_DGRAM
    int  ipv6_recv_pktinfo_ret;
 
} sockopt_t;
 
 
typedef struct EP_SocketAddr_ {
 
    union {
        struct sockaddr        addr;
        struct sockaddr_in     addr4;
        struct sockaddr_in6    addr6;
#ifdef UNIX
        struct sockaddr_un     addrun;
#endif
    } u;
 
    int socklen;
 
    int family;
    int socktype;
 
    struct EP_SocketAddr_ * next;
} ep_sockaddr_t; 
 
typedef struct sockattr_st {
    int      family;
    int      socktype;
    int      protocol;
    SOCKET   fd;
} sockattr_t;

int sock_nonblock_get (SOCKET fd);
int sock_nonblock_set (SOCKET fd, int nbflag);
 
int pipe_create (SOCKET sockfd[2]);
int sock_pair_create (int type, SOCKET sockfd[2]);

int sock_unread_data (SOCKET fd);
 
/* determine socket is open or not */
int sock_is_open (SOCKET fd);
 
 
/* determine if the socket is ready to read or write, 
   the 2nd para is milliseconds waiting for */
int sock_read_ready (SOCKET fd, int ms);
int sock_write_ready (SOCKET fd, int ms);
 
 
void   sock_addr_to_epaddr (void * psa, ep_sockaddr_t * addr);

void   sock_addr_ntop (void * sa, char * buf);
uint16 sock_addr_port (void * sa);
 
int sock_inet_addr_parse (char * text, int len, uint32 * inaddr, int * retlen);
int sock_inet6_addr_parse (char * p, int len, char * addr, int * retlen);
 
/* parset string into sockaddr based on the format of IPv4 or IPv6  */
int sock_addr_parse (char * text, int len, int port, ep_sockaddr_t * addr);
 
/* proto options including: 
     IPPROTO_IP(0), IPPROTO_TCP(6), IPPROTO_UDP(17),
     IPPROTO_RAW(255) IPPROTO_IPV6(41)
   socktype value including: SOCK_STREAM(1), SOCK_DGRAM(2), SOCK_RAW(3) */
int  sock_addr_acquire (ep_sockaddr_t * addr, char * host, int port, int socktype);
 
void sock_addr_freenext (ep_sockaddr_t * addr);
 
int sock_addr_get (char * dst, int dstlen, int port, int socktype,
                   char *ip, int * pport, ep_sockaddr_t * paddr);
 
int sock_option_set (SOCKET fd, sockopt_t * opt);
 
int sock_nodelay_set   (SOCKET fd);
int sock_nodelay_unset (SOCKET fd);
 
int sock_nopush_set   (SOCKET fd);
int sock_nopush_unset (SOCKET fd);
 

void addrinfo_print (void * rp);

SOCKET tcp_listen       (char * localip, int port, void * psockopt, sockattr_t * fdlist, int * fdnum);
 
SOCKET tcp_connect_full (char * host, int port, int nonblk, char * lip, int lport, int * succ, sockattr_t * attr);
SOCKET tcp_connect      (char * host, int port, char * lip, int lport, sockattr_t * attr);
SOCKET tcp_nb_connect   (char * host, int port, char * lip, int lport, int * consucc, sockattr_t * attr);
SOCKET tcp_ep_connect   (ep_sockaddr_t * addr, int nblk, char * lip, int lport, void * popt, int * succ);
 
int    tcp_connected    (SOCKET fd);
 
int    tcp_recv    (SOCKET fd, void * rcvbuf, int toread, long waitms, int * actrcvnum);
int    tcp_send    (SOCKET fd, void * sendbuf, int towrite, long waitms, int * actsndnum);
 
int    tcp_nb_recv (SOCKET fd, void * rcvbuf, int bufsize, int * actrcvnum);
int    tcp_nb_send (SOCKET fd, void * sendbuf, int towrite, int * actnum);
 
int    tcp_writev   (SOCKET fd, void * iov, int iovcnt, int * actnum, int * perr);
int    tcp_sendfile (SOCKET fd, int srcfd, int64 offset, int64 size, int * actnum, int * perr);
 
SOCKET udp_listen (char * localip, int port, void * psockopt, sockattr_t * fdlist, int * fdnum);
 
 
#define ADDR_TYPE_UNKNOWN   0
#define ADDR_TYPE_ETHERNET  1
#define ADDR_TYPE_TOKENRING 2
#define ADDR_TYPE_FDDI      3
#define ADDR_TYPE_PPP       4
#define ADDR_TYPE_LOOPBACK  5
#define ADDR_TYPE_SLIP      6
#define ADDR_TYPE_WIFI      7
 
typedef struct addr_item {
    char              ifac[136];

    char              ipstr[41];
    ep_sockaddr_t     addr;
    ep_sockaddr_t     netmask;

    char              ipstr2[41];
    ep_sockaddr_t     addr2;
    ep_sockaddr_t     netmask2;

    uint8             mac[8];
    int               mtu;
    int               type;
    char              desc[16];
} AddrItem;
 
/* get the ip address of local machine */
int get_selfaddr (int num, AddrItem * pitem);
 
#ifdef __cplusplus
}
#endif
 
#endif

