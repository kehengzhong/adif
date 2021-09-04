/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include "btype.h"
#include "memory.h"
#include "mthread.h"

#ifdef UNIX

#include <sys/time.h>
#include <fcntl.h>

#ifdef _LINUX_
#include <linux/sem.h>
#include <linux/shm.h>
#else
#include <sys/types.h>
#include <sys/ipc.h>
#define _WANT_SEMUN
#include <sys/sem.h>
#include <sys/shm.h>
#endif

extern key_t ftok (const char * pathname, int proj_id);

extern int semctl (int semid, int semnum, int cmd, ...);
extern int semget (key_t key, int nsems, int semflg);
extern int semop (int semid, struct sembuf * sops, size_t nsops);
extern int semtimedop (int semid, struct sembuf * sops, size_t nsops,
                       const struct timespec * timeout);

extern int shmctl (int shmid, int cmd, struct shmid_ds * buf);
extern int shmget (key_t key, size_t size, int shmflg);
extern void * shmat (int shmid, const void * shmaddr, int shmflg);
extern int shmdt (const void * shmaddr);


int InitializeCriticalSection(CRITICAL_SECTION * cs)
{
    return pthread_mutex_init(cs, NULL);
}

int DeleteCriticalSection(CRITICAL_SECTION * cs)
{
    return pthread_mutex_destroy(cs);
}


int EnterCriticalSection (CRITICAL_SECTION * cs)
{
    return pthread_mutex_lock(cs);
}

int LeaveCriticalSection(CRITICAL_SECTION * cs)
{
    return pthread_mutex_unlock(cs);
}


