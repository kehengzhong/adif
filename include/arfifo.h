/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#ifndef _ARFIFO_H_
#define _ARFIFO_H_ 
    
#ifdef __cplusplus
extern "C" {
#endif 

void * ar_fifo_new   (int size);
void   ar_fifo_free  (void * vaf);
void   ar_fifo_free_all (void * vaf, void * freefunc);

void   ar_fifo_zero  (void * vaf);

int    ar_fifo_num   (void * vaf);
void * ar_fifo_value (void * vaf, int i);

int    ar_fifo_push  (void * vaf, void * value);
void * ar_fifo_out   (void * vaf);

void * ar_fifo_front (void * vaf);
void * ar_fifo_back  (void * vaf);


/******************************************
FIFO example:

void * af = NULL;
void * val = NULL;

af = ar_fifo_new(8);

ar_fifo_push(af, pack1);
ar_fifo_push(af, pack2);
ar_fifo_push(af, pack3);
ar_fifo_push(af, pack4);

val = ar_fifo_out(af);

ar_fifo_free(af);

*******************************************/

#ifdef __cplusplus
}
#endif 
 
#endif

