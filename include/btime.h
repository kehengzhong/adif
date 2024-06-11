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

#ifndef _B_TIME_H_
#define _B_TIME_H_ 
    
#ifdef UNIX
#include <sys/time.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif 

/* Binary Time structure */
typedef struct BTime_ {
     long   s; 
     long  ms;
} btime_t;  

/* Return true if the tvp is related to uvp according to the relational
   operator cmp.  Recognized values for cmp are ==, <=, <, >=, and >. */
#define btime_cmp(tvp, cmp, uvp)              \
        (((tvp)->s == (uvp)->s) ?             \
         ((tvp)->ms cmp (uvp)->ms) :          \
         ((tvp)->s cmp (uvp)->s))

int current_timezone ();

/* calling the system function 'ftime' to retrieve the current time,
 * precise can be milli-seconds */
long btime (btime_t * tp);

/* add the specified binary time into the time pointer.*/
void btime_add (btime_t * tp, btime_t tv);

/* add the specified milli-seconds into the btime_t pointer.*/
void btime_add_ms (btime_t * tp, long length);

/* get the current time and add the specified milli-seconds.*/
void btime_now_add (btime_t * tp, long ms);


/* return the btime_t result of 'time1 - time0' */
btime_t btime_diff (btime_t * time0, btime_t * time1);

/* return the milli-seconds result of 'time1 - time0' */
long btime_diff_ms (btime_t * time0, btime_t * time1);

long btime_diff_now (btime_t * tp);

#ifdef __cplusplus
}
#endif 

#endif

