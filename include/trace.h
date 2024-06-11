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

#ifndef _TRACE_H_
#define _TRACE_H_


#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void * g_trace_log;

void * trlog_init (char * logfile, int calcline);
void   trlog_clean (void * vlog);

int trlog_line (void * vlog);
void trlog_rollover (void * vlog, int line);


#define trlog(hlog, rectime, fmt, ...) trlogfile(hlog, rectime, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define tolog(rectime, fmt, ...) trlogfile(g_trace_log, rectime, NULL, __LINE__, fmt, ##__VA_ARGS__)
void trlogfile (void * vlog, int rectime, char * file, int line, char * fmt, ...);


void printOctet(FILE * fp, void * data, int start, int count, int margin);

#ifdef __cplusplus
}
#endif

#endif

