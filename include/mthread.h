/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#ifndef _MTHREAD_H_
#define _MTHREAD_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef UNIX

#include <pthread.h>

#define INFINITE       ((unsigned long)-1)


/* CRITICAL_SECTION */
#define CRITICAL_SECTION   pthread_mutex_t
#define INIT_STATIC_CS(x)  pthread_mutex_t (x) = PTHREAD_MUTEX_INITIALIZER
int InitializeCriticalSection (CRITICAL_SECTION * cs);
int EnterCriticalSection      (CRITICAL_SECTION * cs);
int LeaveCriticalSection      (CRITICAL_SECTION * cs);
int DeleteCriticalSection     (CRITICAL_SECTION * cs);

/* System V IPC Semaphore API */
int ipc_sem_init (char * path, int proj, int * created);
int ipc_sem_clean (int semid);
int ipc_sem_wait (int semid);
int ipc_sem_signal (int semid);

int ipc_shm_init   (char * path, int proj, int size, void ** ppshm, int * created);
int ipc_shm_detach (void * pshm);
int ipc_shm_clean  (int shmid);


/* File Mutex (advisory lock) based on fcntl() */
void * file_mutex_init    (char * filename);
int    file_mutex_destroy (void * vfmutex);

int    file_mutex_lock    (void * vfmutex);
int    file_mutex_unlock  (void * vfmutex);

int    file_mutex_locked  (char * filename);

#endif  //end if UNIX


/* create an instance of EVENT and initialize it. */
void * event_create ();
 
/* wait the event for specified time until being signaled . 
   the time is milli-second */
int event_wait (void * event, int millisec);
 
/* set the event to signified to wake up the suspended thread. */
void event_set (void * event, int val);
void event_signal (void * event, int val);
 
/* destroy the instance of the event. */
void event_destroy (void * event);

ulong get_threadid ();


#ifdef __cplusplus
}
#endif

#endif /*_MTHREAD_H_*/

