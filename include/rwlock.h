/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */
 
#ifndef _RWLOCK_H_
#define _RWLOCK_H_
  
#ifdef __cplusplus 
extern "C" { 
#endif 
  
#ifdef UNIX

typedef struct rwlock_s {

    uint8            alloc;
    pthread_mutex_t  lock;

    pthread_cond_t   readok;
    pthread_cond_t   writeok;

    int              waiting_read;
    int              waiting_write;

    int              reading;
    int              writting;

} rwlock_t;

void * rwlock_init  (void * vlock);
int    rwlock_clean (void * vlock);

int    rwlock_read_lock   (void * vlock);
int    rwlock_read_unlock (void * vlock);

int    rwlock_write_lock   (void * vlock);
int    rwlock_write_unlock (void * vlock);

#endif

#ifdef __cplusplus
}
#endif

#endif

