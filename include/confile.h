/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#ifndef _CONFILE_H_
#define _CONFILE_H_

#include "btype.h"

#ifdef UNIX
#include <ctype.h>
#endif
 
#ifdef __cplusplus
extern "C" {
#endif

void  * cfgline_alloc     ();
int     cfgline_free      (void * line);
int     cfgline_cmp_key   (void * a, void * b);

void  * conf_mgmt_init    (char * file);
int     conf_mgmt_cleanup (void * conf);
int     conf_mgmt_read    (void * conf, char * file);
int     conf_mgmt_save    (void * conf, char * file);
    
char  * conf_get_string   (void * conf, char * sect, char * key);
int     conf_get_int      (void * conf, char * sect, char * key); 
uint32  conf_get_ulong    (void * conf, char * sect, char * key);
long    conf_get_hexlong  (void * conf, char * sect, char * key);
double  conf_get_double   (void * conf, char * sect, char * key);
uint8   conf_get_bool     (void * conf, char * sect, char * key);

int     conf_set_string   (void * conf, char * sect, char * key, char * value);
int     conf_set_int      (void * conf, char * sect, char * key, int value);
int     conf_set_ulong    (void * conf, char * sect, char * key, uint32 value);
int     conf_set_hexlong  (void * conf, char * sect, char * key, long value);
int     conf_set_double   (void * conf, char * sect, char * key, double value);
int     conf_set_bool     (void * conf, char * sect, char * key, uint8 value);

#ifdef __cplusplus
}
#endif

#endif

