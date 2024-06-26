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

#ifdef UNIX

#include "btype.h"
#include "memory.h"
#include <pthread.h>

#include "rwlock.h"


void * rwlock_init  (void * vlock)
{
    rwlock_t * rwlock = (rwlock_t *)vlock;

    if (!rwlock) {
        rwlock = kzalloc(sizeof(*rwlock));
        rwlock->alloc = 1;
    } else{
        rwlock->alloc = 0;
    }

    pthread_mutex_init(&rwlock->lock, NULL);

    pthread_cond_init(&rwlock->readok, NULL);
    pthread_cond_init(&rwlock->writeok, NULL);

    rwlock->waiting_read = 0;
    rwlock->waiting_write = 0;

    rwlock->reading = 0;
    rwlock->writting = 0;

    return rwlock;
}

int rwlock_clean (void * vlock)
{
    rwlock_t * rwlock = (rwlock_t *)vlock;

    if (!rwlock) return -1;

    pthread_mutex_destroy (&rwlock->lock);

    pthread_cond_destroy (&rwlock->readok);
    pthread_cond_destroy (&rwlock->writeok);

    if (rwlock->alloc) kfree(rwlock);

    return 0;
}

int rwlock_read_lock   (void * vlock)
{
    rwlock_t * rwlock = (rwlock_t *)vlock;

    if (!rwlock) return -1;

    pthread_mutex_lock(&rwlock->lock);

    if (rwlock->writting > 0 || rwlock->waiting_write > 0) {
        rwlock->waiting_read++;

        do {
            pthread_cond_wait(&rwlock->readok, &rwlock->lock);
        } while (rwlock->writting > 0 || rwlock->waiting_write > 0);

        rwlock->waiting_read--;
    }

    rwlock->reading++;

    pthread_mutex_unlock(&rwlock->lock);

    return 0;
}

int rwlock_read_unlock (void * vlock)
{
    rwlock_t * rwlock = (rwlock_t *)vlock;

    if (!rwlock) return -1;

    pthread_mutex_lock(&rwlock->lock);

    rwlock->reading--;

    if (rwlock->waiting_write > 0) 
        pthread_cond_signal(&rwlock->writeok);

    pthread_mutex_unlock(&rwlock->lock);

    return 0;
}

int rwlock_write_lock   (void * vlock)
{
    rwlock_t * rwlock = (rwlock_t *)vlock;

    if (!rwlock) return -1;

    pthread_mutex_lock(&rwlock->lock);

    if (rwlock->reading > 0 || rwlock->writting > 0) {
        rwlock->waiting_write++;

        do {
            pthread_cond_wait(&rwlock->writeok, &rwlock->lock);
        } while (rwlock->reading > 0 || rwlock->writting > 0);

        rwlock->waiting_write--;
    }

    rwlock->writting++;

    pthread_mutex_unlock(&rwlock->lock);

    return 0;
}

int rwlock_write_unlock (void * vlock)
{
    rwlock_t * rwlock = (rwlock_t *)vlock;

    if (!rwlock) return -1;

    pthread_mutex_lock(&rwlock->lock);

    rwlock->writting--;

    if (rwlock->waiting_write > 0) {
        pthread_cond_signal(&rwlock->writeok);
    } else if (rwlock->waiting_read > 0) {
        pthread_cond_broadcast(&rwlock->readok);
    }

    pthread_mutex_unlock(&rwlock->lock);

    return 0;
}

#endif