int ipc_sem_init (char * path, int proj, int * created)
{
    key_t        keyid = 0;
    int          semid = 0;
    union semun  semopts;

    keyid = ftok(path, proj);
    if (keyid < 0) return -100;

    semid = semget(keyid, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (semid < 0) {
        if (errno != EEXIST) return -200;
        semid = semget(keyid, 1, 0666);
        if (keyid < 0) return -201;
        if (created) *created = 0;
    } else {
        if (created) *created = 1;
    }

    semopts.val = 1;
    if (semctl(semid, 0, SETVAL, semopts) < 0) {
        semctl(semid, 0, IPC_RMID, 0);
        return -300;
    }

    return semid;
}

int ipc_sem_clean (int semid)
{
    if (semid < 0) return -1;
    return semctl(semid, 0, IPC_RMID, 0);
}

int ipc_sem_wait (int semid)
{
    struct sembuf sem;

    if (semid < 0) return -1;

    sem.sem_num = 0;
    sem.sem_op = -1;
    //mark of option£º0 or IPC_NOWAIT and so on
    sem.sem_flg = SEM_UNDO;

    //the '1' in this sentence means the number of commands
    semop(semid, &sem, 1);        
    
    return 0;
}

int ipc_sem_signal (int semid)
{
    struct sembuf sem;

    if (semid < 0) return -1;

    sem.sem_num = 0;
    sem.sem_op = 1;
    //mark of option£º0 or IPC_NOWAIT and so on
    sem.sem_flg = SEM_UNDO;

    //the '1' in this sentence means the number of commands
    semop(semid, &sem, 1);        
    
    return 0;
}


int ipc_shm_init (char * path, int proj, int size, void ** ppshm, int * created)
{
    key_t        keyid = 0;
    int          shmid = 0;

    if (size <= 0) return -1;
    if (!ppshm) return -2;

    keyid = ftok(path, proj);
    if (keyid < 0) return -100;

    shmid = shmget(keyid, size, SHM_R | SHM_W | IPC_CREAT | IPC_EXCL);
    if (shmid < 0) {
        if (errno != EEXIST) return -200;
        shmid = shmget(keyid, size, SHM_R | SHM_W);
        if (shmid < 0) return -201;
        if (created) *created = 0;
    } else {
        if (created) *created = 1;
    }

    *ppshm = shmat(shmid, NULL, 0);
    return shmid;
}

int ipc_shm_detach (void * pshm)
{
    if (!pshm) return -1;
    return shmdt(pshm);
}

int ipc_shm_clean (int shmid)
{
    if (shmid < 0) return -1;

    return shmctl(shmid, IPC_RMID, (struct shmid_ds *)NULL);
}


typedef struct file_mutex {
    char          filename[128];
    struct flock  lock_it;
    struct flock  unlock_it;
    int           fcntl_fd;
} FileMutex;

void * file_mutex_init (char * filename)
{
    FileMutex  * fmutex = NULL;

    if (!filename) return NULL;

    fmutex = kzalloc(sizeof(*fmutex));
    if (!fmutex) return NULL;

    strncpy(fmutex->filename, filename, sizeof(fmutex->filename)-1);
    
    fmutex->lock_it.l_whence = SEEK_SET;   /* from current point */
    fmutex->lock_it.l_start  = 0;          /* -"- */
    fmutex->lock_it.l_len    = 0;          /* until end of file */
    fmutex->lock_it.l_type   = F_WRLCK;    /* set exclusive/write lock */
    fmutex->lock_it.l_pid    = 0;          /* pid not actually interesting */

    fmutex->unlock_it.l_whence = SEEK_SET; /* from current point */
    fmutex->unlock_it.l_start  = 0;        /* -"- */
    fmutex->unlock_it.l_len    = 0;        /* until end of file */
    fmutex->unlock_it.l_type   = F_UNLCK;  /* set exclusive/write lock */
    fmutex->unlock_it.l_pid    = 0;        /* pid not actually interesting */

    //fmutex->fcntl_fd = open(filename, O_CREAT | O_WRONLY | O_EXCL, 0644);
    fmutex->fcntl_fd = open(filename, O_CREAT | O_WRONLY, 0644);
    if (fmutex->fcntl_fd == -1) {
        perror ("file_mutex_init: open lock file failed");
        kfree(fmutex);
        return NULL;
    }

    return fmutex;
}

int file_mutex_destroy (void * vfmutex)
{
    FileMutex * fmutex = (FileMutex *)vfmutex;

    if (!fmutex) return -1;

    if (fmutex->fcntl_fd >= 0) {
        close(fmutex->fcntl_fd);
        fmutex->fcntl_fd = -1;
    }
    unlink(fmutex->filename);

    kfree(fmutex);
    return 0;
}


int file_mutex_lock (void * vfmutex)
{
    FileMutex * fmutex = (FileMutex *)vfmutex;
    int         ret = 0;

    if (!fmutex) return -1;
    if (fmutex->fcntl_fd < 0) return -2;

    while ((ret = fcntl(fmutex->fcntl_fd, F_SETLKW, &fmutex->lock_it)) < 0 && errno == EINTR)
        continue;

    if (ret < 0) {
        perror ("file_mutex_lock: lock failed");
        return -100;
    }

    return 0;
}

int file_mutex_unlock (void * vfmutex)
{
    FileMutex * fmutex = (FileMutex *)vfmutex;
    int         ret = 0;

    if (!fmutex) return -1;
    if (fmutex->fcntl_fd < 0) return -2;

    while ((ret = fcntl(fmutex->fcntl_fd, F_SETLKW, &fmutex->unlock_it)) < 0 
            && errno == EINTR)
        continue;

    if (ret < 0) {
        perror ("file_mutex_unlock: unlock failed");
        return -100;
    }

    return 0;
}

int file_mutex_locked (char * filename) 
{
    int            fd;
    struct flock   lock;

    if (!filename) return -1;

    //fd = open(filename, O_CREAT | O_WRONLY | O_EXCL, 0644);
    fd = open(filename, O_CREAT | O_WRONLY, 0644);
    if (fd == -1) {
        perror ("file_mutex_locked: open lock file failed"); 
        return -100;
    }

    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;

    if (fcntl(fd, F_GETLK, &lock) < 0){
        perror("file_mutex_locked: fcntl GETLK error");
        return -100;
    }

    if (fd >= 0) close(fd);

    if(lock.l_type == F_UNLCK){
        return 0;
    } 

    return lock.l_pid;
}


typedef struct EventObj_ {
    int              value; 
    pthread_cond_t   cond;  
    pthread_mutex_t  mutex;
#ifdef _DEBUG
    int              eventid;
#endif
} eventobj_t;


void * event_create ()
{
    eventobj_t * event = NULL;
#ifdef _DEBUG
    static int geventid = 0;
#endif
 
    event = kzalloc(sizeof(*event));
    if (!event) return NULL;
 
    event->value = -1;
    pthread_cond_init (&event->cond, NULL);
    pthread_mutex_init (&event->mutex, NULL);
 
#ifdef _DEBUG
    event->eventid = ++geventid;
#endif
 
    return event;
}
 
int event_wait (void * vevent, int millisec)
{
    eventobj_t     * event = (eventobj_t *)vevent;
    struct timeval   tv;
    struct timespec  ts;
    int              ret = -1;
 
    if (!event) return -1;
    
    gettimeofday(&tv, NULL);
    ts.tv_sec = tv.tv_sec + millisec/1000;
    ts.tv_nsec = tv.tv_usec * 1000 + (millisec%1000) * 1000 * 1000;
 
    pthread_mutex_lock(&event->mutex);
    event->value = -1;
    ret = pthread_cond_timedwait (&event->cond, &event->mutex, &ts);
    pthread_mutex_unlock(&event->mutex);
 
    if (ret == ETIMEDOUT) return -1;
 
    return event->value;
}
 
void event_set (void * vevent, int val)
{
    eventobj_t * event = (eventobj_t *)vevent;

    if (!event) return;
 
    pthread_mutex_lock(&event->mutex);
    event->value = val;
    pthread_cond_broadcast (&event->cond);
    pthread_mutex_unlock(&event->mutex);
}
 
void event_signal (void * vevent, int val)
{
    eventobj_t * event = (eventobj_t *)vevent;
 
    if (!event) return;
 
    pthread_mutex_lock(&event->mutex);
    event->value = val;
    pthread_cond_signal(&event->cond);
    pthread_mutex_unlock(&event->mutex);
}


void event_destroy (void * vevent)
{
    eventobj_t * event = (eventobj_t *)vevent;

    if (!event) return;
 
    pthread_mutex_destroy (&event->mutex);
    pthread_cond_destroy (&event->cond);
 
    kfree(event);
}

#endif


#if defined(_WIN32) || defined(_WIN64)

#include "memory.h"

typedef struct EventObj_ {
    int              value;
    HANDLE           hev;
#ifdef _DEBUG
    int              eventid;
#endif
} eventobj_t;
 

void * event_create ()
{ 
    eventobj_t * pev = NULL;
#ifdef _DEBUG 
    static int geventid = 0;
#endif
 
    pev = kzalloc(sizeof(*pev));
    if (!pev) return NULL;
 
    pev->value = -1;
    pev->hev = CreateEvent(NULL, FALSE, FALSE, NULL);
 
#ifdef _DEBUG
    pev->eventid = ++geventid;
#endif
  
    return pev;
}
 
int event_wait (void * vobj, int millisec)
{
    eventobj_t * pev = (eventobj_t *)vobj;
    DWORD         ret = -1;

    if (!pev) return -1;
  
    pev->value = -1;
    if (millisec < 0) 
        ret = WaitForSingleObject(pev->hev, INFINITE);
    else
        ret = WaitForSingleObject(pev->hev, (DWORD)millisec);

    if (ret == WAIT_TIMEOUT) return -1;

    return pev->value;
}
 
 
void event_set (void * vobj, int val)
{
    eventobj_t * pev = (eventobj_t *)vobj;

    if (!pev) return;

    pev->value = val;
    SetEvent(pev->hev);
}
 
void event_signal (void * vobj, int val)
{
    eventobj_t * pev = (eventobj_t *)vobj;
 
    if (!pev) return;
 
    pev->value = val;
    SetEvent(pev->hev);
}
 

void event_destroy (void * vobj)
{
    eventobj_t * pev = (eventobj_t *)vobj;

    if (!pev) return;
 
    CloseHandle(pev->hev);
 
    kfree(pev);
}


typedef struct file_mutex {
    char          filename[128];
    HANDLE        lock_handle;
} FileMutex;

void * file_mutex_init (char * filename)
{
    FileMutex  * fmutex = NULL;

    if (!filename) return NULL;
 
    fmutex = kzalloc(sizeof(*fmutex));
    if (!fmutex) return NULL;
 
    strncpy(fmutex->filename, filename, sizeof(fmutex->filename)-1);
    return fmutex;
}

int file_mutex_destroy (void * vfmutex)
{
    FileMutex * fmutex = (FileMutex *)vfmutex;
 
    if (!fmutex) return -1;
 
    if (fmutex->lock_handle) {
        CloseHandle(fmutex->lock_handle);
        fmutex->lock_handle = NULL;
    }
 
    kfree(fmutex);
    return 0;
}
 
 
int file_mutex_lock (void * vfmutex)
{
    FileMutex * fmutex = (FileMutex *)vfmutex;
    int         ret = 0;
 
    if (!fmutex) return -1;
    if (!fmutex->lock_handle) return -2;

    fmutex->lock_handle = CreateMutex(NULL, TRUE, fmutex->filename);
    if (ERROR_ALREADY_EXISTS == GetLastError() || NULL == fmutex->lock_handle)
        return -100;

    WaitForSingleObject(fmutex->lock_handle, INFINITE);

    return 0;
}

int file_mutex_unlock (void * vfmutex)
{
    FileMutex * fmutex = (FileMutex *)vfmutex;
    int         ret = 0;
 
    if (!fmutex) return -1;
    if (!fmutex->lock_handle) return -2;

    ReleaseMutex(fmutex->lock_handle);

    return 0;
}

int file_mutex_locked (char * filename) 
{
    //int            fd;

    //if (!filename) return -1;

    //fd = open(filename, O_CREAT | O_WRONLY | O_EXCL, 0644);

    return 0;
}


#endif


ulong get_threadid ()
{
#ifdef UNIX
#ifdef _FREEBSD_
#include <pthread_np.h>
    return pthread_getthreadid_np();
#else
    return pthread_self();
#endif
#endif

#if defined(_WIN32) || defined(_WIN64)
    return GetCurrentThreadId();
#endif
}

