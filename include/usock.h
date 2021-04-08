/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#ifdef UNIX

#ifndef _USOCK_H_
#define _USOCK_H_ 
    
#ifdef __cplusplus
extern "C" {
#endif 


/* Create a server endpoint of a connection.
 * Returns fd if all OK, <0 on error. */
int usock_create (const char *name);


/* Accept a client connection request.
 * Returns fd if all OK, <0 on error.  */
int usock_accept (int listenfd);


/* Create a client endpoint and connect to a server.
 * Returns fd if all OK, <0 on error.  */
int usock_connect (const char *name);

int usock_nb_connect (const char * name, int * succ);


#ifdef __cplusplus
}
#endif 
 
#endif

#endif

