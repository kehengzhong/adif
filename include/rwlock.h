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

