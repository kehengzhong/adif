/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#ifdef UNIX

#include "btype.h"
#include "tsock.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#define QLEN 100
#define TMP_PATH    "/tmp/cdn.XXXXXX" 

/* Create a server endpoint of a connection.
 * Returns fd if all OK, <0 on error. */
int usock_create (const char *name)
{
    int                 fd, len, err, rval;
    struct sockaddr_un  un;

    /* create a UNIX domain stream socket */
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        return -1;

    len = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*)&len, sizeof(len)) < 0) {
        rval = -4;
        goto errout;
    }
    unlink(name);   /* in case it already exists */

    /* fill in socket address structure */
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, name);
    len = sizeof(un.sun_family) + strlen(un.sun_path);

    /* bind the name to the descriptor */
    if (bind(fd, (struct sockaddr *)&un, len) < 0) {
        rval = -2;
        goto errout;
    }
    if (listen(fd, QLEN) < 0) { /* tell kernel we're a server */
        rval = -3;
        goto errout;
    }

    chmod(name, 0777);
    return fd;

errout:
    err = errno;
    close(fd);
    errno = err;
    return rval;
}


/* Accept a client connection request.
 * Returns fd if all OK, <0 on error.  */
int usock_accept (int listenfd)
{
    int                 clifd, len, err, rval;
    struct sockaddr_un  un;
    struct stat         statbuf;

    len = sizeof(un);
    if ((clifd = accept(listenfd, (struct sockaddr *)&un, (socklen_t *)&len)) < 0)
        return -1;     /* often errno=EINTR, if signal caught */

    len -= sizeof(un.sun_family);
    if (len >= 0) un.sun_path[len] = 0;

    if (stat(un.sun_path, &statbuf) < 0) {
        rval = -2;
        goto errout;
    }

    if (S_ISSOCK(statbuf.st_mode) == 0) {
        rval = -3;      /* not a socket */
        goto errout;
    }

    unlink(un.sun_path);        /* we're done with pathname now */

    return(clifd);

errout:
    err = errno;
    close(clifd);
    errno = err;
    return(rval);
}


/* Create a client endpoint and connect to a server.
 * Returns fd if all OK, <0 on error.  */

int usock_connect (const char *name)
{
    int                fd, len, err, rval;
    struct sockaddr_un un;

    /* create a UNIX domain stream socket */
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        return(-1);

#if 0
    /* fill socket address structure with our address */
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    sprintf(un.sun_path, "%s", TMP_PATH);
    mkstemp(un.sun_path);
    len = sizeof(un.sun_family) + strlen(un.sun_path);

    unlink(un.sun_path);        /* in case it already exists */
    if (bind(fd, (struct sockaddr *)&un, len) < 0) {
        rval = -2;
        goto errout;
    }
#endif

    /* fill socket address structure with server's address */
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, name);
    len = sizeof(un.sun_family) + strlen(un.sun_path);
    if (connect(fd, (struct sockaddr *)&un, len) < 0) {
        rval = -4;
        goto errout;
    }
    return(fd);

errout:
    err = errno;
    close(fd);
    errno = err;
    return(rval);
}


/* Create a client endpoint and connect to a server.
 * Returns fd if all OK, <0 on error.  */

int usock_nb_connect (const char * name, int * succ)
{
    int   fd, len, err;
    struct sockaddr_un un;

    /* create a UNIX domain stream socket */
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        return -1;

    sock_nonblock_set(fd, 1);

    /* fill socket address structure with server's address */
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, name);
    len = sizeof(un.sun_family) + strlen(un.sun_path);

    if (connect(fd, (struct sockaddr *)&un, len) == 0) {
        if (succ) *succ = 1;
    } else {
        if (succ) *succ = 0;

        if ( errno != 0  && 
             errno != EINPROGRESS &&
             errno != EALREADY && 
             errno != EWOULDBLOCK)
        {
            goto errout;
        }
    }

    return fd;

errout:
    err = errno;
    close(fd);
    errno = err;

    return -1;
}

#endif

